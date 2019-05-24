#ifndef WORKER_H
#define WORKER_H

#include "util/log.hpp"
#include "util/time.hpp"

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

class Worker;

template<class T>
class WorkerQueue {
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	size_t m_size;
	
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

public:
	WorkerQueue(size_t size) : m_size(size) {}
	
	void push(const T& val)
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
		m_queue.push(val);
		unlock();
	}
	
	void collect(std::queue<T>& dst)
	{
		lock();
		while (!m_queue.empty()) {
			dst.push(m_queue.front());
			m_queue.pop();
		}
		unlock();
	}
	
	bool full()
	{
		lock();
		bool isFull = m_queue.size() >= m_size;
		unlock();
		return isFull;
	}
	
	bool empty()
	{
		lock();
		bool isEmpty = m_queue.empty();
		unlock();
		return isEmpty;
	}
};

class WorkerConnection {
private:
	std::atomic<bool> m_closed;

public:
	WorkerConnection(Worker* provider, Worker* user, void* data) :
		m_closed(false),
		provider(provider),
		user(user),
		data(data) {}
	
	Worker* provider;
	Worker* user;
	void* data;
	
	void close() { m_closed.store(true); }
	bool closed() { return m_closed.load(); }
};

class Worker {
protected:
	std::string m_name;
	Log m_log;
	
	std::vector<WorkerConnection*> m_inputs;
	std::vector<WorkerConnection*> m_outputs;
	
	virtual bool init() { return true; }
	virtual bool loop() { return false; }
	virtual void close() {}

public:
	Worker(const std::string& name) : m_name(name), m_log(name) {}
	void run();
	
	static void connect(Worker* provider, Worker* user, void* data);
};

#endif
