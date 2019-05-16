#include "worker.hpp"

#include <exception>

Worker::Worker(const std::string& name, const WorkerPointers& ptrs) :
	m_name(name),
	m_log(name),
	m_cfg(ptrs.cfg),
	m_data(ptrs.data),
	m_db(ptrs.db) {}

void Worker::run()
{
	m_log.debug("Running");
	try {
		while (loop());
	}
	catch (std::exception& e) {
		m_log.critical(e.what());
	}
	m_log.debug("Finished");
}
