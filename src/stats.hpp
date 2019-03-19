#ifndef STATS_H
#define STATS_H

#include "worker.hpp"
#include "core/datetime.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Stats : Worker {
protected:
	FileQueue* m_input;
	double m_V;
	cv::Mat m_particles;
	unsigned int m_frames;
	DateTime m_dt;
	
	void resetStats(const DateTime& dt=DateTime());
	void createStats(StatsRow& row) const;
	bool particleValid(const cv::Ptr<Particle>& par) const;
	void process(const cv::Ptr<File>& file);
	bool cycle();

public:
	Stats(FileQueue* input);
	static void start(FileQueue* input);
};

#endif
