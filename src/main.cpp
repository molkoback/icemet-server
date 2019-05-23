#include "analysis.hpp"
#include "preproc.hpp"
#include "reader.hpp"
#include "recon.hpp"
#include "saver.hpp"
#include "stats.hpp"
#include "watcher.hpp"
#include "core/data.hpp"
#include "core/database.hpp"
#include "util/log.hpp"
#include "util/strfmt.hpp"

#include <opencv2/core/ocl.hpp>

#include <cstdio>
#include <cstdlib>
#include <exception>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

static const char* usageStr = "Usage: icemet-server [options] config.yaml\n";
static const char* helpStr =
"Options:\n"
"  -h                Print this help message and exit\n"
"  -V                Print version info and exit\n"
"  -s                Stats only. Particles will be fetched from the database.\n"
"  -Q                Quit after processing all available files.\n";
static const char* versionStr =
"ICEMET Server " ICEMET_VERSION "\n"
"\n"
"Copyright (C) 2019 Eero Molkoselkä <eero.molkoselka@gmail.com>\n";

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
	for (int i = 1; i < argc; i++) {
		std::string arg(argv[i]);
		if (arg[0] == '-') {
			if (!arg.compare("-h")) {
				printf("%s\n%s", usageStr, helpStr);
				return EXIT_SUCCESS;
			}
			else if (!arg.compare("-V")) {
				printf(versionStr);
				return EXIT_SUCCESS;
			}
			else if (!arg.compare("-s")) {
				args.statsOnly = true;
			}
			else if (!arg.compare("-Q")) {
				args.waitNew = false;
			}
			else {
				printf("Invalid option '%s'\n", arg.c_str());
				return EXIT_FAILURE;
			}
		}
		else {
			args.cfgFile = arg;
		}
	}
	if (args.cfgFile.empty()) {
		printf(usageStr);
		return EXIT_FAILURE;
	}
	
	Log log("MAIN");
	try {
		// Suppress OpenCV errors
		cv::redirectError(cvErrorHandler);
		
		// Read config
		Config cfg(args.cfgFile.c_str());
		cfg.setArgs(args);
		
		// Setup logging
		Log::setLevel(cfg.log().level);
		
		// Initialize OpenCL
		const char* device = cfg.ocl().device.c_str();
		std::vector<char> vec = vecfmt("OPENCV_OPENCL_DEVICE=%s", device);
		if (putenv(&vec[0]) || !cv::ocl::useOpenCL())
			throw std::runtime_error("OpenCL not available");
		log.info("OpenCL device %s:%s", device, cv::ocl::Device::getDefault().name().c_str());
		
		// Connect to database
		const ConnectionInfo& connInfo = cfg.connInfo();
		const DatabaseInfo& dbInfo = cfg.dbInfo();
		Database db(connInfo, dbInfo);
		log.info("Database %s:%d/%s", connInfo.host.c_str(), connInfo.port, dbInfo.name.c_str());
		log.info("Particle table '%s'", dbInfo.particleTable.c_str());
		log.info("Stats table '%s'", dbInfo.statsTable.c_str());
		
		// Create data queues
		Data data;
		
		// Create workers
		WorkerPointers ptrs = {&cfg, &data, &db};
		Watcher watcher(ptrs);
		Reader reader(ptrs);
		Preproc preproc(ptrs);
		Recon recon(ptrs);
		Analysis analysis(ptrs);
		Saver saver(ptrs);
		Stats stats(ptrs);
		
		// Launch worker threads
		std::vector<std::thread> threads;
		if (!args.statsOnly) {
			threads.push_back(std::thread(&Watcher::run, &watcher));
			threads.push_back(std::thread(&Preproc::run, &preproc));
			threads.push_back(std::thread(&Recon::run, &recon));
			threads.push_back(std::thread(&Analysis::run, &analysis));
			threads.push_back(std::thread(&Saver::run, &saver));
			threads.push_back(std::thread(&Stats::run, &stats));
		}
		else {
			threads.push_back(std::thread(&Reader::run, &reader));
			threads.push_back(std::thread(&Stats::run, &stats));
		}
		
		// Join threads
		for (auto it = threads.begin(); it != threads.end(); ++it)
			it->join();
		log.info("Done");
	}
	catch (std::exception& e) {
		log.critical(e.what());
	}
	return EXIT_FAILURE; // NOTREACHED
}
