#ifndef ICEMET_SERVER_H
#define ICEMET_SERVER_H

#include "icemet/database.hpp"
#include "icemet/icemet.hpp"
#include "icemet/util/log.hpp"
#include "icemet/util/version.hpp"
#include "server/config.hpp"

typedef struct _arguments {
	fs::path cfgFile;
	fs::path root;
	bool waitNew;
	bool statsOnly;
	bool particlesOnly;
	LogLevel loglevel;
	
	_arguments() : cfgFile(fs::path()), root(fs::path(".")), waitNew(true), statsOnly(false), particlesOnly(false), loglevel(LOG_INFO) {}
} Arguments;

typedef struct _icemet_server_context {
	Arguments* args;
	Config* cfg;
	Database* db;
} ICEMETServerContext;

const VersionInfo& icemetServerVersion();

#endif
