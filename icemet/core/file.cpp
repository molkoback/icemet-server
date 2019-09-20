#include "file.hpp" 

#include "icemet/util/strfmt.hpp"

#include <stdexcept>

File::File() :
	m_sensor(0),
	m_dt(),
	m_frame(0),
	m_empty(true) {}

File::File(const fs::path& p)
{
	setName(p.stem().string());
	setPath(p);
}

File::File(unsigned int sensor, DateTime dt, unsigned int frame, bool empty) :
	m_sensor(sensor),
	m_dt(dt),
	m_frame(frame),
	m_empty(empty) {}

std::string File::name() const
{
	return strfmt(
		"%02u_%02d%02d%02d_%02d%02d%02d%03d_%06u_%c",
		m_sensor,
		m_dt.day(), m_dt.month(), m_dt.year()%100,
		m_dt.hour(), m_dt.min(), m_dt.sec(), m_dt.millis(),
		m_frame,
		m_empty ? 'F' : 'T'
	);
}

void File::setName(const std::string& str)
{
	DateTimeInfo info;
	try {
		m_sensor = std::stoi(str.substr(0, 2));
		info.d = std::stoi(str.substr(3, 2));
		info.m = std::stoi(str.substr(5, 2));
		info.y = std::stoi(str.substr(7, 2)) + 2000;
		info.H = std::stoi(str.substr(10, 2));
		info.M = std::stoi(str.substr(12, 2));
		info.S = std::stoi(str.substr(14, 2));
		info.MS = std::stoi(str.substr(16, 3));
		m_frame = std::stoi(str.substr(20, 6));
		m_empty = str.substr(27, 1).compare("T");
		m_dt.setInfo(info);
	}
	catch (std::exception& e) {
		throw std::invalid_argument("Invalid filename");
	}
}

fs::path File::path(const fs::path& root, const fs::path& ext, int sub) const
{
	std::string end = sub > 0 ? strfmt("_%d", sub) : "";
	fs::path p = dir(root) / fs::path(name() + end);
	p.replace_extension(ext);
	return p;
}

fs::path File::dir(const fs::path& root) const
{
	fs::path p(strfmt("%02d/%02d/%02d/%02d", m_dt.year()%100, m_dt.month(), m_dt.day(), m_dt.hour()));
	p.make_preferred();
	return root / p;
}

bool operator==(const File& f1, const File& f2)
{
	return (
		f1.m_sensor == f2.m_sensor &&
		f1.m_dt == f2.m_dt &&
		f1.m_frame == f2.m_frame
	);
}
bool operator!=(const File& f1, const File& f2) { return !(f1 == f2); }
bool operator<(const File& f1, const File& f2)
{
	return (
		(f1.m_sensor<f2.m_sensor) ||
		(f1.m_sensor==f2.m_sensor && f1.m_dt<f2.m_dt) ||
		(f1.m_sensor==f2.m_sensor && f1.m_dt==f2.m_dt && f1.m_frame<f2.m_frame)
	);
}
bool operator<=(const File& f1, const File& f2) { return f1==f2 || f1<f2; }
bool operator>(const File& f1, const File& f2) { return !(f1==f2 || f1<f2); }
bool operator>=(const File& f1, const File& f2)  { return !(f1<f2); }