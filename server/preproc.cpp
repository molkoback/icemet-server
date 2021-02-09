#include "preproc.hpp"

#include "icemet/math.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/imgproc.hpp>

#include <exception>

Preproc::Preproc(Config* cfg) :
	Worker(COLOR_BRIGHT_GREEN "PREPROC" COLOR_RESET),
	m_cfg(cfg),
	m_skip(0)
{
	if (m_cfg->bgsub.stackLen > 0) {
		m_stack = cv::makePtr<BGSubStack>(m_cfg->bgsub.stackLen);
	}
	if (m_cfg->img.rotation != 0.0) {
		cv::Point2f center(m_cfg->img.size.width/2.0, m_cfg->img.size.height/2.0);
		m_rot = cv::getRotationMatrix2D(center, m_cfg->img.rotation, 1.0);
	}
	m_hologram = cv::makePtr<Hologram>(m_cfg->hologram.psz, m_cfg->hologram.lambda, m_cfg->hologram.dist);
}

bool Preproc::isEmpty(const cv::UMat& img, int th, const std::string& imgName, const std::string& checkName) const
{
	if (th <= 0)
		return false;
	double minVal, maxVal;
	minMaxLoc(img, &minVal, &maxVal);
	int delta = maxVal - minVal;
	m_log.debug("%s: EmptyVal %s: %d", imgName.c_str(), checkName.c_str(), delta);
	return delta < th;
}

void Preproc::finalize(ImgPtr img)
{
	img->bgVal = Math::median(img->preproc);
	
	if (m_cfg->emptyCheck.reconTh > 0 || m_cfg->noisyCheck.reconTh > 0) {
		m_hologram->setImg(img->preproc);
		cv::UMat imgMin;
		ZRange z = m_cfg->hologram.z;
		z.step *= 10.0;
		m_hologram->min(imgMin, z);
		
		// Empty check
		if (isEmpty(imgMin, m_cfg->emptyCheck.reconTh, img->name(), "recon")) {
			img->setStatus(FILE_STATUS_EMPTY);
			return;
		}
		
		// Noisy check
		if (m_cfg->noisyCheck.reconTh > 0) {
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
			int ncontours = contours.size();
			m_log.debug("%s: NoisyVal: %d", img->name().c_str(), ncontours);
			if (ncontours > m_cfg->noisyCheck.reconTh) {
				img->setStatus(FILE_STATUS_SKIP);
			}
		}
	}
}

void Preproc::processNoBgsub(ImgPtr img)
{
	// Check empty
	if (isEmpty(img->preproc, m_cfg->emptyCheck.preprocTh, img->name(), "preproc")) {
		img->setStatus(FILE_STATUS_EMPTY);
	}
	else {
		finalize(img);
	}
}

bool Preproc::processBgsub(ImgPtr img, ImgPtr& imgDone)
{
	if (m_stack->push(img)) {
		imgDone = m_stack->meddiv();
		
		// Check empty
		if (isEmpty(imgDone->preproc, m_cfg->emptyCheck.preprocTh, imgDone->name(), "preproc")) {
			imgDone->setStatus(FILE_STATUS_EMPTY);
		}
		else {
			finalize(imgDone);
		}
		return true;
	}
	else if (m_skip < m_stack->len() / 2) {
		imgDone = m_stack->get(m_skip++);
		imgDone->preproc = cv::UMat();
		imgDone->setStatus(FILE_STATUS_SKIP);
		return true;
	}
	return false;
}

bool Preproc::process(ImgPtr img, ImgPtr& imgDone)
{
	// Check empty
	if (isEmpty(img->original, m_cfg->emptyCheck.originalTh, img->name(), "original")) {
		img->setStatus(FILE_STATUS_EMPTY);
		imgDone = img;
		return true;
	}
	
	// Crop and rotate
	cv::UMat tmp;
	cv::UMat(img->original, m_cfg->img.rect).copyTo(tmp);
	if (!m_rot.empty())
		cv::warpAffine(tmp, img->preproc, m_rot, tmp.size());
	else
		img->preproc = tmp;
	
	if (m_stack.empty()) {
		processNoBgsub(img);
		imgDone = img;
		return true;
	}
	return processBgsub(img, imgDone);
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
			m_log.debug("%s: Processing", img->name().c_str());
			Measure m;
			ImgPtr imgDone;
			bool ret = process(img, imgDone);
			m_log.debug("%s: Done (%.2f s)", img->name().c_str(), m.time());
			if (ret)
				m_outputs[0]->push(imgDone);
		}
		else {
			m_outputs[0]->push(data);
		}
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_inputs[0]->empty();
}
