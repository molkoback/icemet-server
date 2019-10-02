#include "preproc.hpp"

#include "icemet/core/math.hpp"
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

void Preproc::process(FilePtr file, cv::UMat& imgPP)
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
			if (dynRange(fileDone->preproc) < m_cfg->check.empty_th)
				fileDone->setEmpty(true);
			else
				fileDone->param.bgVal =  Math::median(fileDone->preproc);
		}
		else {
			fileDone->setEmpty(true);
		}
		m_filesPreproc->push(fileDone);
		m_wait.pop();
	}
}

void Preproc::processNoBgsub(FilePtr file, cv::UMat& imgPP)
{
	file->preproc = imgPP;
	if (dynRange(file->preproc) < m_cfg->check.empty_th)
		file->setEmpty(true);
	else
		file->param.bgVal =  Math::median(file->preproc);
	m_filesPreproc->push(file);
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
		
		// Check dynamic range
		if (dynRange(file->original) < m_cfg->check.discard_th) {
			m_log.warning("Discard %s", file->name().c_str());
			fs::remove(file->path());
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
				process(file, imgPP);
			else
				processNoBgsub(file, imgPP);
		}
		m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		files.pop();
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_filesOriginal->empty();
}
