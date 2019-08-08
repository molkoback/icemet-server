#ifndef ICEMET_SERVER_STATS_H
#define ICEMET_SERVER_STATS_H

#include "icemet/worker.hpp"
#include "icemet/core/config.hpp"
#include "icemet/core/file.hpp"
#include "icemet/util/time.hpp"

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
	bool particleValid(const ParticlePtr& par) const;
	void process(const FilePtr& file);
	bool init() override;
	bool loop() override;
	void close() override;

public:
	Stats(Config* cfg, Database* db);
};

#endif
