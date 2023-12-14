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
	m_particles.clear();
	m_frames = 0;
	m_skipped = 0;
}

void Stats::fillStatsRow(StatsRow& row)
{
	row.dt = m_dtCurr;
	row.frames = (m_cfg->stats.frames > 0 ? m_cfg->stats.frames : m_frames) - m_skipped;
	row.particles = m_particles.size();
	if (!row.particles) {
		row.lwc = 0.f;
		row.mvd = 0.f;
		row.conc = 0.f;
		return;
	}
	
	// Total measurement volume
	double Vmeas = m_V * row.frames;
	
	// Create particle diameter and volume matrices
	std::sort(m_particles.begin(), m_particles.end());
	cv::Mat D(1, m_particles.size(), CV_64FC1, m_particles.data());
	cv::Mat V;
	cv::divide(D, 2.0, V);
	cv::pow(V, 3.0, V);
	cv::multiply(V, 4.0/3.0 * Math::pi, V);
	
	// Calculate LWC
	double Vtot = cv::sum(V)[0];
	row.lwc =  Vtot * 1.0e+6 / Vmeas;
	
	// Create cumulative volume matrice and find the half point
	double Vhalf = Vtot / 2.0;
	double Vsum = 0.0;
	cv::Mat Vcum(V.size(), CV_64FC1);
	int i = 0;
	for (; i < V.cols; i++) {
		Vsum += V.at<double>(0, i);
		Vcum.at<double>(0, i) = Vsum;
		if (Vsum > Vhalf)
			break;
	}
	
	// Calculate MVD
	if (i == 0)
		row.mvd = D.at<double>(0, 0);
	else {
		double V0 = V.at<double>(0, i-1);
		double V1 = V.at<double>(0, i);
		double Vcum0 = Vcum.at<double>(0, i-1);
		double Vmvd = V0 + (Vhalf-Vcum0) / V1 * (V1-V0);
		row.mvd =  2.0 * pow(Vmvd / (4.0/3.0 * Math::pi), 1.0/3.0);
	}
	
	// Concentration
	row.conc = row.particles / Vmeas;
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
