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

bool Preproc::init()
{
	m_filesOriginal = static_cast<FileQueue*>(m_inputs[0]->data);
	m_filesPreproc = static_cast<FileQueue*>(m_outputs[0]->data);
	return true;
}

int Preproc::dynRange(const cv::UMat& img) const
{
	double minVal, maxVal;
	minMaxLoc(img, &minVal, &maxVal);
	return maxVal - minVal;
}

void Preproc::finalize(FilePtr file)
{
	file->param.bgVal =  Math::median(file->preproc);
	
	if (m_cfg->emptyCheck.reconTh > 0 || m_cfg->noisyCheck.contours > 0) {
		m_hologram->setImg(file->preproc);
		cv::UMat imgMin;
		cv::icemet::ZRange z = m_cfg->hologram.z;
		z.step *= 10.0;
		m_hologram->min(imgMin, z);
		
		// Empty check
		if (m_cfg->emptyCheck.reconTh > 0) {
			double minVal, maxVal;
			minMaxLoc(imgMin, &minVal, &maxVal);
			if (maxVal - minVal < m_cfg->emptyCheck.reconTh) {
				file->setStatus(FILE_STATUS_EMPTY);
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
			const int th = m_cfg->segment.thFact * file->param.bgVal;
			
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
				file->setStatus(FILE_STATUS_SKIP);
				goto end;
			}
		}
	}
end:
	m_filesPreproc->push(file);
}

void Preproc::processBgsub(FilePtr file, cv::UMat& imgPP)
{
	// Push to background subtraction stack
	bool full = m_stack->push(imgPP);
	
	// Files go through a queue so we maintain the order
	m_wait.push(file);
	if (m_wait.size() >= m_stackLen/2 + 1) {
		FilePtr fileDone = m_wait.front();
		
		// Perform median division if the background subtraction stack was full
		if (full) {
			fileDone->preproc = cv::UMat(m_cfg->img.size, CV_8UC1);
			m_stack->meddiv(fileDone->preproc);
			
			// Check dynamic range
			if (dynRange(fileDone->preproc) < m_cfg->emptyCheck.preprocTh) {
				fileDone->setStatus(FILE_STATUS_EMPTY);
				m_filesPreproc->push(fileDone);
			}
			else {
				finalize(fileDone);
			}
		}
		else {
			fileDone->setStatus(FILE_STATUS_SKIP);
			m_filesPreproc->push(fileDone);
		}
		m_wait.pop();
	}
}

void Preproc::processNoBgsub(FilePtr file, cv::UMat& imgPP)
{
	file->preproc = imgPP;
	if (dynRange(file->preproc) < m_cfg->emptyCheck.preprocTh) {
		file->setStatus(FILE_STATUS_EMPTY);
		m_filesPreproc->push(file);
	}
	else {
		finalize(file);
	}
}

void Preproc::process(FilePtr file)
{
	// Check dynamic range
	if (dynRange(file->original) < m_cfg->emptyCheck.originalTh) {
		file->setStatus(FILE_STATUS_EMPTY);
		m_filesPreproc->push(file);
	}
	else {
		// Crop and rotate
		cv::UMat imgCrop, imgPP;
		cv::UMat(file->original, m_cfg->img.rect).copyTo(imgCrop);
		if (!m_rot.empty())
			cv::warpAffine(imgCrop, imgPP, m_rot, imgCrop.size());
		else
			imgPP = imgCrop;
		
		if (m_cfg->bgsub.enabled)
			processBgsub(file, imgPP);
		else
			processNoBgsub(file, imgPP);
	}
}

bool Preproc::loop()
{
	// Collect files
	std::queue<FilePtr> files;
	m_filesOriginal->collect(files);
	
	// Process
	while (!files.empty()) {
		FilePtr file = files.front();
		m_log.debug("Processing %s", file->name().c_str());
		Measure m;
		process(file);
		m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		files.pop();
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_filesOriginal->empty();
}
