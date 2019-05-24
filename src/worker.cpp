#include "worker.hpp" 

#include <algorithm>
#include <cstdlib>
#include <exception>

void Worker::run()
{
	m_log.debug("Running");
	try {
		if (init()) {
			while (loop());
			close();
		}
	}
	catch (std::exception& e) {
		m_log.critical(e.what());
		exit(EXIT_FAILURE);
	}
	
	// Close connections
	for (const auto& conn : m_inputs)
		conn->close();
	for (const auto& conn : m_outputs)
		conn->close();
	m_log.debug("Finished");
}

void Worker::connect(Worker* provider, Worker* user, void* data)
{
	WorkerConnection *conn = new WorkerConnection(provider, user, data);
	provider->m_outputs.push_back(conn);
	user->m_inputs.push_back(conn);
}
