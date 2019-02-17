#ifndef WATCHER_H
#define WATCHER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

#include <queue>

class Watcher : Worker {
protected:
	FileQueue* m_output;
	cv::Ptr<File> m_prev;
	
	void findFiles(std::queue<cv::Ptr<File>>& files);
	bool cycle();

public:
	Watcher(FileQueue* output);
	static void start(FileQueue* output);
};

#endif
