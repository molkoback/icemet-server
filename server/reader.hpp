#ifndef ICEMET_SERVER_READER_H
#define ICEMET_SERVER_READER_H

#include "icemet/img.hpp"
#include "server/worker.hpp"

#include "icemet/util/time.hpp"

class Reader : public Worker {
protected:
	ImgPtr m_img;
	
	void push();
	bool loop() override;

public:
	Reader(ICEMETServerContext* ctx);
};

#endif
