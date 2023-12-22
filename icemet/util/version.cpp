#include "version.hpp"

#include "icemet/util/strfmt.hpp"

VersionInfo::VersionInfo(unsigned int major, unsigned int minor, unsigned int patch, bool dev, const std::string& commit) :
	m_major(major),
	m_minor(minor), 
	m_patch(patch),
	m_dev(dev),
	m_commit(commit) {}

std::string VersionInfo::str() const
{
	std::string end;
	if (m_dev) {
		end += "-dev";
		if (!m_commit.empty())
			end += ":" + m_commit;
	}
	return strfmt("{}.{}.{}{}", m_major, m_minor, m_patch, end);
}
