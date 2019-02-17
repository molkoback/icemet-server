#include "worker.hpp"

#include <exception>

Worker::Worker(const std::string& name) :
	m_name(name),
	m_log(name),
	m_cfg(Config::getDefaultPtr()),
	m_db(Database::getDefaultPtr()) {}

void Worker::run()
{
	m_log.debug("Running");
	try {
		while (cycle());
	}
	catch (std::exception& e) {
		m_log.critical(e.what());
	}
	m_log.debug("Finished");
}
