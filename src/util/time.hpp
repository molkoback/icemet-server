#ifndef TIME_H
#define TIME_H

#include <chrono>
#include <thread>

namespace chr = std::chrono;

inline void ssleep(int s) { std::this_thread::sleep_for(chr::seconds(s)); }
inline void msleep(int ms) { std::this_thread::sleep_for(chr::milliseconds(ms)); }
inline void usleep(int us) { std::this_thread::sleep_for(chr::microseconds(us)); }
inline void nsleep(int ns) { std::this_thread::sleep_for(chr::nanoseconds(ns)); }

class Measure {
private:
	chr::high_resolution_clock::time_point m_start;

public:
	Measure();
	Measure(const Measure& m);
	
	double time();
};

#endif
