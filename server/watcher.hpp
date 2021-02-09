#ifndef ICEMET_SERVER_WATCHER_H
#define ICEMET_SERVER_WATCHER_H

#include "icemet/icemet.hpp"
#include "icemet/file.hpp"
#include "server/worker.hpp"

#include <queue>

class Watcher : public Worker {
protected:
	FilePtr m_prev;
	
	bool processImg(const fs::path& p);
	bool processPkg(const fs::path& p);
	void findFiles(std::queue<FilePtr>& queue);
	bool loop() override;

public:
	Watcher(ICEMETServerContext* ctx);
};

#endif
