#include "server/server.hpp"

#define VERSION_MAJOR 1
#define VERSION_MINOR 7
#define VERSION_PATCH 1
#define VERSION_DEV 1

static const VersionInfo info = VersionInfo(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_DEV);

const VersionInfo& icemetServerVersion()
{
	return info;
}
