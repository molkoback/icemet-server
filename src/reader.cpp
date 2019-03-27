#include "reader.hpp"

#include "util/sleep.hpp"

Reader::Reader() :
	Worker(COLOR_BRIGHT_CYAN "READER" COLOR_RESET),
	m_id(0),
	m_file(NULL) {}

bool Reader::cycle()
{
	std::vector<ParticleRow> rows;
	m_db->readParticles(rows, m_id);
	if (!m_cfg->args().waitNew && rows.empty()) {
		m_data->analysisStats.close();
		return false;
	}
	for (const auto& row : rows) {
		File tmp = File(row.sensor, row.dt, row.frame, false);
		
		// Make sure we have a file
		if (m_file.empty()) {
			m_file = cv::makePtr<File>(tmp);
		}
		// File complete
		else if (tmp != *m_file) {
			m_log.debug("Read %s", m_file->name().c_str());
			m_data->analysisStats.pushWait(m_file);
			m_file = cv::makePtr<File>(tmp);
		}
		
		// Create particle
		cv::Ptr<Particle> par = cv::makePtr<Particle>();
		par->effPxSz = row.effPxSz;
		par->x = row.x;
		par->y = row.y;
		par->z = row.z;
		par->diam = row.diam;
		par->diamCorr = row.diamCorr;
		par->circularity = row.circularity;
		par->dynRange = row.dynRange;
		m_file->particles.push_back(par);
		m_id = row.id+1;
	}
	msleep(10);
	return true;
}

void Reader::start()
{
	Reader().run();
}
