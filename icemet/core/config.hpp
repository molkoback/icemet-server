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
	int stackLen;
} BGSubParam;

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
	int focusPoints;
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
	Arguments m_args;
	Paths m_paths;
	Types m_types;
	ConnectionInfo m_connInfo;
	DatabaseInfo m_dbInfo;
	ImageParam m_img;
	BGSubParam m_bgsub;
	FilterParam m_lpf;
	HologramParam m_hologram;
	SegmentParam m_segment;
	ParticleParam m_particle;
	DiameterCorrection m_diamCorr;
	OCLParam m_ocl;
	
	fs::path strToPath(const std::string& str) const;

public:
	Config() {}
	Config(const char* fn);
	Config(const Config& cfg);
	
	void load(const char* fn);
	
	void setArgs(const Arguments& args) { m_args = args; }
	
	const Arguments& args() const { return m_args; }
	const Paths& paths() const { return m_paths; }
	const Types& types() const { return m_types; }
	const ConnectionInfo& connInfo() const { return m_connInfo; }
	const DatabaseInfo& dbInfo() const { return m_dbInfo; }
	const ImageParam& img() const { return m_img; }
	const BGSubParam& bgsub() const { return m_bgsub; }
	const FilterParam lpf() const { return m_lpf; }
	const HologramParam& hologram() const { return m_hologram; }
	const SegmentParam& segment() const { return m_segment; }
	const ParticleParam& particle() const { return m_particle; }
	const DiameterCorrection& diamCorr() const { return m_diamCorr; }
	const OCLParam& ocl() const { return m_ocl; }
};

#endif
