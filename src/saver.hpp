#ifndef SERVER_H
#define SERVER_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

class Saver : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	FileQueue* m_filesAnalysis;
	
	void moveOriginal(const FilePtr& file) const;
	void processEmpty(const FilePtr& file) const;
	void process(const FilePtr& file) const;
	bool init() override;
	bool loop() override;

public:
	Saver(Config* cfg, Database* db);
};

#endif
