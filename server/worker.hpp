#ifndef ICEMET_WORKER_H
#define ICEMET_WORKER_H

#include "icemet/img.hpp"
#include "icemet/pkg.hpp"
#include "icemet/util/log.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

typedef enum _worker_data_type {
	WORKER_DATA_IMG = 0,
	WORKER_DATA_PKG
} WorkerDataType;

class WorkerData {
private:
	WorkerDataType m_type;
	ImgPtr m_img;
	PkgPtr m_pkg;

public:
	WorkerData(ImgPtr img) : m_type(WORKER_DATA_IMG), m_img(img) {}
	WorkerData(PkgPtr pkg) : m_type(WORKER_DATA_PKG), m_pkg(pkg) {}
	WorkerData(const WorkerData& data) : m_type(data.m_type), m_img(data.m_img), m_pkg(data.m_pkg) {}
	
	WorkerDataType type() { return m_type; }
	ImgPtr getImg() { return m_img; }
	PkgPtr getPkg() { return m_pkg; }
};

class WorkerQueue {
private:
	std::queue<WorkerData> m_queue;
	std::mutex m_mutex;
	std::atomic<bool> m_closed;
	size_t m_size;
	
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

public:
	WorkerQueue(size_t size) : m_closed(false), m_size(size) {}
	void push(const WorkerData& data);
	void collect(std::queue<WorkerData>& dst);
	bool full();
	bool empty();
	void close() { m_closed.store(true); }
	bool closed() { return m_closed.load(); }
};
typedef cv::Ptr<WorkerQueue> WorkerQueuePtr;

class Worker {
protected:
	std::string m_name;
	Log m_log;
	
	std::vector<WorkerQueuePtr> m_inputs;
	std::vector<WorkerQueuePtr> m_outputs;
	
	virtual bool init() { return true; }
	virtual bool loop() { return false; }
	virtual void close() {}

public:
	Worker(const std::string& name) : m_name(name), m_log(name) {}
	void run();
	void connect(Worker& user, size_t queueSize);
};

#endif
