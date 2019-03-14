#ifndef FILE_H
#define FILE_H

#include "core/datetime.hpp"
#include "core/safequeue.hpp"

#include <opencv2/core.hpp>
#include <opencv2/icemet.hpp>

#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

typedef struct _segment {
	float z;
	double score;
	cv::icemet::FocusMethod method;
	cv::Rect rect;
	cv::Mat img;
	
	_segment() {}
	_segment(float z_, double score_, cv::icemet::FocusMethod method_, const cv::Rect& rect_, const cv::UMat& img_) :
		z(z_), score(score_), method(method_), rect(rect_)
	{
		img_.copyTo(img);
	}
	_segment(const _segment& s) :
		z(s.z), score(s.score), method(s.method), rect(s.rect)
	{
		s.img.copyTo(img);
	}
} Segment;

typedef struct _particle {
	float x, y, z;
	float diam;
	float circularity;
	unsigned char dnr;
	float effpsz;
	cv::Mat img;
	
	_particle() {}
	_particle(float x_, float y_, float z_, float diam_, float circularity_, unsigned char dnr_, float effpsz_, const cv::UMat& img_) :
		x(x_), y(y_), z(z_),
		diam(diam_), circularity(circularity_),
		dnr(dnr_), effpsz(effpsz_)
	{
		img_.copyTo(img);
	}
	_particle(const _particle& p) :
		x(p.x), y(p.y), z(p.z),
		diam(p.diam), circularity(p.circularity),
		dnr(p.dnr), effpsz(p.effpsz)
	{
		p.img.copyTo(img);
	}
} Particle;

typedef struct _file_param {
	unsigned char bgVal;
} FileParam;

class File {
private:
	unsigned int m_sensor;
	DateTime m_dt;
	unsigned int m_frame;
	bool m_empty;
	fs::path m_path;

public:
	File();
	File(const fs::path& p);
	File(unsigned int m_sensor, DateTime dt, unsigned int frame, bool empty);
	File(const File& f);
	
	FileParam param;
	cv::UMat original;
	cv::UMat preproc;
	std::vector<cv::Ptr<Segment>> segments;
	std::vector<cv::Ptr<Particle>> particles;
	
	unsigned int sensor() const { return m_sensor; }
	void setSensor(unsigned int sensor) { m_sensor = sensor; }
	DateTime dt() const { return m_dt; }
	void setDt(DateTime dt) { m_dt = dt; }
	unsigned int frame() const { return m_frame; }
	void setFrame(unsigned int frame) { m_frame = frame; }
	bool empty() const { return m_empty; }
	void setEmpty(bool empty) { m_empty = empty; }
	
	std::string name() const;
	void setName(const std::string& str);
	
	fs::path path() const { return m_path; }
	fs::path path(const fs::path& root, const fs::path& ext, int sub=0) const;
	void setPath(const fs::path& p) { m_path = p; }
	
	fs::path dir(const fs::path& root) const;
	
	friend bool operator==(const File& f1, const File& f2);
	friend bool operator!=(const File& f1, const File& f2);
	friend bool operator<(const File& f1, const File& f2);
	friend bool operator<=(const File& f1, const File& f2);
	friend bool operator>(const File& f1, const File& f2);
	friend bool operator>=(const File& f1, const File& f2);
};

typedef SafeQueue<cv::Ptr<File>> FileQueue;

#endif
