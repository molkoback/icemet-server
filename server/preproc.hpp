#ifndef ICEMET_SERVER_PREPROC_H
#define ICEMET_SERVER_PREPROC_H

#include "icemet/img.hpp"
#include "icemet/config.hpp"
#include "server/worker.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

class Preproc : public Worker {
protected:
	Config* m_cfg;
	cv::Mat m_rot;
	size_t m_stackLen;
	cv::Ptr<cv::icemet::BGSubStack> m_stack;
	std::queue<ImgPtr> m_wait; // Length: m_stackLen/2 + 1
	cv::Ptr<cv::icemet::Hologram> m_hologram;
	
	bool isEmpty(const cv::UMat& img, int th, const std::string& imgName, const std::string& checkName) const;
	void finalize(ImgPtr img);
	void processBgsub(ImgPtr img, cv::UMat& imgPP);
	void processNoBgsub(ImgPtr img, cv::UMat& imgPP);
	void process(ImgPtr img);
	bool loop() override;

public:
	Preproc(Config* cfg);
};

#endif
