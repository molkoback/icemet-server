#include "pkg.hpp"

#include "icemet/file.hpp"

#include <archive.h>
#include <archive_entry.h>
#include <yaml-cpp/yaml.h>

#include <cstdio>
#include <stdexcept>
#include <string>

ICEMETV1Package::ICEMETV1Package(const fs::path& p) : Package(p)
{
	open(p);
}

ICEMETV1Package::~ICEMETV1Package()
{
	if (!m_tmp.empty() && fs::exists(m_tmp))
		fs::remove_all(m_tmp);
}

static bool writeArchive(struct archive* arc, const fs::path& p)
{
	const void* buf;
	size_t size;
	off_t offset;
	FILE* fp;
	if ((fp = fopen(p.c_str(), "wb")) == NULL)
		return false;
	while (archive_read_data_block(arc, &buf, &size, &offset) == ARCHIVE_OK) {
		fwrite(buf, 1, size, fp);
	}
	fclose(fp);
	return true;
}

void ICEMETV1Package::readData()
{
	YAML::Node node = YAML::LoadFile(m_data.string());
	fps = node["fps"].as<float>();
	len = node["len"].as<unsigned int>();
	auto names = node["files"].as<std::vector<std::string>>();
	for (const auto name : names)
		m_images.push(cv::makePtr<Image>(name));
}

void ICEMETV1Package::open(const fs::path& p)
{
	// Open archive
	struct archive* arc = archive_read_new();
	archive_read_support_format_zip(arc);
	if (archive_read_open_filename(arc, p.c_str(), 8192) != ARCHIVE_OK)
		throw(std::runtime_error(std::string("Couldn't open archive '") + p.string() + "'"));
	
	// Create paths
	m_tmp = icemetCacheDir();
	m_data = m_tmp / "data.json";
	m_video = m_tmp / "files.avi";
	
	// Loop entries
	struct archive_entry* entry;
	while (archive_read_next_header(arc, &entry) == ARCHIVE_OK) {
		std::string name(archive_entry_pathname(entry));
		if (name == "data.json") {
			writeArchive(arc, m_data);
			readData();
		}
		else if (name == "files.avi") {
			writeArchive(arc, m_video);
			m_cap = cv::VideoCapture(m_video);
		}
		else {
			throw(std::runtime_error("Invalid archive format"));
		}
	}
	archive_read_close(arc);
	archive_read_free(arc);
}

ImgPtr ICEMETV1Package::next()
{
	if (m_images.empty())
		return ImgPtr();
	ImgPtr img = m_images.front();
	m_images.pop();
	m_cap.read(img->original);
	return img;
}

bool isPackage(const fs::path& p)
{
	const std::string ext = p.extension();
	return ext == ".iv1";
}

PkgPtr createPackage(const fs::path& p)
{
	std::string ext = p.extension();
	if (ext == ".iv1")
		return cv::makePtr<ICEMETV1Package>(p);
	else
		throw(std::runtime_error("Unknown package type '" + ext + "'"));
	return NULL;
}
