#include "watcher.hpp"

#include "icemet/img.hpp"
#include "icemet/pkg.hpp"
#include "icemet/util/time.hpp"

#include <algorithm>
#include <stdexcept>
#include <vector>

Watcher::Watcher(ICEMETServerContext* ctx) :
	Worker(COLOR_BRIGHT_CYAN "WATCHER" COLOR_RESET, ctx),
	m_prev(cv::makePtr<File>())
{
	m_log.info("Watching {}", m_cfg->paths.watch.string());
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
			m_log.debug("Ignoring: '{}'", path.string());
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
		m_log.debug("Invalid image file: '{}'", p.string());
		return false;
	}
	
	// Push to output queue
	m_log.debug("{}: Opened ({:.2f} s)", img->name(), m.time());
	m_outputs[0]->push(img);
	return true;
}

bool Watcher::processPkg(const fs::path& p)
{
	// Open the package
	Measure m1;
	PkgPtr pkg;
	try {
		pkg = createPackage(p);
	}
	catch(std::exception& e) {
		m_log.debug("Invalid package file: '{}'", p.string());
		return false;
	}
	m_log.debug("{}: Opened ({:.2f} s)", pkg->name(), m1.time());
	
	// Loop through images
	while (true) {
		Measure m2;
		ImgPtr img = pkg->next();
		if (img.empty())
			break;
		m_log.debug("{}: Read ({:.2f} s)", img->name(), m2.time());
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
	if (m_args->waitNew) {
		ssleep(1);
		return true;
	}
	m_outputs[0]->push(WORKER_MESSAGE_QUIT);
	return false;
}
