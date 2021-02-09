#ifndef ICEMET_SERVER_ANALYSIS_H
#define ICEMET_SERVER_ANALYSIS_H

#include "icemet/img.hpp"
#include "server/worker.hpp"

#include <vector>

class Analysis : public Worker {
protected:
	bool analyse(const ImgPtr& img, const SegmentPtr& segm, ParticlePtr& par) const;
	void process(ImgPtr img);
	bool loop() override;

public:
	Analysis(ICEMETServerContext* ctx);
};

#endif
