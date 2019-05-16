#ifndef STATS_H
#define STATS_H

#include "worker.hpp"
#include "core/datetime.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Stats : public Worker {
protected:
	double m_V;
	cv::Mat m_particles;
	unsigned int m_frames;
	DateTime m_dt;
	
	void reset(const DateTime& dt=DateTime());
	void fillStatsRow(StatsRow& row) const;
	void statsPoint() const;
	bool particleValid(const cv::Ptr<Particle>& par) const;
	void process(const cv::Ptr<File>& file);
	bool loop() override;

public:
	Stats(const WorkerPointers& ptrs);
};

#endif
