#include "preproc.hpp"

#include "core/math.hpp"
#include "util/sleep.hpp"
#include "util/measure.hpp"

#include <exception>

/* Opened images with Max-Min difference smaller than this will be discarded
 * immediately. */
#define PREPROC_TH 50

Preproc::Preproc(FileQueue* input, FileQueue* output) :
	Worker(COLOR_BRIGHT_GREEN "PREPROC" COLOR_RESET),
	m_input(input),
	m_output(output)
{
	m_stack = cv::icemet::BGSubStack::create(
		m_cfg->img().size,
		m_cfg->bgsub().stackLen
	);
}

void Preproc::process(cv::Ptr<File> file)
{
	m_log.debug("Processing %s", file->name().c_str());
	Measure m;
	
	// Check range
	double minVal, maxVal;
	minMaxLoc(file->original, &minVal, &maxVal);
	if (maxVal-minVal < PREPROC_TH) {
		m_log.warning("Discard %s", file->name().c_str());
		fs::remove(file->path());
		return;
	}
	
	// Crop
	cv::UMat tmp;
	file->original.copyTo(tmp);
	cv::UMat(tmp, m_cfg->img().rect).copyTo(file->preproc);
	
	// Push to background subtraction stack
	m_wait.push(file);
	if (m_stack->push(file->preproc)) {
		cv::Ptr<File> done = m_wait.front();
		m_stack->meddiv(done->preproc);
		
		// Get background parameters
		done->param.med =  Math::median(done->preproc);
		
		m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		m_output->pushWait(done);
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
