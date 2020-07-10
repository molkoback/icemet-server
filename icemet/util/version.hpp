#ifndef ICEMET_VERSION_H
#define ICEMET_VERSION_H

#include <string>

class VersionInfo {
private:
	unsigned int m_major;
	unsigned int m_minor;
	unsigned int m_patch;
	bool m_dev;

public:
	VersionInfo(unsigned int major, unsigned int minor, unsigned int patch, bool dev);
	std::string str() const;
};

#endif
