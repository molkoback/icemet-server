#ifndef RECON_H
#define RECON_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <vector>

class Recon : Worker {
protected:
	FileQueue* m_input;
	FileQueue* m_output;
	cv::Ptr<cv::icemet::Hologram> m_hologram;
	std::vector<cv::UMat> m_stack;
	
	void process(cv::Ptr<File> file);
	bool cycle();

public:
	Recon(FileQueue* input, FileQueue* output);
	static void start(FileQueue* input, FileQueue* output);
};

#endif
