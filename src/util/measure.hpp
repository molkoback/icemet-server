#ifndef MEASURE_H
#define MEASURE_H

#include <chrono>

namespace chr = std::chrono;

class Measure {
private:
	chr::high_resolution_clock::time_point m_start;

public:
	Measure();
	Measure(const Measure& m);
	
	double time();
};

#endif
