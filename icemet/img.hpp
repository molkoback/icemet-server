#ifndef ICEMET_IMAGE_H
#define ICEMET_IMAGE_H

#include "icemet/file.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <string>
#include <vector>

typedef struct _segment {
	float z;
	int iter;
	double score;
	cv::icemet::FocusMethod method;
	cv::Rect rect;
	cv::Mat img;
	
	_segment() {}
	_segment(float z_, int iter_, double score_, cv::icemet::FocusMethod method_, const cv::Rect& rect_, const cv::UMat& img_) :
		z(z_), iter(iter_), score(score_), method(method_), rect(rect_)
	{
		img_.copyTo(img);
	}
	_segment(const _segment& s) :
		z(s.z), iter(s.iter), score(s.score), method(s.method), rect(s.rect)
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
	std::vector<SegmentPtr> segments;
	std::vector<ParticlePtr> particles;
};
typedef cv::Ptr<Image> ImgPtr;

#endif
