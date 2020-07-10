#ifndef ICEMET_SERVER_SAVER_H
#define ICEMET_SERVER_SAVER_H

#include "server/worker.hpp"
#include "icemet/config.hpp"
#include "icemet/file.hpp"

class Saver : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	FileQueue* m_filesAnalysis;
	
	void move(const fs::path& src, const fs::path& dst) const;
	void process(const FilePtr& file) const;
	bool init() override;
	bool loop() override;

public:
	Saver(Config* cfg, Database* db);
};

#endif
