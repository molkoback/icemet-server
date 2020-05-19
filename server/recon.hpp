#ifndef ICEMET_SERVER_RECON_H
#define ICEMET_SERVER_RECON_H

#include "icemet/worker.hpp"
#include "icemet/core/config.hpp"
#include "icemet/core/file.hpp"

#include <opencv2/icemet.hpp>

#include <vector>

class Recon : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesPreproc;
	FileQueue* m_filesRecon;
	cv::Ptr<cv::icemet::Hologram> m_hologram;
	std::vector<cv::UMat> m_stack;
	cv::UMat m_lpf;
	
	void process(FilePtr file);
	bool init() override;
	bool loop() override;

public:
	Recon(Config* cfg);
};

#endif
