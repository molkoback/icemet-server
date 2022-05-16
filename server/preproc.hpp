#ifndef ICEMET_SERVER_PREPROC_H
#define ICEMET_SERVER_PREPROC_H

#include "icemet/img.hpp"
#include "icemet/hologram.hpp"
#include "server/worker.hpp"

#include <opencv2/core.hpp>

#include <queue>

class Preproc : public Worker {
protected:
	cv::Mat m_rot;
	BGSubStackPtr m_stack;
	size_t m_skip;
	HologramPtr m_hologram;
	ZRange m_range;
	
	bool isEmpty(const cv::UMat& img, int th, const std::string& imgName, const std::string& checkName) const;
	void finalize(ImgPtr img);
	bool processBgsub(ImgPtr img, ImgPtr& imgDone);
	void processNoBgsub(ImgPtr img);
	bool process(ImgPtr img,ImgPtr& imgDone );
	bool loop() override;

public:
	Preproc(ICEMETServerContext* ctx);
};

#endif
