#ifndef ICEMET_SERVER_WATCHER_H
#define ICEMET_SERVER_WATCHER_H

#include "icemet/worker.hpp"
#include "icemet/core/config.hpp"
#include "icemet/core/file.hpp"

#include <queue>

class Watcher : public Worker {
protected:
	Config* m_cfg;
	FileQueue* m_filesOriginal;
	FilePtr m_prev;
	
	void findFiles(std::queue<FilePtr>& files);
	bool init() override;
	bool loop() override;

public:
	Watcher(Config* cfg);
};

#endif
