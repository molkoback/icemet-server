#ifndef READER_H
#define READER_H

#include "worker.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Reader : Worker {
protected:
	unsigned int m_id;
	cv::Ptr<File> m_file;
	
	bool cycle();

public:
	Reader();
	static void start();
};

#endif
