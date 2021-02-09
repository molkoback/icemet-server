#include "analysis.hpp"

#include "icemet/hologram.hpp"
#include "icemet/math.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

#define AREA_MAX 0.70

Analysis::Analysis(ICEMETServerContext* ctx) :
	Worker(COLOR_CYAN "ANALYSIS" COLOR_RESET, ctx) {}

bool Analysis::analyse(const ImgPtr& img, const SegmentPtr& segm, ParticlePtr& par) const
{
	// Upscale smaller particles
	cv::Mat imgScaled;
	int smallAxis = std::min(segm->img.cols, segm->img.rows);
	double scaleF = smallAxis < m_cfg->segment.scale ? m_cfg->segment.scale / smallAxis : 1.0;
	cv::resize(segm->img, imgScaled, cv::Size(), scaleF, scaleF, cv::INTER_LANCZOS4);
	cv::Size2f size = imgScaled.size();
	
	// Save minimum
	double min, max;
	cv::Point minLoc, maxLoc;
	cv::minMaxLoc(imgScaled, &min, &max, &minLoc, &maxLoc);
	
	// Calculate threshold
	double bg = img->bgVal;
	double f = m_cfg->particle.thFact;
	int th = bg - f*(bg-min);
	
	// Apply threshold
	cv::Mat imgTh;
	cv::threshold(imgScaled, imgTh, th, 255, cv::THRESH_BINARY_INV);
	
	// Find contours
	std::vector<std::vector<cv::Point>> contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(imgTh, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	int n = contours.size();
	if (!n) return false;
	
	// Find the centermost contour
	cv::Point_<double> origin(size.width/2.0, size.height/2.0);
	double area = 0;
	cv::Point_<double> center;
	double distMin = std::numeric_limits<double>::infinity();
	int idx = 0;
	for (int i = 0; i < n; i++) {
		cv::Moments m = cv::moments(contours[i]);
		double cx = m.m10 / m.m00;
		double cy = m.m01 / m.m00;
		double dx = origin.x - cx;
		double dy = origin.y - cy;
		double dist = sqrt(dx*dx + dy*dy);
		if (dist < distMin) {
			distMin = dist;
			center.x = cx;
			center.y = cy;
			area = m.m00;
			idx = i;
		}
	}
	
	// Check if the minimum is inside our contour and the area is valid
	if (cv::pointPolygonTest(contours[idx], minLoc, false) < 0.0 ||
		area > AREA_MAX*size.width*size.height)
		return false;
	
	// Allocate particle
	par = cv::makePtr<Particle>();
	par->effPxSz = m_cfg->hologram.psz / Hologram::magnf(m_cfg->hologram.dist, segm->z);
	
	// Draw filled contour
	par->img = cv::Mat::zeros(size, CV_8UC1);
	cv::drawContours(par->img, contours, idx, 255, cv::FILLED, cv::LINE_8);
	
	// Calculate diameter and apply correction
	par->diam= par->effPxSz * Math::equivdiam(area) / scaleF;
	par->diamCorr = 1.0;
	float D0 = m_cfg->diamCorr.D0;
	float D1 = m_cfg->diamCorr.D1;
	if (m_cfg->diamCorr.enabled && par->diam < D1 && par->diam > D0) {
		float f0 = m_cfg->diamCorr.f0;
		float f1 = m_cfg->diamCorr.f1;
		par->diamCorr = (par->diam-D0) * (f1-f0) / (D1-D0) + f0;
		par->diam *= par->diamCorr;
	}
	
	// Save properties
	par->x = par->effPxSz * (segm->rect.x + center.x - m_cfg->img.border.width) / scaleF;
	par->y = par->effPxSz * (segm->rect.y + center.y - m_cfg->img.border.height) / scaleF;
	par->z = segm->z;
	par->circularity = Math::heywood(cv::arcLength(contours[idx], true), area);
	par->dynRange = max - min;
	return true;
}

void Analysis::process(ImgPtr img)
{
	// Sort by size
	std::sort(img->segments.begin(), img->segments.end(), [](const auto& s1, const auto& s2) {
		int A1 = s1->rect.width * s1->rect.height;
		int A2 = s2->rect.width * s2->rect.height;
		return A1 > A2;
	});
	
	// Analyse all segments
	std::vector<SegmentPtr> segments;
	std::vector<ParticlePtr> particles;
	for (const auto& segm : img->segments) {
		ParticlePtr par;
		if (analyse(img, segm, par)) {
			segments.push_back(segm);
			particles.push_back(par);
		}
	}
	
	// Find overlapping segments and select the best
	std::vector<SegmentPtr> segmentsUnique;
	std::vector<ParticlePtr> particlesUnique;
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
				// Decide the better particle
				if ((segm->iter == segmOld->iter && segm->rect.area() > segmOld->rect.area()) ||
				    (segm->iter != segmOld->iter && (
						(segm->method == segmOld->method && segm->score > segmOld->score) ||
						(segm->method != segmOld->method && par->dynRange > parOld->dynRange)
					))) {
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
	
	img->segments = segmentsUnique;
	img->particles = particlesUnique;
	int count = img->particles.size();
	img->setStatus(count ? FILE_STATUS_NOTEMPTY : FILE_STATUS_EMPTY);
	m_log.debug("%s: Particles: %d", img->name().c_str(), count);
}

bool Analysis::loop()
{
	std::queue<WorkerData> queue;
	m_inputs[0]->collect(queue);
	
	while (!queue.empty()) {
		WorkerData data = queue.front();
		queue.pop();
		if (data.type() == WORKER_DATA_IMG) {
			ImgPtr img = data.getImg();
			if (img->status() == FILE_STATUS_NONE) {
				m_log.debug("%s: Analysing", img->name().c_str());
				Measure m;
				process(img);
				m_log.debug("%s: Done (%.2f s)", img->name().c_str(), m.time());
			}
		}
		for (const auto& output : m_outputs)
			output->push(data);
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_inputs[0]->empty();
}
