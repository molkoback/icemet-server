#include "icemet/icemet.hpp" 

#define VERSION_MAJOR 1
#define VERSION_MINOR 4
#define VERSION_PATCH 0
#define VERSION_DEV 0

static const VersionInfo info = VersionInfo(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_DEV);

const VersionInfo& icemet_version()
{
	return info;
} 
