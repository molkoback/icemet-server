#ifndef ICEMET_SERVER_READER_H
#define ICEMET_SERVER_READER_H

#include "server/worker.hpp"
#include "icemet/config.hpp"
#include "icemet/file.hpp"

class Reader : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	FileQueue* m_filesAnalysis;
	unsigned int m_id;
	FilePtr m_file;
	
	bool init() override;
	bool loop() override;

public:
	Reader(Config* cfg, Database* db);
};

#endif
