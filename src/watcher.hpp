#ifndef WATCHER_H
#define WATCHER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

#include <queue>

class Watcher : public Worker {
protected:
	cv::Ptr<File> m_prev;
	
	void findFiles(std::queue<cv::Ptr<File>>& files);
	bool loop() override;

public:
	Watcher(const WorkerPointers& ptrs);
};

#endif
