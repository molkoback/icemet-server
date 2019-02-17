#ifndef PREPROC_H
#define PREPROC_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

class Preproc : Worker {
protected:
	FileQueue* m_input;
	FileQueue* m_output;
	std::queue<cv::Ptr<File>> m_wait;
	cv::Ptr<cv::icemet::BGSubStack> m_stack;
	
	void process(cv::Ptr<File> file);
	bool cycle();

public:
	Preproc(FileQueue* input, FileQueue* output);
	static void start(FileQueue* input, FileQueue* output);
};

#endif
