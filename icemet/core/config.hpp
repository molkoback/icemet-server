#ifndef ICEMET_CONFIG_H
#define ICEMET_CONFIG_H

#include "icemet/core/database.hpp"
#include "icemet/core/file.hpp"
#include "icemet/util/log.hpp"

#include <opencv2/core.hpp>

#include <string>

typedef struct _arguments {
	std::string cfgFile;
	bool waitNew;
	bool statsOnly;
	LogLevel loglevel;
	
	_arguments() : cfgFile(std::string()), waitNew(true), statsOnly(false), loglevel(LOG_INFO) {}
} Arguments;

typedef struct _paths {
	fs::path watch;
	fs::path results;
	fs::path original;
	fs::path preproc;
	fs::path recon;
	fs::path threshold;
	fs::path preview;
} Paths;

typedef struct _saves {
	bool original;
	bool preproc;
	bool recon;
	bool threshold;
	bool preview;
} Saves;

typedef struct _types {
	fs::path results;
	fs::path lossy;
} Types;

typedef struct _image_param {
	cv::Size2i size;
	cv::Rect rect;
	cv::Size2i border;
} ImageParam;

typedef struct _bgsub_param {
	bool enabled;
	int stackLen;
} BGSubParam;

typedef struct _check_param {
	int discard_th;
	int empty_th;
} CheckParam;

typedef struct _filter_param {
	bool enabled;
	float f;
} FilterParam;

typedef struct _hologram_param {
	float z0;
	float z1;
	float dz;
	float dist;
	float psz;
	float lambda;
	int step;
	float focusK;
} HologramParam;

typedef struct _segment_param {
	float thFact;
	int nMax;
	int sizeMin;
	int sizeSmall;
	int pad;
} SegmentParam;

typedef struct _particle_param {
	float thFact;
	float zMin;
	float zMax;
	float diamMin;
	float diamMax;
	float diamStep;
	float circMin;
	float circMax;
	unsigned char dynRangeMin;
	unsigned char dynRangeMax;
} ParticleParam;

typedef struct _diameter_correction {
	bool enabled;
	float D0;
	float D1;
	float f0;
	float f1;
} DiameterCorrection;

typedef struct _ocl_param {
	std::string device;
} OCLParam;

class Config {
private:
	fs::path strToPath(const std::string& str) const;

public:
	Config() {}
	Config(const char* fn);
	Config(const Config& cfg);
	
	void load(const char* fn);
	
	Arguments args;
	Paths paths;
	Saves saves;
	Types types;
	ConnectionInfo connInfo;
	DatabaseInfo dbInfo;
	ImageParam img;
	BGSubParam bgsub;
	CheckParam check;
	FilterParam lpf;
	HologramParam hologram;
	SegmentParam segment;
	ParticleParam particle;
	DiameterCorrection diamCorr;
	OCLParam ocl;
};

#endif
