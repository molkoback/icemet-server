#include "stats.hpp"

#include "core/math.hpp"
#include "util/measure.hpp"
#include "util/strfmt.hpp"

#include <opencv2/icemet.hpp>

#include <queue>

Stats::Stats(FileQueue* input) :
	Worker(COLOR_BLUE "STATS" COLOR_RESET),
	m_input(input)
{
	int wpx = m_cfg->img().size.width - 2*m_cfg->img().border.width;
	int hpx = m_cfg->img().size.height - 2*m_cfg->img().border.height;
	double z0 = m_cfg->hologram().z0;
	double z1 = m_cfg->hologram().z1;
	double psz = m_cfg->hologram().psz;
	double dist = m_cfg->hologram().dist;
	
	double h = m_cfg->hologram().z1 - m_cfg->hologram().z0;
	int Apx = wpx * hpx;
	double pszcam = psz / Math::Mz(dist, z1);
	double pszlsr = psz / Math::Mz(dist, z0);
	double Acam = Apx * pszcam*pszcam;
	double Alsr = Apx * pszlsr*pszlsr;
	m_V = Math::Vcone(h, Acam, Alsr);
	m_log.debug("Measurement volume %.2f cm3", m_V * 1000000);
	
	resetStats();
}

void Stats::resetStats(const DateTime& dt)
{
	m_particles = cv::Mat(0, 0, CV_64F);
	m_frames = 0;
	
	m_dt = dt;
	m_dt.S = 0;
	m_dt.MS = 0;
}

void Stats::createStats(StatsRow& row)
{
	unsigned int particles = m_particles.rows;
	if (!particles) {
		row = {0, m_dt, 0.0, 0.0, 0.0, m_frames, 0};
		return;
	}
	
	double Vtot = m_V * m_frames; // Total measurement volume (m3)
	
	// Create histogram
	cv::Mat counts, bins;
	cv::icemet::hist(
		m_particles.getUMat(cv::ACCESS_READ),
		counts, bins,
		m_cfg->particle().diamMin, m_cfg->particle().diamMax, m_cfg->particle().diamStep
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
	
	row = {0, m_dt, lwc, mvd, conc, m_frames, particles};
}

bool Stats::particleValid(const cv::Ptr<Particle>& par)
{
	return (
		par->z >= m_cfg->particle().zMin &&
		par->z <= m_cfg->particle().zMax &&
		par->diam >= m_cfg->particle().diamMin &&
		par->diam <= m_cfg->particle().diamMax &&
		par->circularity >= m_cfg->particle().circularityMin &&
		par->circularity <= m_cfg->particle().circularityMax &&
		par->dnr >= m_cfg->particle().dnrMin &&
		par->dnr <= m_cfg->particle().dnrMax
	);
}

void Stats::process(const cv::Ptr<File>& file)
{
	DateTime dt = file->dt();
	
	// Make sure we have datetime
	if (m_dt.m == 0)
		resetStats(dt);
	
	// Create a new stats point if the minutes differ
	if (dt.M != m_dt.M) {
		StatsRow row;
		createStats(row);
		m_db->writeStats(row);
		m_log.info(
			"[%02d:%02d] LWC %.2f g/m3, MVD %.2f μm, Conc %.2f #/cm3",
			 m_dt.H, m_dt.M,
			 row.lwc, row.mvd*1000000, row.conc/1000000
		);
		resetStats(dt);
	}
	
	// Get particle diameters
	int count = 0;
	for (const auto& par : file->particles) {
		if (particleValid(par)) {
			m_particles.push_back(par->diam);
			count++;
		}
	}
	m_log.debug("Valid particles: %d", count);
}

bool Stats::cycle()
{
	// Collect files
	std::queue<cv::Ptr<File>> files;
	m_input->collect(files);
	
	// Process
	while (!files.empty()) {
		cv::Ptr<File> file = files.front();
		if (!file->empty()) {
			m_log.debug("Analysing %s", file->name().c_str());
			Measure m;
			process(file);
			m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		}
		m_frames++;
		files.pop();
	}
	msleep(1);
	return true;
}

void Stats::start(FileQueue* input)
{
	Stats(input).run();
}
