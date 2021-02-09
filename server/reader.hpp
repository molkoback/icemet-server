#ifndef ICEMET_SERVER_READER_H
#define ICEMET_SERVER_READER_H

#include "icemet/img.hpp"
#include "server/config.hpp"
#include "server/worker.hpp"

#include "icemet/util/time.hpp"

class Reader : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	ImgPtr m_img;
	
	void push();
	bool loop() override;

public:
	Reader(Config* cfg, Database* db);
};

#endif
