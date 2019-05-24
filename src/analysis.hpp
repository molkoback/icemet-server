#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

#include <vector>

class Analysis : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesRecon;
	bool analyse(const cv::Ptr<File>& file, const cv::Ptr<Segment>& segm, cv::Ptr<Particle>& par) const;
	void process(cv::Ptr<File> file);
	bool init() override;
	bool loop() override;

public:
	Analysis(Config* cfg);
};

#endif
