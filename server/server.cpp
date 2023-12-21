#include "server/server.hpp"

#include <string>

#define VERSION_MAJOR 1
#define VERSION_MINOR 15
#define VERSION_PATCH 0
#define VERSION_DEV 1

#ifndef ICEMET_SERVER_COMMIT
#define ICEMET_SERVER_COMMIT std::string()
#endif

static const VersionInfo info = VersionInfo(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_DEV, ICEMET_SERVER_COMMIT);

const VersionInfo& icemetServerVersion()
{
	return info;
}
