#include "stats.hpp"

#include "icemet/core/math.hpp"
#include "icemet/util/strfmt.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <queue>

Stats::Stats(Config* cfg, Database* db) :
	Worker(COLOR_BLUE "STATS" COLOR_RESET),
	m_cfg(cfg),
	m_db(db)
{
	int wpx = m_cfg->img.size.width - 2*m_cfg->img.border.width;
	int hpx = m_cfg->img.size.height - 2*m_cfg->img.border.height;
	double z0 = m_cfg->particle.zMin;
	double z1 = m_cfg->particle.zMax;
	double psz = m_cfg->hologram.psz;
	double dist = m_cfg->hologram.dist;
	
	double h = z1 - z0;
	int Apx = wpx * hpx;
	double pszZ0 = psz / cv::icemet::Hologram::magnf(dist, z0);
	double pszZ1 = psz / cv::icemet::Hologram::magnf(dist, z1);
	double AZ0 = Apx * pszZ0*pszZ0;
	double AZ1 = Apx * pszZ1*pszZ1;
	m_V = Math::Vcone(h, AZ0, AZ1);
	m_log.debug("Measurement volume %.2f cm3", m_V * 1000000);
	
	m_len = m_cfg->stats.time * 1000;
	
	reset();
}

bool Stats::init()
{
	m_filesAnalysis = static_cast<FileQueue*>(m_inputs[0]->data);
	return true;
}

void Stats::reset(const DateTime& dt)
{
	m_particles = cv::Mat(0, 0, CV_64F);
	m_frames = 0;
	
	m_dt.setStamp(dt.stamp() / m_len * m_len);
}

void Stats::fillStatsRow(StatsRow& row) const
{
	// Use fixed frames in -s mode
	unsigned int frames = m_cfg->args.statsOnly ? m_cfg->stats.frames : m_frames;
	
	unsigned int particles = m_particles.rows;
	if (!particles) {
		row = {0, m_dt, 0.0, 0.0, 0.0, frames, 0};
		return;
	}
	
	double Vtot = m_V * frames; // Total measurement volume (m3)
	
	// Create histogram
	cv::Mat counts, bins;
	cv::icemet::hist(
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
	float lwc = cv::sum(lwcs)[0]; // Liquid water content (g/m3)
	
	// Median volume diameter
	cv::Mat pros = lwcs / lwc;
	cv::Mat cums = cv::Mat(pros.size(), CV_64FC1);
	double cumsum = 0.0;
	int idx = -1;
	for (int i = 0; i < pros.cols; i++) {
		cumsum += pros.at<double>(0, i);
		cums.at<double>(0, i) = cumsum;
		if (idx < 0 && cumsum > 0.5)
			idx = i;
	}
	float mvd;
	if (idx <= 0)
		mvd = D.at<double>(0, 0);
	else if (idx >= pros.cols-1)
		mvd = D.at<double>(0, pros.cols-1);
	else {
		double Di = D.at<double>(0, idx);
		double Di1 = D.at<double>(0, idx+1);
		double cumi = cums.at<double>(0, idx-1);
		double proi = pros.at<double>(0, idx);
		mvd = Di + (0.5-cumi) / proi * (Di1-Di);
	}
	
	// Concentration
	float conc = particles / Vtot;
	
	row = {0, m_dt, lwc, mvd, conc, frames, particles};
}

void Stats::statsPoint() const
{
	StatsRow row;
	fillStatsRow(row);
	m_db->writeStats(row);
	m_log.info(
		"[%02d:%02d:%02d] LWC %.2f g/m3, MVD %.2f um, Conc %.2f #/cm3",
		 m_dt.hour(), m_dt.min(), m_dt.sec(),
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

void Stats::process(const FilePtr& file)
{
	DateTime dt = file->dt();
	
	// Make sure we have datetime
	if (m_dt.stamp() == 0)
		reset(dt);
	
	// Create a new stats point if the minutes differ
	if (dt.stamp() - m_dt.stamp() >= m_len) {
		statsPoint();
		reset(dt);
	}
	
	// Get particle diameters
	int count = 0;
	for (const auto& par : file->particles) {
		if (particleValid(par)) {
			m_particles.push_back(par->diam);
			count++;
		}
	}
	m_frames++;
	m_log.debug("Valid particles: %d", count);
}

bool Stats::loop()
{
	// Collect files
	std::queue<FilePtr> files;
	m_filesAnalysis->collect(files);
	
	// Process
	while (!files.empty()) {
		FilePtr file = files.front();
		m_log.debug("Analysing %s", file->name().c_str());
		Measure m;
		
		// Skip files that haven't been preprocessed (the first few files)
		if (m_cfg->args.statsOnly || !file->preproc.empty())
			process(file);
		m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		files.pop();
	}
	
	if (m_inputs.empty()) {
		return false;
	}
	msleep(1);
	return !m_inputs[0]->closed() || !m_filesAnalysis->empty();
}

void Stats::close()
{
	if (m_frames)
		statsPoint();
}
