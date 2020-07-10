#ifndef ICEMET_SERVER_PREPROC_H
#define ICEMET_SERVER_PREPROC_H

#include "server/worker.hpp"
#include "icemet/config.hpp"
#include "icemet/file.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

class Preproc : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesOriginal;
	FileQueue* m_filesPreproc;
	cv::Mat m_rot;
	size_t m_stackLen;
	cv::Ptr<cv::icemet::BGSubStack> m_stack;
	std::queue<FilePtr> m_wait; // Length: m_stackLen/2 + 1
	cv::Ptr<cv::icemet::Hologram> m_hologram;
	
	int dynRange(const cv::UMat& img) const;
	void finalize(FilePtr file);
	void processBgsub(FilePtr file, cv::UMat& imgPP);
	void processNoBgsub(FilePtr file, cv::UMat& imgPP);
	void process(FilePtr file);
	bool init() override;
	bool loop() override;

public:
	Preproc(Config* cfg);
};

#endif
