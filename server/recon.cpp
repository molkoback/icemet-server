#include "recon.hpp"

#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <queue>

Recon::Recon(ICEMETServerContext* ctx) :
	Worker(COLOR_GREEN "RECON" COLOR_RESET, ctx)
{
	m_hologram = cv::makePtr<Hologram>(m_cfg->hologram.psz, m_cfg->hologram.lambda, m_cfg->hologram.dist);
	m_range = ZRange(m_cfg->hologram.z0, m_cfg->hologram.z1, m_cfg->hologram.dz0, m_cfg->hologram.dz1);
}

void Recon::process(ImgPtr img)
{
	const cv::Size2i size = m_cfg->img.size;
	const cv::Size2i border = m_cfg->img.border;
	const cv::Rect2i crop(
		border.width, border.height,
		size.width-2*border.width, size.height-2*border.height
	);
	
	const int reconStep = m_cfg->hologram.reconStep;
	const double focusStep = m_cfg->hologram.focusStep;
	const FocusMethod focusMethod = m_cfg->hologram.focusMethod;
	const FocusMethod focusMethodSmall = m_cfg->hologram.focusMethodSmall;
	
	const int segmSizeMin = m_cfg->segment.sizeMin;
	const int segmSizeMax = m_cfg->segment.sizeMax;
	const int segmSizeSmall = m_cfg->segment.sizeSmall;
	const int pad = m_cfg->segment.pad;
	const int th = m_cfg->segment.thFact * img->bgVal;
	
	int ncontours = 0;
	int nsegments = 0;
	
	// Set our image and apply filters
	m_hologram->setImg(img->preproc);
	if (m_cfg->lpf.f) {
		if (m_lpf.empty())
			m_lpf = m_hologram->createLPF(m_cfg->lpf.f);
		m_hologram->applyFilter(m_lpf);
	}
	
	// Reconstruct whole m_range in steps
	img->min = cv::UMat(size, CV_8UC1, cv::Scalar(255));
	const int nsteps = m_range.n() / reconStep + 1;
	for (int step = 0; step < nsteps; step++) {
		int i0 = step * reconStep;
		int i1 = std::min((step+1) * reconStep, m_range.n()-1);
		ZRange stepRange(m_range.z(i0), m_range.z(i1), m_range.dz(i0), m_range.dz(i1));
		
		// Reconstruct
		cv::UMat imgMin;
		m_hologram->reconMin(m_stack, imgMin, stepRange);
		cv::min(imgMin, img->min, img->min);
		
		// Threshold
		cv::UMat imgTh;
		cv::threshold(imgMin, imgTh, th, 255, cv::THRESH_BINARY_INV);
		
		// Find all contours and process them
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;
		cv::findContours(
			imgTh,
			contours, hierarchy,
			cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE
		);
		
		// Create rects from contours
		ncontours += contours.size();
		for (const auto& cnt : contours) {
			cv::Rect2i rectOrig = cv::boundingRect(cnt);
			
			if ((segmSizeMin > 0 && (rectOrig.width < segmSizeMin || rectOrig.height < segmSizeMin)) ||
			    (segmSizeMax > 0 && (rectOrig.width > segmSizeMax || rectOrig.height > segmSizeMax)) ||
			    (crop & rectOrig).area() < 0.5*rectOrig.area())
				continue;
			
			// Select our focus method
			FocusMethod method = (
				rectOrig.width > segmSizeSmall ||
				rectOrig.height > segmSizeSmall
			) ? focusMethod : focusMethodSmall;
			
			// Create padded rect
			cv::Rect2i rectPad;
			rectPad.x = std::max(rectOrig.x-pad, 0);
			rectPad.y = std::max(rectOrig.y-pad, 0);
			rectPad.width = std::min(rectOrig.width+2*pad, size.width-rectPad.x);
			rectPad.height = std::min(rectOrig.height+2*pad, size.height-rectPad.y);
			
			// Focus
			int idx = 0;
			double score = 0.0;
			Hologram::focus(m_stack, rectPad, idx, score, method, 0, stepRange.n()-1, focusStep);
			
			// Create segment
			SegmentPtr segm = cv::makePtr<Segment>();
			segm->z = stepRange.z(idx);
			segm->step = step;
			segm->score = score;
			segm->method = method;
			segm->rectOrig = rectOrig;
			segm->rectPad = rectPad;
			cv::UMat(m_stack[idx], rectPad).copyTo(segm->img);
			img->segments.push_back(segm);
		}
	}
	
	if ((nsegments = img->segments.size()) == 0)
		img->setStatus(FILE_STATUS_EMPTY);
	m_log.debug("%s: Segments: %d, Contours: %d", img->name().c_str(), nsegments, ncontours);
}

bool Recon::loop()
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
					Measure m;
					m_log.debug("%s: Reconstructing", img->name().c_str());
					process(img);
					m_log.debug("%s: Done (%.2f s)", img->name().c_str(), m.time());
				}
				break;
			}
			case WORKER_DATA_PKG:
				break;
			case WORKER_DATA_MSG:
				if (data.get<WorkerMessage>() == WORKER_MESSAGE_QUIT)
					quit = true;
				break;
		}
		m_outputs[0]->push(data);
	}
	return !quit;
}
