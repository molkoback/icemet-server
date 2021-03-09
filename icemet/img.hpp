#ifndef ICEMET_IMAGE_H
#define ICEMET_IMAGE_H

#include "icemet/file.hpp"
#include "icemet/hologram.hpp"

#include <opencv2/core.hpp>

#include <string>
#include <vector>

typedef struct _segment {
	float z;
	int step;
	double score;
	FocusMethod method;
	cv::Rect rect;
	cv::Mat img;
	
	_segment() {}
	_segment(float z_, int step_, double score_, FocusMethod method_, const cv::Rect& rect_, const cv::UMat& img_) :
		z(z_), step(step_), score(score_), method(method_), rect(rect_)
	{
		img_.copyTo(img);
	}
	_segment(const _segment& s) :
		z(s.z), step(s.step), score(s.score), method(s.method), rect(s.rect)
	{
		s.img.copyTo(img);
	}
} Segment;
typedef cv::Ptr<Segment> SegmentPtr;

typedef struct _particle {
	float x, y, z;
	float diam;
	float diamCorr;
	float circularity;
	unsigned char dynRange;
	float effPxSz;
	cv::Mat img;
	
	_particle() {}
	_particle(float x_, float y_, float z_, float diam_, float diamCorr_, float circularity_, unsigned char dynRange_, float effPxSz_, const cv::UMat& img_) :
		x(x_), y(y_), z(z_),
		diam(diam_), diamCorr(diamCorr_),
		circularity(circularity_),
		dynRange(dynRange_), effPxSz(effPxSz_)
	{
		img_.copyTo(img);
	}
	_particle(const _particle& p) :
		x(p.x), y(p.y), z(p.z),
		diam(p.diam), diamCorr(p.diamCorr),
		circularity(p.circularity),
		dynRange(p.dynRange), effPxSz(p.effPxSz)
	{
		p.img.copyTo(img);
	}
} Particle;
typedef cv::Ptr<Particle> ParticlePtr;

class Image : public File {
public:
	Image();
	Image(const std::string& name);
	Image(const fs::path& p);
	Image(const Image&) = delete;
	Image& operator=(const Image&) = delete;
	
	unsigned char bgVal;
	cv::UMat original;
	cv::UMat preproc;
	cv::UMat min;
	std::vector<SegmentPtr> segments;
	std::vector<ParticlePtr> particles;
};
typedef cv::Ptr<Image> ImgPtr;

class BGSubStack {
private:
	size_t m_len;
	cv::Size2i m_size;
	size_t m_idx;
	bool m_full;
	std::vector<ImgPtr> m_images;
	cv::UMat m_stack;
	cv::Mat m_means;

public:
	BGSubStack(size_t len);
	
	size_t len() { return m_len; }
	
	bool push(const ImgPtr& img);
	ImgPtr get(size_t idx);
	ImgPtr meddiv();
};
typedef cv::Ptr<BGSubStack> BGSubStackPtr;

#endif
