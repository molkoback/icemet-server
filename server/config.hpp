#ifndef ICEMET_SERVER_CONFIG_H
#define ICEMET_SERVER_CONFIG_H

#include "icemet/database.hpp"
#include "icemet/hologram.hpp"
#include "icemet/icemet.hpp"

#include <opencv2/core.hpp>

#include <string>

typedef struct _paths {
	fs::path watch;
	fs::path results;
	fs::path original;
	fs::path preproc;
	fs::path min;
	fs::path recon;
	fs::path threshold;
	fs::path preview;
} Paths;

typedef struct _saves {
	bool original;
	bool preproc;
	bool min;
	bool recon;
	bool threshold;
	bool preview;
	bool empty;
	bool skipped;
} Saves;

typedef struct _types {
	fs::path results;
	fs::path lossy;
} Types;

typedef struct _image_param {
	cv::Size2i size;
	cv::Rect rect;
	cv::Size2i border;
	float rotation;
} ImageParam;

typedef struct _bgsub_param {
	int stackLen;
} BGSubParam;

typedef struct _empty_check_param {
	int originalTh;
	int preprocTh;
	int reconTh;
} EmptyCheckParam;

typedef struct _noisy_check_param {
	int reconTh;
} NoisyCheckParam;

typedef struct _filter_param {
	float f;
} FilterParam;

typedef struct _hologram_param {
	float z0;
	float z1;
	float dz0;
	float dz1;
	float dist;
	float psz;
	float lambda;
	int reconStep;
	double focusStep;
	FocusMethod focusMethod;
	FocusMethod focusMethodSmall;
} HologramParam;

typedef struct _segment_param {
	ReconOutput thMethod;
	int thBg;
	float thFact;
	int nMax;
	int sizeMin;
	int sizeMax;
	int sizeSmall;
	int pad;
	float scale;
} SegmentParam;

typedef struct _particle_param {
	float thFact;
	float zMin;
	float zMax;
	float diamMin;
	float diamMax;
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

typedef struct _stats_param {
	unsigned int time;
	int frames;
	float temp;
	float wind;
} StatsParam;

typedef struct _ocl_param {
	std::string device;
} OCLParam;

class Config {
private:
	std::string m_str;
	fs::path strToPath(const std::string& str) const;

public:
	Config() {}
	Config(const fs::path& fn);
	Config(const Config& cfg);
	
	void load(const fs::path& fn);
	const std::string& str() { return m_str; };
	
	Paths paths;
	Saves saves;
	Types types;
	ConnectionInfo connInfo;
	DatabaseInfo dbInfo;
	ImageParam img;
	BGSubParam bgsub;
	EmptyCheckParam emptyCheck;
	NoisyCheckParam noisyCheck;
	FilterParam lpf;
	HologramParam hologram;
	SegmentParam segment;
	ParticleParam particle;
	DiameterCorrection diamCorr;
	StatsParam stats;
	OCLParam ocl;
};

#endif
