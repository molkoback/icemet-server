#ifndef ICEMET_H
#define ICEMET_H

#include "icemet/util/version.hpp"

#include <opencv2/core.hpp>

#include <filesystem>
#include <limits>
#include <cmath>

#define NAN_FLOAT std::numeric_limits<float>::signaling_NaN()
#define NAN_DOUBLE std::numeric_limits<double>::signaling_NaN()
#define IS_NAN(x) std::isnan(x)
namespace fs = std::filesystem;

const VersionInfo& icemetVersion();

fs::path icemetCacheDir();

#endif
