#ifndef ICEMET_SERVER_PREPROC_H
#define ICEMET_SERVER_PREPROC_H

#include "icemet/worker.hpp"
#include "icemet/core/config.hpp"
#include "icemet/core/file.hpp"

#include <opencv2/icemet.hpp>

#include <queue>

class Preproc : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesOriginal;
	FileQueue* m_filesPreproc;
	size_t m_stackLen;
	cv::Ptr<cv::icemet::BGSubStack> m_stack;
	std::queue<FilePtr> m_wait; // Length: m_stackLen/2 + 1
	
	int dynRange(const cv::UMat& img) const;
	void process(FilePtr file);
	void processNoBgsub(FilePtr file);
	bool init() override;
	bool loop() override;

public:
	Preproc(Config* cfg);
};

#endif
