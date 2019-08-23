#include "reader.hpp"

#include "icemet/util/time.hpp"

Reader::Reader(Config* cfg, Database* db) :
	Worker(COLOR_BRIGHT_CYAN "READER" COLOR_RESET),
	m_cfg(cfg),
	m_db(db),
	m_id(0),
	m_file(NULL) {}

bool Reader::init()
{
	m_filesAnalysis = static_cast<FileQueue*>(m_outputs[0]->data);
	return true;
}

bool Reader::loop()
{
	std::vector<ParticleRow> rows;
	m_db->readParticles(rows, m_id);
	for (const auto& row : rows) {
		FilePtr tmp = cv::makePtr<File>(row.sensor, row.dt, row.frame, false);
		
		// Make sure we have a file
		if (m_file.empty()) {
			m_file = tmp;
		}
		// File complete
		else if (*tmp != *m_file) {
			m_log.debug("Read %s", m_file->name().c_str());
			m_filesAnalysis->push(m_file);
			m_file = tmp;
		}
		
		// Create particle
		ParticlePtr par = cv::makePtr<Particle>();
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
	return m_cfg->args.waitNew || !rows.empty();
}
