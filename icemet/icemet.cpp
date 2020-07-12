#include "icemet/icemet.hpp"

#include <cstdlib>
#include <ctime>

#define VERSION_MAJOR 1
#define VERSION_MINOR 5
#define VERSION_PATCH 0
#define VERSION_DEV 1

static const VersionInfo info = VersionInfo(VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_DEV);
static bool seeded = false;

const VersionInfo& icemetVersion()
{
	return info;
} 

static std::string randstr(unsigned int len)
{
	if (!seeded) {
		srand(time(NULL));
		seeded = true;
	}
	std::string str;
	for (unsigned int i = 0; i < len; i++)
		str += (char)(rand() % 26 + 97);
	return str;
}

fs::path icemetCacheDir()
{
	fs::path p = fs::temp_directory_path() / "icemet" / randstr(16);
	fs::create_directories(p);
	return p;
}
