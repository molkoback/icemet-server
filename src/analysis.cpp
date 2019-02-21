#include "analysis.hpp"

#include "core/math.hpp"
#include "util/sleep.hpp"
#include "util/measure.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <limits>
#include <queue>

#define AREA_MAX 0.70

Analysis::Analysis(FileQueue* input, const std::vector<FileQueue*>& outputs) :
	Worker(COLOR_CYAN "ANALYSIS" COLOR_RESET),
	m_input(input),
	m_outputs(outputs) {}

bool Analysis::analyse(const cv::Ptr<File>& file, const cv::Ptr<Segment>& segm, cv::Ptr<Particle>& par)
{
	// Calculate threshold
	double min, max;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(segm->img, &min, &max, &minLoc, &maxLoc);
	double med = file->param.med;
	double f = m_cfg->particle().thFact;
	int th = med - f*(med-min);
	
	// Threshold
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
	if (area > AREA_MAX*segm->rect.width*segm->rect.height)
		return false;
	double perim = cv::arcLength(contours[idx], true);
	
	// Save properties
	par = cv::makePtr<Particle>();
	par->img = imgPar;
	par->effpsz = m_cfg->hologram().psz / Math::Mz(m_cfg->hologram().dist, segm->z);
	par->x = par->effpsz * (segm->rect.x + center.x - m_cfg->img().border.width);
	par->y = par->effpsz * (segm->rect.y + center.y - m_cfg->img().border.height);
	par->z = segm->z;
	par->diam = par->effpsz * Math::equivdiam(area);
	par->circularity = Math::heywood(perim, area);
	par->dnr = max - min;
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
	for (int i = 0; i < (int)segments.size(); i++) {
		const auto& segm = segments[i];
		const auto& par = particles[i];
		
		bool found = false;
		for (int j = 0; j < (int)segmentsUnique.size(); j++) {
			const auto& segmOld = segmentsUnique[j];
			const auto& parOld = particlesUnique[j];
			
			// Check overlap
			if ((segm->rect & segmOld->rect).area() > 0) {
				// Compare score or dynamic
				if ((segm->method == segmOld->method && segm->score > segmOld->score) ||
				    (segm->method != segmOld->method && par->dnr > parOld->dnr)) {
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
	m_log.debug("Particles: %d", (int)file->particles.size());
}

bool Analysis::cycle()
{
	// Collect files
	std::queue<cv::Ptr<File>> files;
	m_input->collect(files);
	
	// Process
	while (!files.empty()) {
		cv::Ptr<File> file = files.front();
		m_log.debug("Analysing %s", file->name().c_str());
		Measure m;
		process(file);
		m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		for (const auto& output : m_outputs)
			output->pushWait(file);
		files.pop();
	}
	msleep(1);
	return true;
}

void Analysis::start(FileQueue* input, const std::vector<FileQueue*>& outputs)
{
	Analysis(input, outputs).run();
}
