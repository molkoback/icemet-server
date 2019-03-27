#ifndef WATCHER_H
#define WATCHER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

#include <queue>

class Watcher : Worker {
protected:
	cv::Ptr<File> m_prev;
	
	void findFiles(std::queue<cv::Ptr<File>>& files);
	bool cycle();

public:
	Watcher();
	static void start();
};

#endif
