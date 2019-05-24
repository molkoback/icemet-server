#include "analysis.hpp"

#include "core/math.hpp"
#include "util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#define AREA_MAX 0.70

Analysis::Analysis(Config* cfg) :
	Worker(COLOR_CYAN "ANALYSIS" COLOR_RESET),
	m_cfg(cfg) {}

bool Analysis::init()
{
	m_filesRecon = static_cast<FileQueue*>(m_inputs[0]->data);
	return true;
}

bool Analysis::analyse(const cv::Ptr<File>& file, const cv::Ptr<Segment>& segm, cv::Ptr<Particle>& par) const
{
	// Calculate threshold
	double min, max;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(segm->img, &min, &max, &minLoc, &maxLoc);
	double bg = file->param.bgVal;
	double f = m_cfg->particle().thFact;
	int th = bg - f*(bg-min);
	
	// Apply threshold
	cv::Mat imgTh;
	cv::threshold(segm->img, imgTh, th, 255, cv::THRESH_BINARY_INV);
	
	// Find contours
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(imgTh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	int n = contours.size();
	if (!n) return false;
	
	// Find the centermost contour
	cv::Point_<double> origin(segm->rect.width/2.0, segm->rect.height/2.0);
	cv::Point_<double> center;
	double distMin = std::numeric_limits<double>::infinity();
	int idx = 0;
	for (int i = 0; i < n; i++) {
		cv::Rect rect = cv::boundingRect(contours[i]);
		cv::Point_<double> c(rect.x + rect.width/2.0, rect.y + rect.height/2.0);
		double dx = origin.x - c.x;
		double dy = origin.y - c.y;
		double dist = sqrt(dx*dx + dy*dy);
		if (dist < distMin) {
			distMin = dist;
			center = c;
			idx = i;
		}
	}
	
	// Check if the minimum is inside our contour
	if (cv::pointPolygonTest(contours[idx], minLoc, false) < 0.0)
		return false;
	
	// Draw filled contour
	cv::Mat imgPar = cv::Mat::zeros(imgTh.size(), CV_8UC1);
	cv::drawContours(imgPar, contours, idx, 255, cv::FILLED);
	
	// Calculate area and perimeter
	double area = cv::countNonZero(imgPar);
	//if (area > AREA_MAX*segm->rect.width*segm->rect.height)
	//	return false;
	double perim = cv::arcLength(contours[idx], true);
	
	// Allocate particle
	par = cv::makePtr<Particle>();
	par->img = imgPar;
	par->effPxSz = m_cfg->hologram().psz / cv::icemet::Hologram::magnf(m_cfg->hologram().dist, segm->z);
	
	// Calculate diameter and apply correction
	double diam = par->effPxSz * Math::equivdiam(area);
	double diamCorr = 1.0;
	double D0 = m_cfg->diamCorr().D0;
	double D1 = m_cfg->diamCorr().D1;
	if (m_cfg->diamCorr().enabled && diam < D1 && diam > D0) {
		double f0 = m_cfg->diamCorr().f0;
		double f1 = m_cfg->diamCorr().f1;
		diamCorr = (diam-D0) * (f1-f0) / (D1-D0) + f0;
		diam *= diamCorr;
	}
	
	// Save properties
	par->x = par->effPxSz * (segm->rect.x + center.x - m_cfg->img().border.width);
	par->y = par->effPxSz * (segm->rect.y + center.y - m_cfg->img().border.height);
	par->z = segm->z;
	par->diam = diam;
	par->diamCorr = diamCorr;
	par->circularity = Math::heywood(perim, area);
	par->dynRange = max - min;
	return true;
}

void Analysis::process(cv::Ptr<File> file)
{
	// Sort by size
	std::sort(file->segments.begin(), file->segments.end(), [](const auto& s1, const auto& s2) {
		int A1 = s1->rect.width * s1->rect.height;
		int A2 = s2->rect.width * s2->rect.height;
		return A1 > A2;
	});
	
	// Analyse all segments
	std::vector<cv::Ptr<Segment>> segments;
	std::vector<cv::Ptr<Particle>> particles;
	for (const auto& segm : file->segments) {
		cv::Ptr<Particle> par;
		if (analyse(file, segm, par)) {
			segments.push_back(segm);
			particles.push_back(par);
		}
	}
	
	// Find overlapping segments and select the best
	std::vector<cv::Ptr<Segment>> segmentsUnique;
	std::vector<cv::Ptr<Particle>> particlesUnique;
	int ni = segments.size();
	for (int i = 0; i < ni; i++) {
		const auto& segm = segments[i];
		const auto& par = particles[i];
		
		bool found = false;
		int nj = segmentsUnique.size();
		for (int j = 0; j < nj; j++) {
			const auto& segmOld = segmentsUnique[j];
			const auto& parOld = particlesUnique[j];
			
			// Check overlap
			if ((segm->rect & segmOld->rect).area() > 0) {
				// Compare score or dynamic
				if ((segm->method == segmOld->method && segm->score > segmOld->score) ||
				    (segm->method != segmOld->method && par->dynRange > parOld->dynRange)) {
					segmentsUnique[j] = segm;
					particlesUnique[j] = par;
				}
				found = true;
				break;
			}
		}
		if (!found) {
			segmentsUnique.push_back(segm);
			particlesUnique.push_back(par);
		}
	}
	
	file->segments = segmentsUnique;
	file->particles = particlesUnique;
	int count = file->particles.size();
	if (!count)
		file->setEmpty(true);
	m_log.debug("Particles: %d", count);
}

bool Analysis::loop()
{
	// Collect files
	std::queue<cv::Ptr<File>> files;
	m_filesRecon->collect(files);
	
	// Process
	while (!files.empty()) {
		cv::Ptr<File> file = files.front();
		if (!file->empty()) {
			m_log.debug("Analysing %s", file->name().c_str());
			Measure m;
			process(file);
			m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		}
		for (const auto& conn : m_outputs)
			static_cast<FileQueue*>(conn->data)->push(file);
		files.pop();
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_filesRecon->empty();
}
