#ifndef ICEMET_TIME_H
#define ICEMET_TIME_H

#include "icemet/icemet.hpp"

#include <thread>

typedef unsigned long long Timestamp;

typedef struct _datetime_info {
	int y, m, d;
	int H, M, S, MS;
} DateTimeInfo;

class DateTime {
private:
	Timestamp m_stamp;
	DateTimeInfo m_info;

public:
	DateTime() { setStamp(0); }
	DateTime(Timestamp stamp) { setStamp(stamp); }
	DateTime(const DateTimeInfo& info) { setInfo(info); }
	DateTime(const std::string& str);
	DateTime(const DateTime& dt) : m_stamp(dt.m_stamp), m_info(dt.m_info) {}
	
	std::string str(const std::string& fmt=std::string(), bool local=false) const;
	
	Timestamp stamp() const { return m_stamp; };
	DateTimeInfo info() const { return m_info; };
	void setStamp(Timestamp stamp);
	void setInfo(const DateTimeInfo& info);
	
	int year() const { return m_info.y; }
	int month() const { return m_info.m; }
	int day() const { return m_info.d; }
	int hour() const { return m_info.H; }
	int min() const { return m_info.M; }
	int sec() const { return m_info.S; }
	int millis() const { return m_info.MS; }
	void setYear(int y) { m_info.y = y; setInfo(m_info); }
	void setMonth(int m) { m_info.m = m; setInfo(m_info); }
	void setDay(int d) { m_info.d = d; setInfo(m_info); }
	void setHour(int H) { m_info.H = H; setInfo(m_info); }
	void setMin(int M) { m_info.M = M; setInfo(m_info); }
	void setSec(int S) { m_info.S = S; setInfo(m_info); }
	void setMillis(int MS) { m_info.MS = MS; setInfo(m_info); }
	
	DateTime& operator=(const DateTime& dt) { m_stamp = dt.m_stamp; m_info = dt.m_info; return *this; }
	friend bool operator==(const DateTime& dt1, const DateTime& dt2) { return dt1.m_stamp == dt2.m_stamp; }
	friend bool operator!=(const DateTime& dt1, const DateTime& dt2) { return !(dt1 == dt2); }
	friend bool operator<(const DateTime& dt1, const DateTime& dt2) { return dt1.m_stamp < dt2.m_stamp; }
	friend bool operator<=(const DateTime& dt1, const DateTime& dt2) { return dt1==dt2 || dt1<dt2; }
	friend bool operator>(const DateTime& dt1, const DateTime& dt2) { return !(dt1==dt2 || dt1<dt2); }
	friend bool operator>=(const DateTime& dt1, const DateTime& dt2) { return !(dt1<dt2); }
	
	static DateTime now();
};

class Measure {
private:
	chr::high_resolution_clock::time_point m_start;

public:
	Measure() : m_start(chr::high_resolution_clock::now()) {}
	Measure(const Measure& m) : m_start(m.m_start) {}
	
	double time();
};

inline void ssleep(int s) { std::this_thread::sleep_for(chr::seconds(s)); }
inline void msleep(int ms) { std::this_thread::sleep_for(chr::milliseconds(ms)); }
inline void usleep(int us) { std::this_thread::sleep_for(chr::microseconds(us)); }
inline void nsleep(int ns) { std::this_thread::sleep_for(chr::nanoseconds(ns)); }

#endif
