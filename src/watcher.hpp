#ifndef WATCHER_H
#define WATCHER_H

#include "worker.hpp"
#include "core/config.hpp"
#include "core/file.hpp"

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
