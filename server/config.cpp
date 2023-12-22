#include "config.hpp"

#include "icemet/util/strfmt.hpp"

#include <yaml-cpp/yaml.h>

#include <exception>
#include <fstream>
#include <stdexcept>
#include <streambuf>

Config::Config(const fs::path& fn)
{
	load(fn);
}

Config::Config(const Config& cfg) :
	paths(cfg.paths),
	saves(cfg.saves),
	types(cfg.types),
	connInfo(cfg.connInfo),
	dbInfo(cfg.dbInfo),
	img(cfg.img),
	bgsub(cfg.bgsub),
	emptyCheck(cfg.emptyCheck),
	noisyCheck(cfg.noisyCheck),
	lpf(cfg.lpf),
	hologram(cfg.hologram),
	segment(cfg.segment),
	particle(cfg.particle),
	diamCorr(cfg.diamCorr),
	ocl(cfg.ocl) {}

fs::path Config::strToPath(const std::string& str) const
{
	fs::path p(str);
	p.make_preferred();
	return p;
}

YAML::Node getYAMLNode(const YAML::Node& node, const std::string& key)
{
	YAML::Node ret = node[key];
	if (!ret.IsDefined())
		throw(std::runtime_error(strfmt("Config parameter not found: '{}'", key)));
	return ret;
}

void Config::load(const fs::path& fn)
{
	std::ifstream stream(fn);
	if (!stream.is_open())
		throw(std::runtime_error(strfmt("Could not open config file: '{}'", fn.string())));
	
	m_str = std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
	YAML::Node node = YAML::Load(m_str);
	
	try {
		connInfo.host = getYAMLNode(node, "sql_host").as<std::string>();
		connInfo.port = getYAMLNode(node, "sql_port").as<int>();
		connInfo.user = getYAMLNode(node, "sql_user").as<std::string>();
		connInfo.passwd = getYAMLNode(node, "sql_passwd").as<std::string>();
		
		dbInfo.name = getYAMLNode(node, "sql_database").as<std::string>();
		dbInfo.particlesTable = getYAMLNode(node, "sql_table_particles").as<std::string>();
		dbInfo.statsTable = getYAMLNode(node, "sql_table_stats").as<std::string>();
		dbInfo.metaTable = getYAMLNode(node, "sql_table_meta").as<std::string>();
		
		paths.watch = strToPath(getYAMLNode(node, "path_watch").as<std::string>());
		paths.results = strToPath(getYAMLNode(node, "path_results").as<std::string>()) / fs::path(dbInfo.name) / fs::path(dbInfo.particlesTable);
		paths.original = paths.results / fs::path("original");
		paths.preproc = paths.results / fs::path("preproc");
		paths.min = paths.results / fs::path("min");
		paths.recon = paths.results / fs::path("recon");
		paths.threshold = paths.results / fs::path("threshold");
		paths.preview = paths.results / fs::path("preview");
		
		std::string savesStr(getYAMLNode(node, "save_results").as<std::string>());
		saves.original = savesStr.find('o') != std::string::npos;
		saves.preproc = savesStr.find('p') != std::string::npos;
		saves.min = savesStr.find('m') != std::string::npos;
		saves.recon = savesStr.find('r') != std::string::npos;
		saves.threshold = savesStr.find('t') != std::string::npos;
		saves.preview = savesStr.find('v') != std::string::npos;
		saves.empty = getYAMLNode(node, "save_empty").as<bool>();
		saves.skipped = getYAMLNode(node, "save_skipped").as<bool>();
		
		types.results = strToPath(getYAMLNode(node, "type_results").as<std::string>());
		types.lossy = strToPath(getYAMLNode(node, "type_results_lossy").as<std::string>());
		
		img.rect.x = getYAMLNode(node, "img_x").as<int>();
		img.rect.y = getYAMLNode(node, "img_y").as<int>();
		img.rect.width = getYAMLNode(node, "img_w").as<int>();
		img.rect.height = getYAMLNode(node, "img_h").as<int>();
		img.size.width = img.rect.width;
		img.size.height = img.rect.height;
		img.border.width = getYAMLNode(node, "img_ignore_x").as<int>();
		img.border.height = getYAMLNode(node, "img_ignore_y").as<int>();
		img.rotation = getYAMLNode(node, "img_rotation").as<float>();
		
		bgsub.stackLen = getYAMLNode(node, "bgsub_stack_len").as<int>();
		
		emptyCheck.originalTh = getYAMLNode(node, "empty_th_original").as<int>();
		emptyCheck.preprocTh = getYAMLNode(node, "empty_th_preproc").as<int>();
		emptyCheck.reconTh = getYAMLNode(node, "empty_th_recon").as<int>();
		
		noisyCheck.reconTh = getYAMLNode(node, "noisy_th_recon").as<int>();
		
		lpf.f = getYAMLNode(node, "filt_lowpass").as<float>();
		
		hologram.z0 = getYAMLNode(node, "holo_z0").as<float>();
		hologram.z1 = getYAMLNode(node, "holo_z1").as<float>();
		hologram.dz0 = getYAMLNode(node, "holo_dz0").as<float>();
		hologram.dz1 = getYAMLNode(node, "holo_dz1").as<float>();
		hologram.psz = getYAMLNode(node, "holo_pixel_size").as<float>();
		hologram.lambda = getYAMLNode(node, "holo_lambda").as<float>();
		hologram.dist = getYAMLNode(node, "holo_distance").as<float>();
		hologram.reconStep = getYAMLNode(node, "recon_step").as<int>();
		hologram.focusStep = getYAMLNode(node, "focus_step").as<double>();
		hologram.focusMethod = static_cast<FocusMethod>(getYAMLNode(node, "focus_method").as<int>());
		hologram.focusMethodSmall = static_cast<FocusMethod>(getYAMLNode(node, "focus_method_small").as<int>());
		
		segment.thFact = getYAMLNode(node, "segment_th_factor").as<float>();
		segment.sizeMin = getYAMLNode(node, "segment_size_min").as<int>();
		segment.sizeMax = getYAMLNode(node, "segment_size_max").as<int>();
		segment.sizeSmall = getYAMLNode(node, "segment_size_small").as<int>();
		segment.pad = getYAMLNode(node, "segment_pad").as<int>();
		segment.scale = getYAMLNode(node, "segment_scale").as<float>();
		
		particle.thFact = getYAMLNode(node, "particle_th_factor").as<float>();
		particle.zMin = getYAMLNode(node, "particle_z_min").as<float>();
		particle.zMax = getYAMLNode(node, "particle_z_max").as<float>();
		particle.diamMin = getYAMLNode(node, "particle_diam_min").as<float>();
		particle.diamMax = getYAMLNode(node, "particle_diam_max").as<float>();
		particle.circMin = getYAMLNode(node, "particle_circ_min").as<float>();
		particle.circMax = getYAMLNode(node, "particle_circ_max").as<float>();
		particle.dynRangeMin = getYAMLNode(node, "particle_dynrange_min").as<int>();
		particle.dynRangeMax = getYAMLNode(node, "particle_dynrange_max").as<int>();
		
		diamCorr.enabled = getYAMLNode(node, "diam_corr").as<bool>();
		diamCorr.D0 = getYAMLNode(node, "diam_corr_d0").as<float>();
		diamCorr.D1 = getYAMLNode(node, "diam_corr_d1").as<float>();
		diamCorr.f0 = getYAMLNode(node, "diam_corr_f0").as<float>();
		diamCorr.f1 = getYAMLNode(node, "diam_corr_f1").as<float>();
		
		stats.time = getYAMLNode(node, "stats_time").as<int>();
		stats.frames = getYAMLNode(node, "stats_frames").as<int>();
		stats.temp = getYAMLNode(node, "stats_temp").IsNull() ? NAN_FLOAT : node["stats_temp"].as<float>();
		stats.wind = getYAMLNode(node, "stats_wind").IsNull() ? NAN_FLOAT : node["stats_wind"].as<float>();
		
		ocl.device = getYAMLNode(node, "ocl_device").as<std::string>();
	}
	catch (YAML::Exception& e) {
		throw(std::runtime_error(strfmt("Invalid config value at line {}", e.mark.line+1)));
	}
}
