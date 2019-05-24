#ifndef SERVER_H
#define SERVER_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

#include <opencv2/core.hpp>

class Saver : public Worker {
protected:
	Config* m_cfg;
	Database* m_db;
	FileQueue* m_filesAnalysis;
	
	void moveOriginal(const cv::Ptr<File>& file) const;
	void processEmpty(const cv::Ptr<File>& file) const;
	void process(const cv::Ptr<File>& file) const;
	bool init() override;
	bool loop() override;

public:
	Saver(Config* cfg, Database* db);
};

#endif
