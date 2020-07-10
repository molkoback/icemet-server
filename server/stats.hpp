#ifndef ICEMET_SERVER_STATS_H
#define ICEMET_SERVER_STATS_H

#include "server/worker.hpp"
#include "icemet/config.hpp"
#include "icemet/file.hpp"
#include "icemet/util/time.hpp"

class Stats : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	FileQueue* m_filesAnalysis;
	double m_V;
	DateTime m_dt;
	Timestamp m_len;
	cv::Mat m_particles;
	unsigned int m_frames;
	
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
