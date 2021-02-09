#ifndef ICEMET_FILE_H
#define ICEMET_FILE_H

#include "icemet/icemet.hpp"
#include "icemet/util/time.hpp"

#include <opencv2/core.hpp>

#include <string>
#include <vector>

typedef char FileStatus;
const FileStatus FILE_STATUS_NONE = 'X';
const FileStatus FILE_STATUS_NOTEMPTY = 'T';
const FileStatus FILE_STATUS_EMPTY = 'F';
const FileStatus FILE_STATUS_SKIP = 'S';

class File {
private:
	unsigned int m_sensor;
	DateTime m_dt;
	unsigned int m_frame;
	FileStatus m_status;
	fs::path m_path;

public:
	File();
	File(const std::string& name);
	File(const fs::path& p);
	File(unsigned int m_sensor, DateTime dt, unsigned int frame, FileStatus status);
	File(const File&) = delete;
	File& operator=(const File&) = delete;
	
	unsigned int sensor() const { return m_sensor; }
	void setSensor(unsigned int sensor) { m_sensor = sensor; }
	DateTime dt() const { return m_dt; }
	void setDt(const DateTime& dt) { m_dt = dt; }
	unsigned int frame() const { return m_frame; }
	void setFrame(unsigned int frame) { m_frame = frame; }
	
	FileStatus status() const { return m_status; }
	void setStatus(FileStatus status);
	
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
typedef cv::Ptr<File> FilePtr;

#endif
