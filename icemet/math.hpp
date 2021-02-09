#ifndef ICEMET_MATH_H
#define ICEMET_MATH_H

#include <opencv2/core.hpp>

class Math {
public:
	static const double pi;
	static double equivdiam(double area);
	static double heywood(double perim, double area);
	static double Vcone(double h, double A1, double A2);
	static int median(cv::UMat img);
	static void adjust(const cv::Mat& src, cv::Mat& dst, uchar a0, uchar a1, uchar b0, uchar b1);
	static void adjust(const cv::UMat& src, cv::UMat& dst, uchar a0, uchar a1, uchar b0, uchar b1);
	static void hist(const cv::UMat& src, cv::Mat& counts, cv::Mat& bins, float min, float max, float step);
	static void hist(const cv::UMat& src, cv::Mat& dst);
};

#endif
