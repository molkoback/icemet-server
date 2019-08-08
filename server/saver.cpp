#include "saver.hpp"

#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

Saver::Saver(Config* cfg, Database* db) :
	Worker(COLOR_BRIGHT_BLUE "SAVER" COLOR_RESET),
	m_cfg(cfg),
	m_db(db)
{
	m_log.info("Results %s", m_cfg->paths().results.string().c_str());
}

bool Saver::init()
{
	m_filesAnalysis = static_cast<FileQueue*>(m_inputs[0]->data);
	return true;
}

void Saver::moveOriginal(const FilePtr& file) const
{
	fs::path src(file->path());
	fs::path dst = file->path(m_cfg->paths().original, src.extension());
	if (fs::exists(dst))
		fs::remove(dst);
	fs::rename(src, dst);
}

void Saver::processEmpty(const FilePtr& file) const
{
	// Preproc
	if (file->preproc.u) {
		fs::create_directories(file->dir(m_cfg->paths().preproc));
		fs::path dst = file->path(m_cfg->paths().preproc, m_cfg->types().results);
		cv::imwrite(dst.string(), file->preproc.getMat(cv::ACCESS_READ));
	}
	
	// Original
	fs::create_directories(file->dir(m_cfg->paths().original));
	moveOriginal(file);
}

void Saver::process(const FilePtr& file) const
{
	int n = file->particles.size();
	
	// Create preview
	cv::UMat preview = cv::UMat::zeros(m_cfg->img().size, CV_8UC1);
	for (const auto& segm : file->segments) {
		cv::UMat tmp;
		segm->img.copyTo(tmp);
		cv::UMat inv, invTh;
		cv::bitwise_not(tmp, inv);
		unsigned char th = cv::threshold(inv, invTh, 0, 255, cv::THRESH_OTSU);
		cv::UMat adj(inv.size(), CV_8UC1);
		cv::icemet::adjust(inv, adj, th, 255, 0, 255);
		adj.copyTo(cv::UMat(preview, segm->rect));
	}
	
	// Create directories
	fs::create_directories(file->dir(m_cfg->paths().original));
	fs::create_directories(file->dir(m_cfg->paths().preproc));
	fs::create_directories(file->dir(m_cfg->paths().recon));
	fs::create_directories(file->dir(m_cfg->paths().threshold));
	fs::create_directories(file->dir(m_cfg->paths().preview));
	
	// Save images
	fs::path dst;
	dst = file->path(m_cfg->paths().preproc, m_cfg->types().results);
	cv::imwrite(dst.string(), file->preproc.getMat(cv::ACCESS_READ));
	dst = file->path(m_cfg->paths().preview, m_cfg->types().lossy);
	cv::imwrite(dst.string(), preview.getMat(cv::ACCESS_READ));
	for (int i = 0; i < n; i++) {
		dst = file->path(m_cfg->paths().recon, m_cfg->types().results, i+1);
		cv::imwrite(dst.string(), file->segments[i]->img);
		
		dst = file->path(m_cfg->paths().threshold, m_cfg->types().results, i+1);
		cv::imwrite(dst.string(), file->particles[i]->img);
	}
	moveOriginal(file);
	
	// Write SQL
	for (int i = 0; i < n; i++) {
		const auto& segm = file->segments[i];
		const auto& par = file->particles[i];
		m_db->writeParticle({
			0, file->dt(),
			file->sensor(), file->frame(), (unsigned int)i+1,
			par->x, par->y, par->z,
			par->diam, par->diamCorr,
			par->circularity, par->dynRange, par->effPxSz,
			segm->rect
		});
	}
}

bool Saver::loop()
{
	// Collect files
	std::queue<FilePtr> files;
	m_filesAnalysis->collect(files);
	
	// Process
	while (!files.empty()) {
		FilePtr file = files.front();
		m_log.debug("Saving %s", file->name().c_str());
		Measure m;
		if (file->empty())
			processEmpty(file);
		else
			process(file);
		m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		m_log.info("Done %s", file->name().c_str());
		files.pop();
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_filesAnalysis->empty();
}
