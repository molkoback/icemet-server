#include "reader.hpp"

#include "icemet/util/time.hpp"

Reader::Reader(Config* cfg, Database* db) :
	Worker(COLOR_BRIGHT_CYAN "READER" COLOR_RESET),
	m_cfg(cfg),
	m_db(db),
	m_id(0),
	m_img(NULL) {}

bool Reader::loop()
{
	std::vector<ParticleRow> rows;
	m_db->readParticles(rows, m_id);
	for (const auto& row : rows) {
		ImgPtr tmp = cv::makePtr<Image>(File(row.sensor, row.dt, row.frame, FILE_STATUS_NOTEMPTY).name());
		
		// Make sure we have a file
		if (m_img.empty()) {
			m_img = tmp;
		}
		// File complete
		else if (*tmp != *m_img) {
			m_log.debug("Read %s", m_img->name().c_str());
			m_outputs[0]->push(m_img);
			m_img = tmp;
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
		m_img->particles.push_back(par);
		m_id = row.id+1;
	}
	msleep(10);
	return m_cfg->args.waitNew || !rows.empty();
}
