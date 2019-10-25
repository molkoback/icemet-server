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
	m_hologram = cv::icemet::Hologram::create(
		m_cfg->img.size,
		m_cfg->hologram.psz, m_cfg->hologram.lambda,
		m_cfg->hologram.dist
	);
	
	// Create filters
	if (m_cfg->lpf.enabled)
		m_lpf = m_hologram->createLPF(m_cfg->lpf.f);
}

bool Recon::init()
{
	m_filesPreproc = static_cast<FileQueue*>(m_inputs[0]->data);
	m_filesRecon = static_cast<FileQueue*>(m_outputs[0]->data);
	return true;
}

bool Recon::isEmpty()
{
	cv::icemet::ZRange z = m_cfg->hologram.z;
	z.step *= 10.0;
	cv::UMat imgMin;
	m_hologram->min(imgMin, z);
	double minVal, maxVal;
	minMaxLoc(imgMin, &minVal, &maxVal);
	return maxVal - minVal < m_cfg->emptyCheck.reconTh;
}

void Recon::process(FilePtr file)
{
	cv::Size2i size = m_cfg->img.size;
	cv::Size2i border = m_cfg->img.border;
	cv::Rect crop(
		border.width, border.height,
		size.width-2*border.width, size.height-2*border.height
	);
	
	float focusK = m_cfg->hologram.focusK;
	int segmNMax = m_cfg->segment.nMax;
	int segmSizeMin = m_cfg->segment.sizeMin;
	int segmSizeSmall = m_cfg->segment.sizeSmall;
	int pad = m_cfg->segment.pad;
	
	cv::icemet::ZRange gz = m_cfg->hologram.z;
	gz.step *= m_cfg->hologram.step;
	cv::icemet::ZRange lz = m_cfg->hologram.z;
	
	int th = m_cfg->segment.thFact * file->param.bgVal;
	
	int iter = 0;
	int count = 0;
	int ncontours = 0;
	
	// Set our image and apply filters
	m_hologram->setImg(file->preproc);
	if (!m_lpf.empty())
		m_hologram->applyFilter(m_lpf);
	
	// Emtpy check
	if (m_cfg->emptyCheck.reconTh > 0 && isEmpty()) {
		file->setEmpty(true);
		goto end;
	}
	// Reconstruct whole range in steps
	for (; gz.start < gz.stop; gz.start += gz.step) {
		lz.start = gz.start;
		lz.stop = std::min(lz.start+gz.step, gz.stop);
		int last = lz.n() - 1;
		cv::UMat imgMin;
		m_hologram->reconMin(m_stack, imgMin, lz);
		
		// Threshold
		cv::UMat imgTh;
		cv::threshold(cv::UMat(imgMin, crop), imgTh, th, 255, cv::THRESH_BINARY_INV);
		
		// Find all contours and process them
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;
		cv::findContours(
			imgTh,
			contours, hierarchy,
			cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE,
			cv::Point(border.width, border.height)
		);
		
		// Create rects from contours
		ncontours += contours.size();
		for (const auto& cnt : contours) {
			cv::Rect rect = cv::boundingRect(cnt);
			if (rect.width < segmSizeMin || rect.height < segmSizeMin)
				continue;
			
			// Select our focus method
			cv::icemet::FocusMethod method = (
				rect.width > segmSizeSmall ||
				rect.height > segmSizeSmall
			) ? cv::icemet::FOCUS_STD : cv::icemet::FOCUS_MIN;
			
			// Grow rect
			rect.x = std::max(rect.x-pad, border.width);
			rect.y = std::max(rect.y-pad, border.height);
			rect.width = std::min(rect.width+2*pad, size.width-border.width-rect.x);
			rect.height = std::min(rect.height+2*pad, size.height-border.height-rect.y);
			
			// Focus
			int idx = 0;
			double score = 0.0;
			cv::icemet::Hologram::focus(m_stack, rect, idx, score, method, 0, last, focusK);
			
			// Create segment
			SegmentPtr segm = cv::makePtr<Segment>();
			segm->z = lz.z(idx);
			segm->iter = iter;
			segm->score = score;
			segm->method = method;
			segm->rect = rect;
			cv::UMat(m_stack[idx], rect).copyTo(segm->img);
			file->segments.push_back(segm);
			
			if (++count >= segmNMax) {
				m_log.warning("The maximum number of segments reached");
				goto end;
			}
		}
		iter++;
	}
	if (!count)
		file->setEmpty(true);
end:
	m_log.debug("Segments: %d, Contours: %d", count, ncontours);
}

bool Recon::loop()
{
	// Collect files
	std::queue<FilePtr> files;
	m_filesPreproc->collect(files);
	
	// Process
	while (!files.empty()) {
		FilePtr file = files.front();
		if (!file->empty()) {
			m_log.debug("Reconstructing %s", file->name().c_str());
			Measure m;
			process(file);
			m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		}
		m_filesRecon->push(file);
		files.pop();
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_filesPreproc->empty();
}
