#include "saver.hpp"

#include "util/time.hpp"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

Saver::Saver() :
	Worker(COLOR_BRIGHT_BLUE "SAVER" COLOR_RESET)
{
	m_log.info("Results %s", m_cfg->paths().results.string().c_str());
}

void Saver::moveOriginal(const cv::Ptr<File>& file) const
{
	fs::path src(file->path());
	fs::path dst = file->path(m_cfg->paths().original, src.extension());
	if (fs::exists(dst))
		fs::remove(dst);
	fs::rename(src, dst);
}

void Saver::processEmpty(const cv::Ptr<File>& file) const
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

void Saver::process(const cv::Ptr<File>& file) const
{
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
	for (size_t i = 0; i < file->segments.size(); i++) {
		dst = file->path(m_cfg->paths().recon, m_cfg->types().results, i+1);
		cv::imwrite(dst.string(), file->segments[i]->img);
	}
	for (size_t i = 0; i < file->particles.size(); i++) {
		dst = file->path(m_cfg->paths().threshold, m_cfg->types().results, i+1);
		cv::imwrite(dst.string(), file->particles[i]->img);
	}
	moveOriginal(file);
	
	// Write SQL
	for (unsigned int i = 0; i < file->particles.size(); i++) {
		cv::Ptr<Segment> segm = file->segments[i];
		cv::Ptr<Particle> par = file->particles[i];
		m_db->writeParticle({
			0, file->dt(),
			file->sensor(), file->frame(), i+1,
			par->x, par->y, par->z,
			par->diam, par->diamCorr,
			par->circularity, par->dynRange, par->effPxSz,
			segm->rect
		});
	}
}

bool Saver::cycle()
{
	// Collect files
	std::queue<cv::Ptr<File>> files;
	m_data->analysisSaver.collect(files);
	
	// Process
	while (!files.empty()) {
		cv::Ptr<File> file = files.front();
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
	return m_cfg->args().waitNew || !m_data->analysisSaver.done();
}

void Saver::start()
{
	Saver().run();
}
