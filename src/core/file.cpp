#include "file.hpp" 

#include "util/strfmt.hpp"

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

File::File(const File& f) :
	m_sensor(f.m_sensor),
	m_dt(f.m_dt),
	m_frame(f.m_frame),
	m_empty(f.m_empty),
	m_path(f.m_path)
{
	param = f.param;
	f.original.copyTo(original);
	f.preproc.copyTo(preproc);
	segments = f.segments;
	particles = f.particles;
}

std::string File::name() const
{
	return strfmt(
		"%02u_%02d%02d%02d_%02d%02d%02d%03d_%06u_%c",
		m_sensor,
		m_dt.d, m_dt.m, m_dt.y-2000,
		m_dt.H, m_dt.M, m_dt.S, m_dt.MS,
		m_frame,
		m_empty ? 'F' : 'T'
	);
}

void File::setName(const std::string& str)
{
	try {
		m_sensor = std::stoi(str.substr(0, 2));
		m_dt.d = std::stoi(str.substr(3, 2));
		m_dt.m = std::stoi(str.substr(5, 2));
		m_dt.y = std::stoi(str.substr(7, 2)) + 2000;
		m_dt.H = std::stoi(str.substr(10, 2));
		m_dt.M = std::stoi(str.substr(12, 2));
		m_dt.S = std::stoi(str.substr(14, 2));
		m_dt.MS = std::stoi(str.substr(16, 3));
		m_frame = std::stoi(str.substr(20, 6));
		m_empty = str.substr(27, 1).compare("T");
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
	fs::path p(strfmt("%02d/%02d/%02d/%02d", m_dt.y-2000, m_dt.m, m_dt.d, m_dt.H));
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
