#ifndef ICEMET_PKG_H
#define ICEMET_PKG_H

#include "icemet/file.hpp"
#include "icemet/img.hpp"

#include <opencv2/core.hpp>

#include <queue>

class Package : public File {
protected:
	std::queue<ImgPtr> m_images;

public:
	Package(const fs::path& p) : File(p) {}
	Package(const Package&) = delete;
	Package& operator=(const Package&) = delete;
	virtual ~Package() {}
	virtual ImgPtr next() = 0;
	
	float fps;
	unsigned int len;
};

class ImageReader {
protected:
	fs::path m_path;

public:
	ImageReader(const fs::path& p) : m_path(p) {}
	virtual ~ImageReader() {}
	virtual bool read(cv::UMat& dst) = 0;
};

class ICEMETV1Package : public Package {
protected:
	fs::path m_tmp;
	cv::Ptr<ImageReader> m_reader;
	
	void open(const fs::path& p);

public:
	ICEMETV1Package(const fs::path& p);
	~ICEMETV1Package();
	ICEMETV1Package(const ICEMETV1Package&) = delete;
	ICEMETV1Package& operator=(const ICEMETV1Package&) = delete;
	ImgPtr next() override;
};
typedef cv::Ptr<Package> PkgPtr;

bool isPackage(const fs::path& p);
PkgPtr createPackage(const fs::path& p);

#endif
