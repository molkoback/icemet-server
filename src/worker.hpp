#ifndef WORKER_H
#define WORKER_H

#include "core/config.hpp"
#include "core/data.hpp"
#include "core/database.hpp"
#include "util/log.hpp"

#include <string>

typedef struct _worker_pointers {
	Config* cfg;
	Data* data;
	Database* db;
} WorkerPointers;

class Worker {
protected:
	std::string m_name;
	Log m_log;
	Config* m_cfg;
	Data* m_data;
	Database* m_db;
	
	virtual bool loop() { return false; }

public:
	Worker(const std::string& name, const WorkerPointers& ptrs);
	void run();
};

#endif
