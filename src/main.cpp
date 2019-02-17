#include "analysis.hpp"
#include "preproc.hpp"
#include "reader.hpp"
#include "recon.hpp"
#include "saver.hpp"
#include "stats.hpp"
#include "watcher.hpp"
#include "worker.hpp"
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
"  -s                Stats only. Particles will be fetched from the database.\n";
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
	// Parse args
	std::string cfgFile;
	bool statsFlag = false;
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
				statsFlag = true;
			}
			else {
				printf("Invalid option '%s'\n", arg.c_str());
				return EXIT_FAILURE;
			}
		}
		else {
			cfgFile = arg;
		}
	}
	if (cfgFile.empty()) {
		printf(usageStr);
		return EXIT_FAILURE;
	}
	
	Log log("MAIN");
	try {
		// Suppress OpenCV errors
		cv::redirectError(cvErrorHandler);
		
		// Read config
		Config cfg(cfgFile.c_str());
		Config::setDefaultPtr(&cfg);
		
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
		Database::setDefaultPtr(&db);
		
		// Create data queues
		FileQueue dataWatcher(4);
		FileQueue dataPreproc(2);
		FileQueue dataRecon(2);
		FileQueue dataAnalysisSaver(2);
		FileQueue dataAnalysisStats(2);
		std::vector<FileQueue*> dataAnalysis;
		dataAnalysis.push_back(&dataAnalysisSaver);
		dataAnalysis.push_back(&dataAnalysisStats);
		
		// Launch worker threads
		if (!statsFlag) {
			std::thread watcher(Watcher::start, &dataWatcher);
			std::thread preproc(Preproc::start, &dataWatcher, &dataPreproc);
			std::thread recon(Recon::start, &dataPreproc, &dataRecon);
			std::thread analysis(Analysis::start, &dataRecon, dataAnalysis);
			std::thread saver(Saver::start, &dataAnalysisSaver);
			Stats::start(&dataAnalysisStats);
		}
		else {
			std::thread reader(Reader::start, &dataAnalysisStats);
			Stats::start(&dataAnalysisStats);
		}
	}
	catch (std::exception& e) {
		log.critical(e.what());
	}
	return EXIT_FAILURE; // NOTREACHED
}
