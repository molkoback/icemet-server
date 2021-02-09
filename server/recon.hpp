#ifndef ICEMET_SERVER_RECON_H
#define ICEMET_SERVER_RECON_H

#include "icemet/img.hpp"
#include "server/config.hpp"
#include "icemet/hologram.hpp"
#include "server/worker.hpp"

#include <vector>

class Recon : public Worker {
protected:
	Config* m_cfg;
	HologramPtr m_hologram;
	std::vector<cv::UMat> m_stack;
	cv::UMat m_lpf;
	
	void process(ImgPtr img);
	bool loop() override;

public:
	Recon(Config* cfg);
};

#endif
