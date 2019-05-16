#ifndef READER_H
#define READER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Reader : public Worker {
protected:
	unsigned int m_id;
	cv::Ptr<File> m_file;
	
	bool loop() override;

public:
	Reader(const WorkerPointers& ptrs);
};

#endif
