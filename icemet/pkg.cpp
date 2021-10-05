#include "pkg.hpp"

#include "icemet/file.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <opencv2/imgproc.hpp>
#include <yaml-cpp/yaml.h>

#include <cstdio>
#include <stdexcept>
#include <string>

class ICEMETV1PackageEntry {
private:
	char* m_data;
	size_t m_size;

public:
	ICEMETV1PackageEntry() : m_data(NULL), m_size(0) {}
	~ICEMETV1PackageEntry() { free(m_data); }
	
	bool empty() { return m_size == 0; }
	
	bool open(struct archive* arc, struct archive_entry* entry)
	{
		m_size = archive_entry_size(entry);
		m_data = (char*)malloc(m_size);
		
		size_t offsetData = 0;
		const void* block;
		size_t sizeBlock;
		la_int64_t offsetBlock;
		int r;
		while ((r = archive_read_data_block(arc, &block, &sizeBlock, &offsetBlock)) == ARCHIVE_OK) {
			if (offsetData + sizeBlock > m_size)
				goto fail;
			memcpy(m_data + offsetData, block, sizeBlock);
			offsetData += sizeBlock;
		}
		if (r < 0)
			goto fail;
		
		return true;
		fail:
		free(m_data);
		m_data = NULL;
		m_size = 0;
		return false;
	}
	
	bool save(const fs::path& path)
	{
		FILE* fp;
		if ((fp = fopen(path.string().c_str(), "wb")) == NULL)
			return false;
		size_t n = fwrite(m_data, 1, m_size, fp);
		fclose(fp);
		return n == m_size;
	}
};

ICEMETV1Package::ICEMETV1Package(const fs::path& p) : Package(p)
{
	open(p);
}

ICEMETV1Package::~ICEMETV1Package()
{
	if (m_cap.isOpened())
		m_cap.release();
	if (!m_tmp.empty() && fs::exists(m_tmp))
		fs::remove_all(m_tmp);
}

void ICEMETV1Package::open(const fs::path& p)
{
	// Open archive
	struct archive* arc = archive_read_new();
	archive_read_support_format_zip(arc);
	if (archive_read_open_filename(arc, p.string().c_str(), 8192) != ARCHIVE_OK)
		throw(std::runtime_error(std::string("Couldn't open archive '") + p.string() + "'"));
	
	// Paths
	m_tmp = icemetCacheDir();
	fs::path pathImages, pathData;
	
	// Loop entries
	struct archive_entry* entry;
	ICEMETV1PackageEntry entryData;
	ICEMETV1PackageEntry entryVideo;
	int r;
	while ((r = archive_read_next_header(arc, &entry)) == ARCHIVE_OK) {
		std::string name(archive_entry_pathname(entry));
		if (name.rfind("data", 0) == 0) {
			if (!entryData.open(arc, entry))
				throw(std::runtime_error("Incomplete or corrupted archive"));
			pathData = m_tmp / name;
		}
		else if (name.rfind("images", 0) == 0) {
			if (!entryVideo.open(arc, entry))
				throw(std::runtime_error("Incomplete or corrupted archive"));
			pathImages = m_tmp / name;
		}
		else {
			throw(std::runtime_error("Invalid archive format"));
		}
	}
	archive_read_close(arc);
	archive_read_free(arc);
	if (r < 0 || entryData.empty())
		throw(std::runtime_error("Incomplete or corrupted archive"));
	
	// Save entires
	entryData.save(pathData);
	if (!entryVideo.empty())
		entryVideo.save(pathImages);
	
	// Open files
	YAML::Node node = YAML::LoadFile(pathData.string());
	fps = node["fps"].as<float>();
	len = node["len"].as<unsigned int>();
	auto names = node["images"].as<std::vector<std::string>>();
	if (!names.empty()) {
		if (entryVideo.empty())
			throw(std::runtime_error("Incomplete or corrupted archive"));
		for (auto name : names)
			m_images.push(cv::makePtr<Image>(name));
		m_cap = cv::VideoCapture(pathImages.string());
		if (!m_cap.isOpened())
			throw(std::runtime_error("Incomplete or corrupted archive"));
	}
}

ImgPtr ICEMETV1Package::next()
{
	if (m_images.empty())
		return ImgPtr();
	ImgPtr img = m_images.front();
	m_images.pop();
	
	cv::UMat imgBGR;
	if (!m_cap.read(imgBGR))
		return ImgPtr();
	
	cv::cvtColor(imgBGR, img->original, cv::COLOR_BGR2GRAY);
	return img;
}

bool isPackage(const fs::path& p)
{
	std::string ext = p.extension().string();
	return ext == ".iv1" || ext == ".ip1";
}

PkgPtr createPackage(const fs::path& p)
{
	std::string ext = p.extension().string();
	if (ext == ".iv1" || ext == ".ip1")
		return cv::makePtr<ICEMETV1Package>(p);
	else
		throw(std::runtime_error("Unknown package type '" + ext + "'"));
	return NULL;
}
