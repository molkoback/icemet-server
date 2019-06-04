#ifndef RECON_H
#define RECON_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

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
