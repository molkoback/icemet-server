#ifndef SERVER_H
#define SERVER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Saver : Worker {
protected:
	FileQueue* m_input;
	
	void moveOriginal(const cv::Ptr<File>& file);
	void processEmpty(const cv::Ptr<File>& file);
	void process(const cv::Ptr<File>& file);
	bool cycle();

public:
	Saver(FileQueue* input);
	static void start(FileQueue* input);
};

#endif
