#ifndef ICEMET_SERVER_WATCHER_H
#define ICEMET_SERVER_WATCHER_H

#include "server/worker.hpp"
#include "icemet/config.hpp"
#include "icemet/file.hpp"

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
