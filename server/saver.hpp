#ifndef ICEMET_SERVER_SERVER_H
#define ICEMET_SERVER_SERVER_H

#include "icemet/worker.hpp"
#include "icemet/core/config.hpp"
#include "icemet/core/file.hpp"

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
