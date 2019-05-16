#ifndef DATA_H
#define DATA_H

#include "core/file.hpp"
#include "util/time.hpp"

#include <atomic>
#include <mutex>
#include <queue>

template<class T>
class SafeQueue {
private:
	std::queue<T> m_queue;
	std::mutex m_mutex;
	size_t m_size;
	std::atomic<bool> m_closed;

public:
	SafeQueue(size_t size) : m_size(size), m_closed(false) {}
	
	void pushWait(const T& val)
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
	
	void close() { m_closed.store(true); }
	bool done() { return m_closed.load() && empty(); }
	
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }
	
	std::queue<T>& queue() { return m_queue; }
};

typedef SafeQueue<cv::Ptr<File>> FileQueue;

class Data {
public:
	Data() : original(4), preproc(2), recon(2), analysisSaver(2), analysisStats(2) {}
	
	FileQueue original;
	FileQueue preproc;
	FileQueue recon;
	FileQueue analysisSaver;
	FileQueue analysisStats;
};

#endif
