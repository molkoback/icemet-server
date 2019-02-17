#ifndef WORKER_H
#define WORKER_H

#include "core/config.hpp"
#include "core/database.hpp"
#include "util/log.hpp"

#include <string>

class Worker {
protected:
	std::string m_name;
	Log m_log;
	Config* m_cfg;
	Database* m_db;
	
	virtual bool cycle() = 0;

public:
	Worker(const std::string& name);
	void run();
};

#endif
