#include "config.hpp"

#include "util/strfmt.hpp"

#include <yaml-cpp/yaml.h>

#include <exception>
#include <stdexcept>

static Config* cfgDefault = NULL;

Config::Config(const char* fn)
{
	load(fn);
}

Config::Config(const Config& cfg) :
	m_log(cfg.m_log),
	m_paths(cfg.m_paths),
	m_types(cfg.m_types),
	m_connInfo(cfg.m_connInfo),
	m_dbInfo(cfg.m_dbInfo),
	m_img(cfg.m_img),
	m_bgsub(cfg.m_bgsub),
	m_lpf(cfg.m_lpf),
	m_hologram(cfg.m_hologram),
	m_segment(cfg.m_segment),
	m_particle(cfg.m_particle),
	m_ocl(cfg.m_ocl) {}

fs::path Config::strToPath(const std::string& str) const
{
	fs::path p(str);
	p.make_preferred();
	return p;
}

void Config::load(const char* fn)
{
	try {
		YAML::Node node = YAML::LoadFile(fn);
		
		m_log.level = static_cast<LogLevel>(node["log_level"].as<int>());
		
		m_connInfo.host = node["sql_host"].as<std::string>();
		m_connInfo.port = node["sql_port"].as<int>();
		m_connInfo.user = node["sql_user"].as<std::string>();
		m_connInfo.passwd = node["sql_passwd"].as<std::string>();
		
		m_dbInfo.name = node["sql_database"].as<std::string>();
		m_dbInfo.particleTable = node["sql_table_particles"].as<std::string>();
		m_dbInfo.statsTable = node["sql_table_stats"].as<std::string>();
		
		m_paths.watch = strToPath(node["path_watch"].as<std::string>());
		m_paths.results = strToPath(node["path_results"].as<std::string>()) / fs::path(m_dbInfo.name) / fs::path(m_dbInfo.particleTable);
		m_paths.original = m_paths.results / fs::path("original");
		m_paths.preproc = m_paths.results / fs::path("preproc");
		m_paths.recon = m_paths.results / fs::path("recon");
		m_paths.threshold = m_paths.results / fs::path("threshold");
		m_paths.preview = m_paths.results / fs::path("preview");
		
		m_types.results = strToPath(node["type_results"].as<std::string>());
		m_types.lossy = strToPath(node["type_results_lossy"].as<std::string>());
		
		m_img.rect.x = node["img_x"].as<int>();
		m_img.rect.y = node["img_y"].as<int>();
		m_img.rect.width = node["img_w"].as<int>();
		m_img.rect.height = node["img_h"].as<int>();
		m_img.size.width = m_img.rect.width;
		m_img.size.height = m_img.rect.height;
		m_img.border.width = node["img_ignore_x"].as<int>();
		m_img.border.height = node["img_ignore_y"].as<int>();
		
		m_bgsub.stackLen = node["bgsub_stack_len"].as<int>();
		
		m_lpf.enabled = node["filt_lowpass"].as<bool>();
		m_lpf.f = node["filt_lowpass_f"].as<float>();
		
		m_hologram.z0 = node["holo_z_start"].as<float>();
		m_hologram.z1 = node["holo_z_end"].as<float>();
		m_hologram.dz = node["holo_z_step"].as<float>();
		m_hologram.psz = node["holo_pixel_size"].as<float>();
		m_hologram.lambda = node["holo_lambda"].as<float>();
		m_hologram.dist = node["holo_collimated"].as<bool>() ? 0.0 : node["holo_distance"].as<float>();
		m_hologram.step = node["recon_step"].as<int>();
		m_hologram.focusPoints = node["focus_points"].as<int>();
		
		m_segment.thFact = node["segment_th_factor"].as<float>();
		m_segment.nMax = node["segment_n_max"].as<int>();
		m_segment.sizeMin = node["segment_size_min"].as<int>();
		m_segment.sizeSmall = node["segment_size_small"].as<int>();
		m_segment.pad = node["segment_pad"].as<int>();
		
		m_particle.zMin = node["particle_z_min"].as<float>();
		m_particle.zMax = node["particle_z_max"].as<float>();
		m_particle.thFact = node["particle_th_factor"].as<float>();
		m_particle.diamMin = node["particle_diam_min"].as<float>();
		m_particle.diamMax = node["particle_diam_max"].as<float>();
		m_particle.diamStep = node["particle_diam_step"].as<float>();
		m_particle.circMin = node["particle_circularity_min"].as<float>();
		m_particle.circMax = node["particle_circularity_max"].as<float>();
		m_particle.dynRangeMin = node["particle_dnr_min"].as<int>();
		m_particle.dynRangeMax = node["particle_dnr_max"].as<int>();
		
		m_diamCorr.enabled = node["diam_correction"].as<bool>();
		m_diamCorr.D0 = node["diam_correction_start"].as<float>();
		m_diamCorr.D1 = node["diam_correction_end"].as<float>();
		m_diamCorr.f0 = node["diam_correction_start_factor"].as<float>();
		m_diamCorr.f1 = node["diam_correction_end_factor"].as<float>();
		
		m_ocl.device = node["ocl_device"].as<std::string>();
	}
	catch (std::exception& e) {
		throw(std::runtime_error(strfmt("Couldn't parse config file '%s'", fn)));
	}
}

Config* Config::getDefaultPtr()
{
	return cfgDefault;
}

void Config::setDefaultPtr(Config* cfg)
{
	cfgDefault = cfg;
}
