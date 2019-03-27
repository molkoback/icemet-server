#ifndef RECON_H
#define RECON_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <vector>

class Recon : Worker {
protected:
	cv::Ptr<cv::icemet::Hologram> m_hologram;
	std::vector<cv::UMat> m_stack;
	cv::UMat m_lpf;
	
	void process(cv::Ptr<File> file);
	bool cycle();

public:
	Recon();
	static void start();
};

#endif
