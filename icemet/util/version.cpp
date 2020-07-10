#include "version.hpp"

#include "icemet/util/strfmt.hpp"

VersionInfo::VersionInfo(unsigned int major, unsigned int minor, unsigned int patch, bool dev=false) :
	m_major(major),
	m_minor(minor), 
	m_patch(patch),
	m_dev(dev) {}

std::string VersionInfo::str() const
{
	std::string end = m_dev ? std::string("-dev") : std::string();
	return strfmt("%d.%d.%d%s", m_major, m_minor, m_patch, end.c_str());
}
