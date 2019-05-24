#ifndef STATS_H
#define STATS_H

#include "worker.hpp"
#include "core/datetime.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Stats : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	FileQueue* m_filesAnalysis;
	double m_V;
	cv::Mat m_particles;
	unsigned int m_frames;
	DateTime m_dt;
	
	void reset(const DateTime& dt=DateTime());
	void fillStatsRow(StatsRow& row) const;
	void statsPoint() const;
	bool particleValid(const cv::Ptr<Particle>& par) const;
	void process(const cv::Ptr<File>& file);
	bool init() override;
	bool loop() override;
	void close() override;

public:
	Stats(Config* cfg, Database* db);
};

#endif
