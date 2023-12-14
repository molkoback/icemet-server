#ifndef ICEMET_SERVER_STATS_H
#define ICEMET_SERVER_STATS_H

#include "icemet/file.hpp"
#include "icemet/util/time.hpp"
#include "server/worker.hpp"

#include <vector>

class Stats : public Worker {
protected:
	double m_V;
	DateTime m_dtCurr;
	DateTime m_dtPrev;
	Timestamp m_len;
	std::vector<double> m_particles;
	unsigned int m_frames;
	unsigned int m_skipped;
	
	void reset();
	void fillStatsRow(StatsRow& row);
	void statsPoint();
	bool particleValid(const ParticlePtr& par) const;
	void process(const ImgPtr& img);
	bool init() override;
	bool loop() override;
	void close() override;

public:
	Stats(ICEMETServerContext* ctx);
};

#endif
