#include "preproc.hpp"

#include "core/math.hpp"
#include "util/sleep.hpp"
#include "util/measure.hpp"

#include <exception>

/* Opened images with dynamic range smaller than this will be removed. */
#define ORIGINAL_EMPTY_TH 50

/* Preprocessed images with dynamic range smaller than this will be marked as empty. */
#define PREPROC_EMPTY_TH 10

Preproc::Preproc(FileQueue* input, FileQueue* output) :
	Worker(COLOR_BRIGHT_GREEN "PREPROC" COLOR_RESET),
	m_input(input),
	m_output(output)
{
	m_stackLen = m_cfg->bgsub().stackLen;
	m_stack = cv::icemet::BGSubStack::create(m_cfg->img().size, m_stackLen);
}

int Preproc::dynRange(const cv::UMat& img)
{
	double minVal, maxVal;
	minMaxLoc(img, &minVal, &maxVal);
	return maxVal - minVal;
}

void Preproc::process(cv::Ptr<File> file)
{
	m_log.debug("Processing %s", file->name().c_str());
	Measure m;
	
	// Check dynamic range
	if (dynRange(file->original) < ORIGINAL_EMPTY_TH) {
		m_log.warning("Discard %s", file->name().c_str());
		fs::remove(file->path());
		return;
	}
	
	// Crop
	cv::UMat imgCrop;
	cv::UMat(file->original, m_cfg->img().rect).copyTo(imgCrop);
	
	// Push to background subtraction stack
	bool full = m_stack->push(imgCrop);
	
	// Files go through a queue so we maintain the order
	m_wait.push(file);
	if (m_wait.size() >= m_stackLen/2 + 1) {
		cv::Ptr<File> fileDone = m_wait.front();
		
		// Perform median division if the background subtraction stack was full
		if (full) {
			fileDone->preproc = cv::UMat(m_cfg->img().size, CV_8UC1);
			m_stack->meddiv(fileDone->preproc);
			
			// Check dynamic range
			if (dynRange(fileDone->preproc) < PREPROC_EMPTY_TH)
				fileDone->setEmpty(true);
			else
				fileDone->param.med =  Math::median(fileDone->preproc);
		}
		else {
			fileDone->setEmpty(true);
		}
		m_log.debug("Done %s (%.2f s)", fileDone->name().c_str(), m.time());
		m_output->pushWait(fileDone);
		m_wait.pop();
	}
}

bool Preproc::cycle()
{
	// Collect files
	std::queue<cv::Ptr<File>> files;
	m_input->collect(files);
	
	// Process
	while (!files.empty()) {
		process(files.front());
		files.pop();
	}
	msleep(1);
	return true;
}

void Preproc::start(FileQueue* input, FileQueue* output)
{
	Preproc(input, output).run();
}
