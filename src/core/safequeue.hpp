#ifndef SAFEQUEUE_H
#define SAFEQUEUE_H

#include "util/sleep.hpp"

#include <mutex>
#include <queue>

template<class T>
class SafeQueue {
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	size_t m_size;

public:
	SafeQueue(size_t size) : m_size(size) {}
	
	void pushWait(const T& val)
	{
		// Wait until we have space
		while (true) {
			lock();
			if (!full())
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
	
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }
	
	bool full() const { return m_queue.size() >= m_size; }
	
	std::queue<T>& queue() { return m_queue; }
};

#endif
