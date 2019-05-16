#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

#include <vector>

class Analysis : public Worker {
protected:
	bool analyse(const cv::Ptr<File>& file, const cv::Ptr<Segment>& segm, cv::Ptr<Particle>& par) const;
	void process(cv::Ptr<File> file);
	bool loop() override;

public:
	Analysis(const WorkerPointers& ptrs);
};

#endif
