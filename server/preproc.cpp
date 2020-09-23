#include "preproc.hpp"

#include "icemet/math.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/imgproc.hpp>

#include <exception>

Preproc::Preproc(Config* cfg) :
	Worker(COLOR_BRIGHT_GREEN "PREPROC" COLOR_RESET),
	m_cfg(cfg)
{
	if (m_cfg->bgsub.enabled) {
		m_stackLen = m_cfg->bgsub.stackLen;
		m_stack = cv::icemet::BGSubStack::create(m_cfg->img.size, m_stackLen);
	}
	if (m_cfg->img.rotation != 0.0) {
		cv::Point2f center(m_cfg->img.size.width/2.0, m_cfg->img.size.height/2.0);
		m_rot = cv::getRotationMatrix2D(center, m_cfg->img.rotation, 1.0);
	}
	m_hologram = cv::icemet::Hologram::create(
		m_cfg->img.size,
		m_cfg->hologram.psz, m_cfg->hologram.lambda,
		m_cfg->hologram.dist
	);
}

int Preproc::dynRange(const cv::UMat& img) const
{
	double minVal, maxVal;
	minMaxLoc(img, &minVal, &maxVal);
	return maxVal - minVal;
}

void Preproc::finalize(ImgPtr img)
{
	img->bgVal = Math::median(img->preproc);
	
	if (m_cfg->emptyCheck.reconTh > 0 || m_cfg->noisyCheck.contours > 0) {
		m_hologram->setImg(img->preproc);
		cv::UMat imgMin;
		cv::icemet::ZRange z = m_cfg->hologram.z;
		z.step *= 10.0;
		m_hologram->min(imgMin, z);
		
		// Empty check
		if (m_cfg->emptyCheck.reconTh > 0) {
			if (dynRange(imgMin) < m_cfg->emptyCheck.reconTh) {
				img->setStatus(FILE_STATUS_EMPTY);
				goto end;
			}
		}
		
		// Noisy check
		if (m_cfg->noisyCheck.contours > 0) {
			const cv::Size2i size = m_cfg->img.size;
			const cv::Size2i border = m_cfg->img.border;
			const cv::Rect crop(
				border.width, border.height,
				size.width-2*border.width, size.height-2*border.height
			);
			const int th = m_cfg->segment.thFact * img->bgVal;
			
			// Threshold
			cv::UMat imgTh;
			cv::threshold(cv::UMat(imgMin, crop), imgTh, th, 255, cv::THRESH_BINARY_INV);
			
			// Find contours
			std::vector<std::vector<cv::Point>> contours;
			std::vector<cv::Vec4i> hierarchy;
			cv::findContours(
				imgTh,
				contours, hierarchy,
				cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE
			);
			if ((int)contours.size() > m_cfg->noisyCheck.contours) {
				img->setStatus(FILE_STATUS_SKIP);
				goto end;
			}
		}
	}
end:
	m_outputs[0]->push(img);
}

void Preproc::processBgsub(ImgPtr img, cv::UMat& imgPP)
{
	// Push to background subtraction stack
	bool full = m_stack->push(imgPP);
	
	// Files go through a queue so we maintain the order
	m_wait.push(img);
	if (m_wait.size() >= m_stackLen/2 + 1) {
		ImgPtr imgDone = m_wait.front();
		m_wait.pop();
		
		// Perform median division if the background subtraction stack was full
		if (full) {
			imgDone->preproc = cv::UMat(m_cfg->img.size, CV_8UC1);
			m_stack->meddiv(imgDone->preproc);
			
			// Check dynamic range
			if (dynRange(imgDone->preproc) < m_cfg->emptyCheck.preprocTh) {
				imgDone->setStatus(FILE_STATUS_EMPTY);
				m_outputs[0]->push(imgDone);
			}
			else {
				finalize(imgDone);
			}
		}
		else {
			imgDone->setStatus(FILE_STATUS_SKIP);
			m_outputs[0]->push(imgDone);
		}
	}
}

void Preproc::processNoBgsub(ImgPtr img, cv::UMat& imgPP)
{
	img->preproc = imgPP;
	if (dynRange(img->preproc) < m_cfg->emptyCheck.preprocTh) {
		img->setStatus(FILE_STATUS_EMPTY);
		m_outputs[0]->push(img);
	}
	else {
		finalize(img);
	}
}

void Preproc::process(ImgPtr img)
{
	// Check dynamic range
	if (dynRange(img->original) < m_cfg->emptyCheck.originalTh) {
		img->setStatus(FILE_STATUS_EMPTY);
		m_outputs[0]->push(img);
	}
	else {
		// Crop and rotate
		cv::UMat imgCrop, imgPP;
		cv::UMat(img->original, m_cfg->img.rect).copyTo(imgCrop);
		if (!m_rot.empty())
			cv::warpAffine(imgCrop, imgPP, m_rot, imgCrop.size());
		else
			imgPP = imgCrop;
		
		if (m_cfg->bgsub.enabled)
			processBgsub(img, imgPP);
		else
			processNoBgsub(img, imgPP);
	}
}

bool Preproc::loop()
{
	std::queue<WorkerData> queue;
	m_inputs[0]->collect(queue);
	
	while (!queue.empty()) {
		WorkerData data = queue.front();
		queue.pop();
		if (data.type() == WORKER_DATA_IMG) {
			ImgPtr img = data.getImg();
			m_log.debug("Processing %s", img->name().c_str());
			Measure m;
			process(img);
			m_log.debug("Done %s (%.2f s)", img->name().c_str(), m.time());
		}
		else {
			m_outputs[0]->push(data);
		}
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_inputs[0]->empty();
}
