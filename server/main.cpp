#include "icemet/database.hpp"
#include "icemet/util/log.hpp"
#include "icemet/util/strfmt.hpp"
#include "icemet/util/time.hpp"
#include "analysis.hpp"
#include "preproc.hpp"
#include "reader.hpp"
#include "recon.hpp"
#include "saver.hpp"
#include "server.hpp"
#include "stats.hpp"
#include "watcher.hpp"

#include <opencv2/core/ocl.hpp>

#include <cstdlib>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static const char* usageStr = "Usage: icemet-server [options] config.yaml\n";
static const char* helpStr =
"Options:\n"
"  -h                Print this help message and exit.\n"
"  -V                Print version info and exit.\n"
"  -t                Test config file and exit.\n"
"  -p                Particles only.\n"
"  -s                Stats only. Particles will be fetched from the database.\n"
"  -Q                Quit after processing all available files.\n"
"  -d                Enable debug messages.\n";
static const char* versionFmt =
"ICEMET Server {}\n"
"\n"
"Copyright (C) 2019-2024 Eero Molkoselkä <eero.molkoselka@gmail.com>\n";

template <typename... Args>
static void print(const std::string& fmt, Args... args)
{
	std::cout << strfmt(fmt, args...);
}

static int cvErrorHandler(int status, const char* func, const char* msg, const char* fn, int line, void* data)
{
	(void)status;
	(void)func;
	(void)msg;
	(void)fn;
	(void)line;
	(void)data;
	return 0;
}

int main(int argc, char* argv[])
{
	Arguments args;
	args.root = fs::path(argv[0]).parent_path();
	for (int i = 1; i < argc; i++) {
		std::string arg(argv[i]);
		if (arg[0] == '-') {
			if (!arg.compare("-h")) {
				print("{}\n{}", usageStr, helpStr);
				return EXIT_SUCCESS;
			}
			else if (!arg.compare("-V")) {
				print(versionFmt, icemetServerVersion().str());
				return EXIT_SUCCESS;
			}
			else if (!arg.compare("-t")) {
				args.testConfig = true;
			}
			else if (!arg.compare("-p")) {
				args.particlesOnly = true;
			}
			else if (!arg.compare("-s")) {
				args.statsOnly = true;
			}
			else if (!arg.compare("-Q")) {
				args.waitNew = false;
			}
			else if (!arg.compare("-d")) {
				args.loglevel = LOG_DEBUG;
			}
			else {
				print("Invalid option '{}'\n", arg);
				return EXIT_FAILURE;
			}
		}
		else {
			args.cfgFile = arg;
		}
	}
	if (args.cfgFile.empty()) {
		print(usageStr);
		return EXIT_FAILURE;
	}
	
	Log log("MAIN");
	log.info("ICEMET Server {}", icemetServerVersion().str());
	try {
		// Suppress OpenCV errors
		cv::redirectError(cvErrorHandler);
		
		// Read config
		Config cfg(args.cfgFile);
		if (args.testConfig) {
			log.info("Config file OK");
			return EXIT_SUCCESS;
		}
		
		// Setup logging
		Log::setLevel(args.loglevel);
		
		// Initialize OpenCL
		std::string str = strfmt("OPENCV_OPENCL_DEVICE={}", cfg.ocl.device);
		if (putenv(&str[0]) || !cv::ocl::useOpenCL())
			throw std::runtime_error("OpenCL not available");
		log.info("OpenCL device {}:{}", !cfg.ocl.device.empty() ? cfg.ocl.device : "DEFAULT", cv::ocl::Device::getDefault().name());
		
		// Connect to database
		if (args.particlesOnly)
			cfg.dbInfo.statsTable.clear();
		Database db(cfg.connInfo, cfg.dbInfo);
		log.info("Database {}:{}/{}", cfg.connInfo.host, cfg.connInfo.port, cfg.dbInfo.name);
		log.info("Particles table '{}'", cfg.dbInfo.particlesTable);
		if (!cfg.dbInfo.statsTable.empty())
			log.info("Stats table '{}'", cfg.dbInfo.statsTable);
		if (!cfg.dbInfo.metaTable.empty()) {
			log.info("Meta table '{}'", cfg.dbInfo.metaTable);
			db.writeMeta({
				0, DateTime::now(),
				cfg.dbInfo.particlesTable,
				cfg.dbInfo.statsTable,
				icemetServerVersion(),
				cfg.str()
			});
		}
		
		// Create workers
		ICEMETServerContext ctx{&args, &cfg, &db};
		Watcher watcher(&ctx);
		Reader reader(&ctx);
		Preproc preproc(&ctx);
		Recon recon(&ctx);
		Analysis analysis(&ctx);
		Saver saver(&ctx);
		Stats stats(&ctx);
		
		// Launch worker threads
		std::vector<std::thread> threads;
		if (args.statsOnly) {
			reader.connect(stats, 2);
			
			threads.push_back(std::thread(&Reader::run, &reader));
			threads.push_back(std::thread(&Stats::run, &stats));
		}
		else if (args.particlesOnly) {
			watcher.connect(preproc, 4);
			preproc.connect(recon, 2);
			recon.connect(analysis, 2);
			analysis.connect(saver, 2);
			
			threads.push_back(std::thread(&Watcher::run, &watcher));
			threads.push_back(std::thread(&Preproc::run, &preproc));
			threads.push_back(std::thread(&Recon::run, &recon));
			threads.push_back(std::thread(&Analysis::run, &analysis));
			threads.push_back(std::thread(&Saver::run, &saver));
		}
		else {
			watcher.connect(preproc, 4);
			preproc.connect(recon, 2);
			recon.connect(analysis, 2);
			analysis.connect(saver, 2);
			analysis.connect(stats, 2);
			
			threads.push_back(std::thread(&Watcher::run, &watcher));
			threads.push_back(std::thread(&Preproc::run, &preproc));
			threads.push_back(std::thread(&Recon::run, &recon));
			threads.push_back(std::thread(&Analysis::run, &analysis));
			threads.push_back(std::thread(&Saver::run, &saver));
			threads.push_back(std::thread(&Stats::run, &stats));
		}
		
		// Join threads
		for (auto it = threads.begin(); it != threads.end(); ++it)
			it->join();
		log.info("Done");
	}
	catch (std::exception& e) {
		log.critical(e.what());
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
