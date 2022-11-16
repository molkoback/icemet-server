#include "saver.hpp"

#include "icemet/math.hpp"
#include "icemet/pkg.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <queue>

Saver::Saver(ICEMETServerContext* ctx) :
	Worker(COLOR_BRIGHT_BLUE "SAVER" COLOR_RESET, ctx)
{
	m_log.info("Results %s", m_cfg->paths.results.string().c_str());
}

void Saver::move(const fs::path& src, const fs::path& dst) const
{
	if (fs::exists(dst))
		fs::remove(dst);
	try {
		fs::rename(src, dst);
	}
	catch (std::exception& e) {
		fs::copy(src, dst);
		fs::remove(src);
	}
}

void Saver::processImg(const ImgPtr& img) const
{
	fs::path pathOrig(img->path());
	if ((img->status() == FILE_STATUS_EMPTY && !m_cfg->saves.empty) ||
	    (img->status() == FILE_STATUS_SKIP && !m_cfg->saves.skipped) ||
	    (img->status() == FILE_STATUS_NONE)) {
		if (!pathOrig.empty())
			fs::remove(pathOrig);
		return;
	}
	
	// Move or remove original images
	if (!pathOrig.empty()) {
		if (m_cfg->saves.original) {
			fs::create_directories(img->dir(m_cfg->paths.original));
			move(pathOrig, img->path(m_cfg->paths.original, pathOrig.extension()));
		}
		else {
			fs::remove(pathOrig);
		}
	}
	
	// Save other images
	int n = img->particles.size();
	if (m_cfg->saves.preproc && !img->preproc.empty()) {
		fs::create_directories(img->dir(m_cfg->paths.preproc));
		fs::path dst(img->path(m_cfg->paths.preproc, m_cfg->types.results));
		cv::imwrite(dst.string(), img->preproc.getMat(cv::ACCESS_READ));
	}
	if (m_cfg->saves.min && !img->min.empty()) {
		fs::create_directories(img->dir(m_cfg->paths.min));
		fs::path dst(img->path(m_cfg->paths.min, m_cfg->types.results));
		cv::imwrite(dst.string(), img->min.getMat(cv::ACCESS_READ));
	}
	if (m_cfg->saves.recon && img->status() == FILE_STATUS_NOTEMPTY) {
		fs::create_directories(img->dir(m_cfg->paths.recon));
		for (int i = 0; i < n; i++) {
			fs::path dst(img->path(m_cfg->paths.recon, m_cfg->types.results, i+1));
			cv::imwrite(dst.string(), img->segments[i]->img);
		}
	}
	if (m_cfg->saves.threshold && img->status() == FILE_STATUS_NOTEMPTY) {
		fs::create_directories(img->dir(m_cfg->paths.threshold));
		for (int i = 0; i < n; i++) {
			fs::path dst(img->path(m_cfg->paths.threshold, m_cfg->types.results, i+1));
			cv::imwrite(dst.string(), img->particles[i]->img);
		}
	}
	if (m_cfg->saves.preview && img->status() == FILE_STATUS_NOTEMPTY) {
		fs::create_directories(img->dir(m_cfg->paths.preview));
		
		cv::Mat preview = cv::Mat::zeros(m_cfg->img.size, CV_8UC1);
		for (const auto& segm : img->segments) {
			// Invert
			cv::Mat imgInv;
			cv::bitwise_not(segm->img, imgInv);
			
			// Adjust
			cv::Mat imgTh, imgAdj;
			unsigned char th = cv::threshold(imgInv, imgTh, 0, 255, cv::THRESH_OTSU);
			Math::adjust(imgInv, imgAdj, th, 255, 0, 255);
			
			// Draw
			imgAdj.copyTo(cv::Mat(preview, segm->rect));
		}
		fs::path dst(img->path(m_cfg->paths.preview, m_cfg->types.lossy));
		cv::imwrite(dst.string(), preview);
	}
	
	// Write SQL
	for (int i = 0; i < n; i++) {
		const auto& segm = img->segments[i];
		const auto& par = img->particles[i];
		m_db->writeParticle({
			0, img->dt(),
			img->sensor(), img->frame(), (unsigned int)i+1,
			par->x, par->y, par->z,
			par->diam, par->diamCorr,
			par->circularity, par->dynRange, par->effPxSz,
			segm->rect
		});
	}
}

void Saver::processPkg(const PkgPtr& pkg) const
{
	fs::path pathOrig(pkg->path());
	if (m_cfg->saves.original) {
		fs::create_directories(pkg->dir(m_cfg->paths.original));
		move(pathOrig, pkg->path(m_cfg->paths.original, pathOrig.extension()));
	}
	else {
		fs::remove(pathOrig);
	}
}

bool Saver::loop()
{
	std::queue<WorkerData> queue;
	m_inputs[0]->collect(queue);
	if (queue.empty())
		msleep(1);
	
	bool quit = false;
	while (!queue.empty()) {
		WorkerData data(queue.front());
		queue.pop();
		switch (data.type()) {
			case WORKER_DATA_IMG: {
				ImgPtr img = data.get<ImgPtr>();
				m_log.debug("%s: Saving", img->name().c_str());
				Measure m;
				processImg(img);
				m_log.debug("%s: Done (%.2f s)", img->name().c_str(), m.time());
				m_log.info("%s", img->name().c_str());
				break;
			}
			case WORKER_DATA_PKG:
				processPkg(data.get<PkgPtr>());
				break;
			case WORKER_DATA_MSG:
				if (data.get<WorkerMessage>() == WORKER_MESSAGE_QUIT)
					quit = true;
				break;
		}
	}
	return !quit;
}
