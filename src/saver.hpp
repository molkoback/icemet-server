#ifndef SERVER_H
#define SERVER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Saver : public Worker {
protected:
	void moveOriginal(const cv::Ptr<File>& file) const;
	void processEmpty(const cv::Ptr<File>& file) const;
	void process(const cv::Ptr<File>& file) const;
	bool loop() override;

public:
	Saver(const WorkerPointers& ptrs);
};

#endif
