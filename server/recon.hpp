#ifndef ICEMET_SERVER_RECON_H
#define ICEMET_SERVER_RECON_H

#include "icemet/img.hpp"
#include "icemet/hologram.hpp"
#include "server/worker.hpp"

#include <vector>

class Recon : public Worker {
protected:
	HologramPtr m_hologram;
	ZRange m_range;
	std::vector<cv::UMat> m_stack;
	cv::UMat m_lpf;
	
	void process(ImgPtr img);
	bool loop() override;

public:
	Recon(ICEMETServerContext* ctx);
};

#endif
