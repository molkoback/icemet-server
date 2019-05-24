#ifndef PREPROC_H
#define PREPROC_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

class Preproc : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesOriginal;
	FileQueue* m_filesPreproc;
	size_t m_stackLen;
	cv::Ptr<cv::icemet::BGSubStack> m_stack;
	std::queue<cv::Ptr<File>> m_wait; // Length: m_stackLen/2 + 1
	
	int dynRange(const cv::UMat& img) const;
	void process(cv::Ptr<File> file);
	bool init() override;
	bool loop() override;

public:
	Preproc(Config* cfg);
};

#endif
