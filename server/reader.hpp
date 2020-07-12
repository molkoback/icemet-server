#ifndef ICEMET_SERVER_READER_H
#define ICEMET_SERVER_READER_H

#include "icemet/img.hpp"
#include "icemet/config.hpp"
#include "server/worker.hpp"

class Reader : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	unsigned int m_id;
	ImgPtr m_img;
	
	bool loop() override;

public:
	Reader(Config* cfg, Database* db);
};

#endif
