#ifndef ANALYSIS_H
#define ANALYSIS_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

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
