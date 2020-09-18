#include "watcher.hpp"

#include "icemet/img.hpp"
#include "icemet/pkg.hpp"
#include "icemet/util/time.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

Watcher::Watcher(Config* cfg) :
	Worker(COLOR_BRIGHT_CYAN "WATCHER" COLOR_RESET),
	m_cfg(cfg),
	m_prev(cv::makePtr<File>())
{
	m_log.info("Watching %s", m_cfg->paths.watch.string().c_str());
}

void Watcher::findFiles(std::queue<FilePtr>& queue)
{
	// List files
	std::vector<FilePtr> files;
	auto iter = fs::recursive_directory_iterator(m_cfg->paths.watch);
	for (const auto& entry : iter) {
		if (!entry.is_regular_file())
			continue;
		
		// Check if the path is an ICEMET file
		fs::path path(entry.path());
		try {
			files.push_back(cv::makePtr<File>(path));
		}
		catch (std::exception& e) {
			m_log.debug("Ignoring: '%s'", path.string().c_str());
		}
	}
	
	// Sort
	std::sort(files.begin(), files.end(), [](const auto& f1, const auto& f2) {
		return *f1 < *f2;
	});
	
	// Push to queue
	for (const auto& file : files)
		queue.push(file);
}

bool Watcher::processImg(const fs::path& p)
{
	// Open the image
	Measure m;
	ImgPtr img;
	try {
		img = cv::makePtr<Image>(p);
	}
	catch(std::exception& e) {
		m_log.debug("Invalid image file: '%s'", p.c_str());
		return false;
	}
	
	// Push to output queue
	img->setStatus(FILE_STATUS_NONE);
	m_log.debug("Opened %s (%.2f s)", img->name().c_str(), m.time());
	m_outputs[0]->push(img);
	return true;
}

bool Watcher::processPkg(const fs::path& p)
{
	Measure m1;
	PkgPtr pkg;
	try {
		pkg = createPackage(p);
	}
	catch(std::exception& e) {
		m_log.debug("Invalid package file: '%s'", p.c_str());
		return false;
	}
	m_log.debug("Opened %s (%.2f s)", pkg->name().c_str(), m1.time());
	
	while (true) {
		Measure m2;
		ImgPtr img = pkg->next();
		if (img.empty())
			break;
		img->setStatus(FILE_STATUS_NONE);
		m_log.debug("Read %s (%.2f s)", img->name().c_str(), m2.time());
		m_outputs[0]->push(img);
	}
	m_outputs[0]->push(pkg);
	return true;
}

bool Watcher::loop()
{
	std::queue<FilePtr> queue;
	findFiles(queue);
	while (!queue.empty()) {
		FilePtr file = queue.front();
		queue.pop();
		if (*file > *m_prev) {
			fs::path p = file->path();
			if (isPackage(p)) {
				if (processPkg(p))
					m_prev = file;
			}
			else {
				if (processImg(p))
					m_prev = file;
			}
		}
	}
	
	if (!m_cfg->args.waitNew)
		return false;
	ssleep(1);
	return !m_outputs.empty();
}
