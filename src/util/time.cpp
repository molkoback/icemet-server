#include "time.hpp"

Measure::Measure() :
	m_start(chr::high_resolution_clock::now()) {}

Measure::Measure(const Measure& m) :
	m_start(m.m_start) {}

double Measure::time()
{
	auto now = chr::high_resolution_clock::now();
	double delta = chr::duration_cast<chr::microseconds>(now - m_start).count() / 1000000.0;
	m_start = chr::high_resolution_clock::now();
	return delta;
}
