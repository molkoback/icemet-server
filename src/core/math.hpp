#ifndef MATH_H
#define MATH_H

#include <opencv2/core.hpp>

class Math {
public:
	static const double pi;
	static double Mz(double dist, double z);
	static double equivdiam(double area);
	static double heywood(double perim, double area);
	static int median(cv::UMat img);
	static double Vcone(double h, double A1, double A2);
};

#endif
