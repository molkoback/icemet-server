#ifndef ICEMET_HOLOGRAM_H
#define ICEMET_HOLOGRAM_H

#include <opencv2/core.hpp>

#include <vector>

typedef enum _recon_output {
	RECON_OUTPUT_AMPLITUDE,
	RECON_OUTPUT_PHASE,
	RECON_OUTPUT_COMPLEX
} ReconOutput;

typedef enum _focus_method {
	FOCUS_MIN,
	FOCUS_MAX,
	FOCUS_RANGE,
	FOCUS_STD,
	FOCUS_TOG
} FocusMethod;

typedef enum _filter_type {
	FILTER_LOWPASS,
	FILTER_HIGHPASS
} FilterType;

class ZRange {
public:
	ZRange() : start(0), stop(0), step(0) {}
	ZRange(float start_, float stop_, float step_) : start(start_), stop(stop_), step(step_) {}
	
	int n();
	float z(int i);
	int i(float z);
	
	float start;
	float stop;
	float step;
};

class Hologram {
private:
	cv::Size2i m_sizeOrig;
	cv::Size2i m_sizePad;
	
	float m_psz;
	float m_lambda;
	float m_dist;
	
	cv::UMat m_prop;
	cv::UMat m_dft;
	cv::UMat m_complex;
	
	void propagate(float z);

public:
	Hologram(float psz, float lambda, float dist=0.0);
	
	void setImg(const cv::UMat& img);
	void recon(cv::UMat& dst, float z, ReconOutput output=RECON_OUTPUT_AMPLITUDE);
	
	void min(cv::UMat& dst, ZRange z);
	void reconMin(std::vector<cv::UMat>& dst, cv::UMat& dstMin, ZRange z);
	
	float focus(ZRange z, FocusMethod method=FOCUS_STD, float K=3.0);
	float focus(ZRange z, std::vector<cv::UMat>& src, const cv::Rect& rect, int &idx, double &score, FocusMethod method=FOCUS_STD, float K=3.0);
	
	void applyFilter(const cv::UMat& H);
	cv::UMat createFilter(float f, FilterType type) const;
	cv::UMat createLPF(float f) const;
	cv::UMat createHPF(float f) const;
	
	static float magnf(float dist, float z);
	
	static void focus(std::vector<cv::UMat>& src, const cv::Rect& rect, int &idx, double &score, FocusMethod method=FOCUS_STD, int begin=0, int end=-1, float K=3.0);
};
typedef cv::Ptr<Hologram> HologramPtr;

#endif
