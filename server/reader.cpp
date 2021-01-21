#include "reader.hpp"

Reader::Reader(Config* cfg, Database* db) :
	Worker(COLOR_BRIGHT_CYAN "READER" COLOR_RESET),
	m_cfg(cfg),
	m_db(db),
	m_img(NULL) {}

void Reader::push()
{
	m_log.debug("%s: Read", m_img->name().c_str());
	m_outputs[0]->push(m_img);
}

bool Reader::loop()
{
	DatabaseIterator iter;
	ParticleRow row;
	while (m_db->readParticles(iter, row)) {
		ImgPtr tmp = cv::makePtr<Image>(File(row.sensor, row.dt, row.frame, FILE_STATUS_NOTEMPTY).name());
		
		// Make sure we have a file
		if (m_img.empty()) {
			m_img = tmp;
		}
		// File complete
		else if (*tmp != *m_img) {
			push();
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
	}
	if (!m_img.empty())
		push();
	return false;
}
