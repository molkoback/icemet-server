#ifndef ICEMET_SERVER_ANALYSIS_H
#define ICEMET_SERVER_ANALYSIS_H

#include "server/worker.hpp"
#include "icemet/config.hpp"
#include "icemet/file.hpp"

#include <vector>

class Analysis : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesRecon;
	bool analyse(const FilePtr& file, const SegmentPtr& segm, ParticlePtr& par) const;
	void process(FilePtr file);
	bool init() override;
	bool loop() override;

public:
	Analysis(Config* cfg);
};

#endif
