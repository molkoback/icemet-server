#ifndef ICEMET_H
#define ICEMET_H

#include "icemet/util/version.hpp"

#include <opencv2/core.hpp>

#include <filesystem>

namespace fs = std::filesystem;

const VersionInfo& icemetVersion();

fs::path icemetCacheDir();

#endif
