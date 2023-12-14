#include "stats.hpp"

#include "icemet/hologram.hpp"
#include "icemet/math.hpp"
#include "icemet/util/strfmt.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>

#include <queue>
#include <stdexcept>

Stats::Stats(ICEMETServerContext* ctx) :
	Worker(COLOR_BLUE "STATS" COLOR_RESET, ctx)
{
	int wpx = m_cfg->img.size.width - 2*m_cfg->img.border.width;
	int hpx = m_cfg->img.size.height - 2*m_cfg->img.border.height;
	double z0 = m_cfg->particle.zMin;
	double z1 = m_cfg->particle.zMax;
	double psz = m_cfg->hologram.psz;
	double dist = m_cfg->hologram.dist;
	
	double h = z1 - z0;
	int Apx = wpx * hpx;
	double pszZ0 = psz / Hologram::magnf(dist, z0);
	double pszZ1 = psz / Hologram::magnf(dist, z1);
	double AZ0 = Apx * pszZ0*pszZ0;
	double AZ1 = Apx * pszZ1*pszZ1;
	m_V = Math::Vcone(h, AZ0, AZ1);
	m_log.debug("Measurement volume %.2f cm3", m_V * 1000000);
	
	m_len = m_cfg->stats.time * 1000;
	
	reset();
}

bool Stats::init()
{
	return true;
}

void Stats::reset()
{
	m_particles = cv::Mat(0, 0, CV_64F);
	m_frames = 0;
	m_skipped = 0;
}

void Stats::fillStatsRow(StatsRow& row) const
{
	row.dt = m_dtCurr;
	row.frames = (m_cfg->stats.frames > 0 ? m_cfg->stats.frames : m_frames) - m_skipped;
	row.particles = m_particles.rows;
	if (!row.particles) {
		row.lwc = 0.f;
		row.mvd = 0.f;
		row.conc = 0.f;
		return;
	}
	
	double Vtot = m_V * row.frames; // Total measurement volume (m3)
	
	// Create histogram
	cv::Mat counts, bins;
	Math::hist(
		m_particles.getUMat(cv::ACCESS_READ),
		counts, bins,
		m_cfg->particle.diamMin, m_cfg->particle.diamMax, m_cfg->particle.diamStep
	);
	cv::Mat N, D;
	counts.convertTo(N, CV_64FC1);
	bins.convertTo(D, CV_64FC1);
	
	// Calculate particle water masses
	cv::Mat r3;
	cv::pow(D/2.0, 3, r3);
	cv::Mat m = N.mul(r3) * 4.0/3.0*Math::pi * 1000000; // Water masses (g)
	
	// Liquid water content
	cv::Mat lwcs = m / Vtot; // Liquid water contents (g/m3)
	row.lwc = cv::sum(lwcs)[0]; // Liquid water content (g/m3)
	
	// Median volume diameter
	cv::Mat pros = lwcs / row.lwc;
	cv::Mat cums = cv::Mat(pros.size(), CV_64FC1);
	double cumsum = 0.0;
	int idx = -1;
	for (int i = 0; i < pros.cols; i++) {
		cumsum += pros.at<double>(0, i);
		cums.at<double>(0, i) = cumsum;
		if (idx < 0 && cumsum > 0.5)
			idx = i;
	}
	if (idx <= 0)
		row.mvd = D.at<double>(0, 0);
	else if (idx >= pros.cols-1)
		row.mvd = D.at<double>(0, pros.cols-1);
	else {
		double Di = D.at<double>(0, idx);
		double Di1 = D.at<double>(0, idx+1);
		double cumi = cums.at<double>(0, idx-1);
		double proi = pros.at<double>(0, idx);
		row.mvd = Di + (0.5-cumi) / proi * (Di1-Di);
	}
	
	// Concentration
	row.conc = row.particles / Vtot;
}

void Stats::statsPoint()
{
	StatsRow row;
	fillStatsRow(row);
	m_db->writeStats(row);
	m_log.info(
		"[%d-%02d-%02d %02d:%02d:%02d] LWC %.2f g/m3, MVD %.2f um, Conc %.2f #/cm3",
		row.dt.year(), row.dt.month(), row.dt.day(),
		row.dt.hour(), row.dt.min(), row.dt.sec(),
		row.lwc, row.mvd*1000000, row.conc/1000000
	);
}

bool Stats::particleValid(const ParticlePtr& par) const
{
	return (
		par->z >= m_cfg->particle.zMin &&
		par->z <= m_cfg->particle.zMax &&
		par->diam >= m_cfg->particle.diamMin &&
		par->diam <= m_cfg->particle.diamMax &&
		par->circularity >= m_cfg->particle.circMin &&
		par->circularity <= m_cfg->particle.circMax &&
		par->dynRange >= m_cfg->particle.dynRangeMin &&
		par->dynRange <= m_cfg->particle.dynRangeMax
	);
}

void Stats::process(const ImgPtr& img)
{
	DateTime dt = img->dt();
	
	// Make sure we have datetime
	if (m_dtCurr.stamp() == 0) {
		m_dtCurr.setStamp(dt.stamp() / m_len * m_len);
	}
	// Create a new stats point and update datetime
	else if (dt.stamp() - m_dtCurr.stamp() >= m_len) {
		if (m_dtCurr != m_dtPrev) {
			statsPoint();
			m_dtPrev = m_dtCurr;
			reset();
		}
		m_dtCurr.setStamp(dt.stamp() / m_len * m_len);
	}
	
	// Get particle diameters
	int count = 0;
	for (const auto& par : img->particles) {
		if (particleValid(par)) {
			m_particles.push_back(par->diam);
			count++;
		}
	}
	m_frames++;
	if (img->status() == FILE_STATUS_SKIP)
		m_skipped++;
	m_log.debug("%s: Valid particles: %d", img->name().c_str(), count);
}

bool Stats::loop()
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
				m_log.debug("%s: Processing", img->name().c_str());
				Measure m;
				process(img);
				m_log.debug("%s: Done (%.2f s)", img->name().c_str(), m.time());
				break;
			}
			case WORKER_DATA_PKG:
				if (m_dtCurr != m_dtPrev) {
					statsPoint();
					m_dtPrev = m_dtCurr;
					reset();
				}
				break;
			case WORKER_DATA_MSG:
				if (data.get<WorkerMessage>() == WORKER_MESSAGE_QUIT)
					quit = true;
				break;
		}
	}
	return !quit;
}

void Stats::close()
{
	if (m_dtCurr != m_dtPrev)
		statsPoint();
}
