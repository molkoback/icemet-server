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
	
	// Find the largest contour
	double area = 0;
	int idx = 0;
	for (int i = 0; i < n; i++) {
		double a = cv::contourArea(contours[i]);
		if (a > area) {
			area = a;
			idx = i;
		}
	}
	
	// Make sure the area and perimeter are valid, the minimum is inside the contour and the contour doesn't touch the edges
	cv::Rect bbox = cv::boundingRect(contours[idx]);
	double perim = cv::arcLength(contours[idx], true);
	if (area == 0.0 || perim == 0.0 ||
	    cv::pointPolygonTest(contours[idx], minLoc, false) < 0.0 ||
	    bbox.x == 0 || bbox.y == 0 ||
	    (bbox.x+bbox.width) == size.width ||
	    (bbox.y+bbox.height) == size.height)
		return false;
	
	// Find the contour center
	cv::Moments m = cv::moments(contours[idx]);
	cv::Point_<double> center;
	center.x = m.m10 / m.m00;
	center.y = m.m01 / m.m00;
	
	// Allocate particle
	par = cv::makePtr<Particle>();
	par->effPxSz = m_cfg->hologram.psz / Hologram::magnf(m_cfg->hologram.dist, segm->z);
	
	// Draw filled contour
	par->img = cv::Mat::zeros(size, CV_8UC1);
	cv::drawContours(par->img, contours, idx, 255, cv::FILLED, cv::LINE_8);
	
	// Calculate diameter and apply correction
	par->diam = par->effPxSz * Math::equivdiam(area) / scaleF;
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
	par->x = par->effPxSz * (segm->rectPad.x + center.x/scaleF - m_cfg->img.size.width/2.0);
	par->y = par->effPxSz * -(segm->rectPad.y + center.y/scaleF - m_cfg->img.size.height/2.0);
	par->z = segm->z;
	par->circularity = Math::heywood(perim, area);
	par->dynRange = max - min;
	return true;
}

void Analysis::process(ImgPtr img)
{
	// Sort by size
	std::sort(img->segments.begin(), img->segments.end(), [](const auto& s1, const auto& s2) {
		return s1->rectOrig.area() > s2->rectOrig.area();
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
			if ((segm->rectOrig & segmOld->rectOrig).area() > 0) {
				// Decide the better particle
				if ((segm->step == segmOld->step && segm->rectOrig.area() > segmOld->rectOrig.area()) ||
				    (segm->step != segmOld->step && (
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
	m_log.debug("{}: Particles: {}", img->name(), count);
}

bool Analysis::loop()
{
	std::queue<WorkerData> queue;
	m_inputs[0]->collect(queue);
	if (queue.empty())
		msleep(1);
	
	bool quit = false;
	while (!queue.empty()) {
		WorkerData data(queue.front());
		queue.pop();
		switch (data.type()) {
			case WORKER_DATA_IMG: {
				ImgPtr img = data.get<ImgPtr>();
				if (img->status() == FILE_STATUS_NONE) {
					m_log.debug("{}: Analysing", img->name());
					Measure m;
					process(img);
					m_log.debug("{}: Done ({:.2f} s)", img->name(), m.time());
				}
				break;
			}
			case WORKER_DATA_PKG:
				break;
			case WORKER_DATA_MSG:
				if (data.get<WorkerMessage>() == WORKER_MESSAGE_QUIT)
					quit = true;
		}
		for (const auto& output : m_outputs)
			output->push(data);
	}
	return !quit;
}
