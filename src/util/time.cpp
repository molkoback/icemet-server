#include "time.hpp"

#include "util/strfmt.hpp"

#include <ctime>
#include <stdexcept>

DateTime::DateTime(const std::string& str)
{
	DateTimeInfo info;
	try {
		info.y = std::stoi(str.substr(0, 4));
		info.m = std::stoi(str.substr(5, 2));
		info.d = std::stoi(str.substr(8, 2));
		info.H = std::stoi(str.substr(11, 2));
		info.M = std::stoi(str.substr(14, 2));
		info.S = std::stoi(str.substr(17, 2));
		info.MS = std::stoi(str.substr(20, 3));
		setInfo(info);
	}
	catch (std::exception& e) {
		throw std::invalid_argument("Invalid datetime");
	}
}

void DateTime::setStamp(Timestamp stamp)
{
	m_stamp = stamp;
	time_t t = m_stamp / 1000;
	struct tm* timeinfo = localtime(&t);
	m_info.y = timeinfo->tm_year + 1900;
	m_info.m = timeinfo->tm_mon + 1;
	m_info.d = timeinfo->tm_mday;
	m_info.H = timeinfo->tm_hour;
	m_info.M = timeinfo->tm_min;
	m_info.S = timeinfo->tm_sec;
	m_info.MS = m_stamp % 1000;
}

void DateTime::setInfo(const DateTimeInfo& info)
{
	struct tm timeinfo;
	timeinfo.tm_year = info.y - 1900;
	timeinfo.tm_mon = info.m - 1;
	timeinfo.tm_mday = info.d;
	timeinfo.tm_hour = info.H;
	timeinfo.tm_min = info.M;
	timeinfo.tm_sec = info.S;
	timeinfo.tm_isdst = -1;
	int t = mktime(&timeinfo);
	if (t < 0)
		throw std::exception();
	m_info = info;
	m_stamp = (Timestamp)t*1000 + (Timestamp)info.MS % 1000;
}

std::string DateTime::str() const
{
	time_t t = m_stamp / 1000;
	struct tm* timeinfo = localtime(&t);
	char buf[23];
	strftime(buf, 23, "%Y-%m-%d %H:%M:%S", timeinfo);
	return strfmt("%s.%03d", buf, m_stamp % 1000);
}

double Measure::time()
{
	auto now = chr::high_resolution_clock::now();
	double delta = chr::duration_cast<chr::microseconds>(now - m_start).count() / 1000000.0;
	m_start = chr::high_resolution_clock::now();
	return delta;
}
