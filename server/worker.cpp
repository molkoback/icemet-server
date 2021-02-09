#include "worker.hpp" 

#include <algorithm>
#include <cstdlib>
#include <exception>

void WorkerQueue::push(const WorkerData& data)
{
	// Wait until we have space
	while (true) {
		lock();
		if (m_queue.size() < m_size)
			break;
		unlock();
		msleep(1);
	}
	
	// Push to queue
	m_queue.push(data);
	unlock();
}

void WorkerQueue::collect(std::queue<WorkerData>& dst)
{
	lock();
	while (!m_queue.empty()) {
		dst.push(m_queue.front());
		m_queue.pop();
	}
	unlock();
}

bool WorkerQueue::full()
{
	lock();
	bool isFull = m_queue.size() >= m_size;
	unlock();
	return isFull;
}

bool WorkerQueue::empty()
{
	lock();
	bool isEmpty = m_queue.empty();
	unlock();
	return isEmpty;
}

Worker::Worker(const std::string& name, ICEMETServerContext* ctx) :
	m_name(name),
	m_ctx(ctx),
	m_log(name),
	m_cfg(ctx->cfg),
	m_db(ctx->db) {}

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
	
	// Close queue
	for (const auto& queue : m_inputs)
		queue->close();
	for (const auto& queue : m_outputs)
		queue->close();
	m_log.debug("Finished");
}

void Worker::connect(Worker& user, size_t queueSize)
{
	WorkerQueuePtr queue = cv::makePtr<WorkerQueue>(queueSize);
	m_outputs.push_back(queue);
	user.m_inputs.push_back(queue);
}
