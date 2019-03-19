#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

#include <vector>

class Analysis : Worker {
protected:
	FileQueue* m_input;
	std::vector<FileQueue*> m_outputs;
	
	bool analyse(const cv::Ptr<File>& file, const cv::Ptr<Segment>& segm, cv::Ptr<Particle>& par) const;
	void process(cv::Ptr<File> file);
	bool cycle();

public:
	Analysis(FileQueue* input, const std::vector<FileQueue*>& outputs);
	static void start(FileQueue* input, const std::vector<FileQueue*>& outputs);
};

#endif
