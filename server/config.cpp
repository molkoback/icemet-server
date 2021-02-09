#include "config.hpp"

#include "icemet/util/strfmt.hpp"

#include <yaml-cpp/yaml.h>

#include <exception>
#include <stdexcept>

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

void Config::load(const fs::path& fn)
{
	try {
		YAML::Node node = YAML::LoadFile(fn.string());
		
		connInfo.host = node["sql_host"].as<std::string>();
		connInfo.port = node["sql_port"].as<int>();
		connInfo.user = node["sql_user"].as<std::string>();
		connInfo.passwd = node["sql_passwd"].as<std::string>();
		
		dbInfo.name = node["sql_database"].as<std::string>();
		dbInfo.particleTable = node["sql_table_particles"].as<std::string>();
		dbInfo.statsTable = node["sql_table_stats"].as<std::string>();
		
		paths.watch = strToPath(node["path_watch"].as<std::string>());
		paths.results = strToPath(node["path_results"].as<std::string>()) / fs::path(dbInfo.name) / fs::path(dbInfo.particleTable);
		paths.original = paths.results / fs::path("original");
		paths.preproc = paths.results / fs::path("preproc");
		paths.recon = paths.results / fs::path("recon");
		paths.threshold = paths.results / fs::path("threshold");
		paths.preview = paths.results / fs::path("preview");
		
		std::string savesStr(node["save_results"].as<std::string>());
		saves.original = savesStr.find('o') != std::string::npos;
		saves.preproc = savesStr.find('p') != std::string::npos;
		saves.recon = savesStr.find('r') != std::string::npos;
		saves.threshold = savesStr.find('t') != std::string::npos;
		saves.preview = savesStr.find('v') != std::string::npos;
		saves.empty = node["save_empty"].as<bool>();
		saves.skipped = node["save_skipped"].as<bool>();
		
		types.results = strToPath(node["type_results"].as<std::string>());
		types.lossy = strToPath(node["type_results_lossy"].as<std::string>());
		
		img.rect.x = node["img_x"].as<int>();
		img.rect.y = node["img_y"].as<int>();
		img.rect.width = node["img_w"].as<int>();
		img.rect.height = node["img_h"].as<int>();
		img.size.width = img.rect.width;
		img.size.height = img.rect.height;
		img.border.width = node["img_ignore_x"].as<int>();
		img.border.height = node["img_ignore_y"].as<int>();
		img.rotation = node["img_rotation"].as<float>();
		
		bgsub.stackLen = node["bgsub_stack_len"].as<int>();
		
		emptyCheck.originalTh = node["empty_th_original"].as<int>();
		emptyCheck.preprocTh = node["empty_th_preproc"].as<int>();
		emptyCheck.reconTh = node["empty_th_recon"].as<int>();
		
		noisyCheck.reconTh = node["noisy_th_recon"].as<int>();
		
		lpf.f = node["filt_lowpass"].as<float>();
		
		hologram.z.start = node["holo_z0"].as<float>();
		hologram.z.stop = node["holo_z1"].as<float>();
		hologram.z.step = node["holo_dz"].as<float>();
		hologram.psz = node["holo_pixel_size"].as<float>();
		hologram.lambda = node["holo_lambda"].as<float>();
		hologram.dist = node["holo_distance"].as<float>();
		hologram.step = node["recon_step"].as<int>();
		hologram.focusK = node["focus_k"].as<float>();
		
		segment.thFact = node["segment_th_factor"].as<float>();
		segment.sizeMin = node["segment_size_min"].as<int>();
		segment.sizeMax = node["segment_size_max"].as<int>();
		segment.sizeSmall = node["segment_size_small"].as<int>();
		segment.pad = node["segment_pad"].as<int>();
		segment.scale = node["segment_scale"].as<float>();
		
		particle.thFact = node["particle_th_factor"].as<float>();
		particle.zMin = node["particle_z_min"].as<float>();
		particle.zMax = node["particle_z_max"].as<float>();
		particle.diamMin = node["particle_diam_min"].as<float>();
		particle.diamMax = node["particle_diam_max"].as<float>();
		particle.diamStep = node["particle_diam_step"].as<float>();
		particle.circMin = node["particle_circ_min"].as<float>();
		particle.circMax = node["particle_circ_max"].as<float>();
		particle.dynRangeMin = node["particle_dynrange_min"].as<int>();
		particle.dynRangeMax = node["particle_dynrange_max"].as<int>();
		
		diamCorr.enabled = node["diam_corr"].as<bool>();
		diamCorr.D0 = node["diam_corr_d0"].as<float>();
		diamCorr.D1 = node["diam_corr_d1"].as<float>();
		diamCorr.f0 = node["diam_corr_f0"].as<float>();
		diamCorr.f1 = node["diam_corr_f1"].as<float>();
		
		stats.time = node["stats_time"].as<int>();
		stats.frames = node["stats_frames"].as<int>();
		
		ocl.device = node["ocl_device"].as<std::string>();
	}
	catch (std::exception& e) {
		throw(std::runtime_error(strfmt("Couldn't parse config file: ") + e.what()));
	}
}
