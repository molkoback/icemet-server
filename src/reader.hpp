#ifndef READER_H
#define READER_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

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
