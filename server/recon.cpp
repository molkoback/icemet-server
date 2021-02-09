#include "recon.hpp"

#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <queue>

Recon::Recon(Config* cfg) :
	Worker(COLOR_GREEN "RECON" COLOR_RESET),
	m_cfg(cfg)
{
	m_hologram = cv::makePtr<Hologram>(m_cfg->hologram.psz, m_cfg->hologram.lambda, m_cfg->hologram.dist);
	if (m_cfg->lpf.f)
		m_lpf = m_hologram->createLPF(m_cfg->lpf.f);
}

void Recon::process(ImgPtr img)
{
	const cv::Size2i size = m_cfg->img.size;
	const cv::Size2i border = m_cfg->img.border;
	const cv::Rect2i crop(
		border.width, border.height,
		size.width-2*border.width, size.height-2*border.height
	);
	
	const float focusK = m_cfg->hologram.focusK;
	const int segmSizeMin = m_cfg->segment.sizeMin;
	const int segmSizeMax = m_cfg->segment.sizeMax;
	const int segmSizeSmall = m_cfg->segment.sizeSmall;
	const int pad = m_cfg->segment.pad;
	
	const int th = m_cfg->segment.thFact * img->bgVal;
	
	ZRange gz = m_cfg->hologram.z;
	gz.step *= m_cfg->hologram.step;
	ZRange lz = m_cfg->hologram.z;
	
	int iter = 0;
	int ncontours = 0;
	int nsegments = 0;
	
	// Set our image and apply filters
	m_hologram->setImg(img->preproc);
	if (!m_lpf.empty())
		m_hologram->applyFilter(m_lpf);
	
	// Reconstruct whole range in steps
	for (; gz.start < gz.stop; gz.start += gz.step) {
		lz.start = gz.start;
		lz.stop = std::min(lz.start+gz.step, gz.stop);
		int last = lz.n() - 1;
		cv::UMat imgMin;
		m_hologram->reconMin(m_stack, imgMin, lz);
		
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
			cv::Rect2i rect = cv::boundingRect(cnt);
			
			if ((segmSizeMin > 0 && (rect.width < segmSizeMin || rect.height < segmSizeMin)) ||
			    (segmSizeMax > 0 && (rect.width > segmSizeMax || rect.height > segmSizeMax)) ||
                (crop & rect).area() < 0.5*rect.area())
				continue;
			
			// Select our focus method
			FocusMethod method = (
				rect.width > segmSizeSmall ||
				rect.height > segmSizeSmall
			) ? FOCUS_STD : FOCUS_MIN;
			
			// Grow rect
			rect.x = std::max(rect.x-pad, 0);
			rect.y = std::max(rect.y-pad, 0);
			rect.width = std::min(rect.width+2*pad, size.width-rect.x);
			rect.height = std::min(rect.height+2*pad, size.height-rect.y);
			
			// Focus
			int idx = 0;
			double score = 0.0;
			Hologram::focus(m_stack, rect, idx, score, method, 0, last, focusK);
			
			// Create segment
			SegmentPtr segm = cv::makePtr<Segment>();
			segm->z = lz.z(idx);
			segm->iter = iter;
			segm->score = score;
			segm->method = method;
			segm->rect = rect;
			cv::UMat(m_stack[idx], rect).copyTo(segm->img);
			img->segments.push_back(segm);
		}
		iter++;
	}
	if ((nsegments = img->segments.size()) == 0)
		img->setStatus(FILE_STATUS_EMPTY);
	m_log.debug("%s: Segments: %d, Contours: %d", img->name().c_str(), nsegments, ncontours);
}

bool Recon::loop()
{
	std::queue<WorkerData> queue;
	m_inputs[0]->collect(queue);
	
	while (!queue.empty()) {
		WorkerData data = queue.front();
		queue.pop();
		if (data.type() == WORKER_DATA_IMG) {
			ImgPtr img = data.getImg();
			if (img->status() == FILE_STATUS_NONE) {
				Measure m;
				m_log.debug("%s: Reconstructing", img->name().c_str());
				process(img);
				m_log.debug("%s: Done (%.2f s)", img->name().c_str(), m.time());
			}
		}
		m_outputs[0]->push(data);
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_inputs[0]->empty();
}
