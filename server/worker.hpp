#ifndef ICEMET_WORKER_H
#define ICEMET_WORKER_H

#include "icemet/img.hpp"
#include "icemet/pkg.hpp"
#include "icemet/util/log.hpp"
#include "icemet/util/time.hpp"
#include "server/server.hpp"

#include <opencv2/core.hpp>

#include <atomic>
#include <mutex>
#include <queue>
#include <string>
#include <variant>
#include <vector>

typedef enum _worker_message {
	WORKER_MESSAGE_QUIT = 0
} WorkerMessage;

typedef enum _worker_data_type {
	WORKER_DATA_IMG = 0,
	WORKER_DATA_PKG,
	WORKER_DATA_MSG
} WorkerDataType;

class WorkerData {
private:
	WorkerDataType m_type;
	std::variant<ImgPtr, PkgPtr, WorkerMessage> m_data;

public:
	WorkerData(ImgPtr img) : m_type(WORKER_DATA_IMG), m_data(img) {}
	WorkerData(PkgPtr pkg) : m_type(WORKER_DATA_PKG), m_data(pkg) {}
	WorkerData(WorkerMessage msg) : m_type(WORKER_DATA_MSG), m_data(msg) {}
	WorkerData(const WorkerData& data) : m_type(data.m_type)
	{
		switch (data.m_type) {
			case WORKER_DATA_IMG:
				m_data = std::get<ImgPtr>(data.m_data);
				break;
			case WORKER_DATA_PKG:
				m_data = std::get<PkgPtr>(data.m_data);
				break;
			case WORKER_DATA_MSG:
				m_data = std::get<WorkerMessage>(data.m_data);
				break;
		}
	}
	
	WorkerDataType type() { return m_type; }
	
	template <class T>
	T get() { return std::get<T>(m_data); }
};

class WorkerQueue {
private:
	std::queue<WorkerData> m_queue;
	std::mutex m_mutex;
	size_t m_size;
	
	void lock() { m_mutex.lock(); }
	void unlock() { m_mutex.unlock(); }

public:
	WorkerQueue(size_t size) : m_size(size) {}
	void push(const WorkerData& data);
	void collect(std::queue<WorkerData>& dst);
	bool full();
	bool empty();
};
typedef cv::Ptr<WorkerQueue> WorkerQueuePtr;

class Worker {
protected:
	std::string m_name;
	Log m_log;
	Arguments* m_args;
	Config* m_cfg;
	Database* m_db;
	
	std::vector<WorkerQueuePtr> m_inputs;
	std::vector<WorkerQueuePtr> m_outputs;
	
	virtual bool init() { return true; }
	virtual bool loop() { return false; }
	virtual void close() {}

public:
	Worker(const std::string& name, ICEMETServerContext* ctx);
	void run();
	void connect(Worker& user, size_t queueSize);
};

#endif
