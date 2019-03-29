#include "math.hpp" 

#include <opencv2/icemet.hpp>

#include <cmath>

const double Math::pi = 3.14159265358979323846;

double Math::equivdiam(double area)
{
	return sqrt(4*area/pi);
}

double Math::heywood(double perim, double area)
{
	return perim / (2*sqrt(pi*area));
}

int Math::median(cv::UMat img)
{
	cv::Mat hist;
	cv::icemet::imghist(img, hist);
	int sum = 0;
	int i = 0;
	while (sum < img.cols * img.rows / 2)
		sum += hist.at<int>(0, i++);
	return i-1;
}

double Math::Vcone(double h, double A1, double A2)
{
	return h * (A1 + sqrt(A1*A2) + A2) / 3.0;
}
