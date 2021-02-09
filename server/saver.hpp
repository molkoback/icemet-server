#ifndef ICEMET_SERVER_SAVER_H
#define ICEMET_SERVER_SAVER_H

#include "icemet/icemet.hpp"
#include "icemet/img.hpp"
#include "icemet/pkg.hpp"
#include "server/worker.hpp"

class Saver : public Worker {
protected:
	void move(const fs::path& src, const fs::path& dst) const;
	void processImg(const ImgPtr& img) const;
	void processPkg(const PkgPtr& pkg) const;
	bool loop() override;

public:
	Saver(ICEMETServerContext* ctx);
};

#endif
