#include "math.hpp" 

#include "opencl/icemet_math_ocl.hpp"

#include <opencv2/core/ocl.hpp>

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

double Math::Vcone(double h, double A1, double A2)
{
	return h * (A1 + sqrt(A1*A2) + A2) / 3.0;
}

int Math::median(cv::UMat img)
{
	cv::Mat hist;
	Math::hist(img, hist);
	int sum = 0;
	int i = 0;
	while (sum < img.cols * img.rows / 2)
		sum += hist.at<int>(0, i++);
	return i-1;
}

void Math::adjust(const cv::Mat& src, cv::Mat& dst, uchar a0, uchar a1, uchar b0, uchar b1)
{
	CV_Assert(src.type() == CV_8UC1);
	cv::Mat tmp;
	src.convertTo(tmp, CV_32FC1);
	
	cv::max(tmp, a0, tmp);
	cv::min(tmp, a1, tmp);
	cv::subtract(tmp, a0, tmp);
	cv::divide(tmp, a1-a0, tmp);
	cv::multiply(tmp, b1-b0, tmp);
	cv::add(tmp, b0, tmp);
	
	tmp.convertTo(dst, CV_8UC1);
}

void Math::adjust(const cv::UMat& src, cv::UMat& dst, uchar a0, uchar a1, uchar b0, uchar b1)
{
	CV_Assert(src.type() == CV_8UC1);
	if (dst.empty())
		dst = cv::UMat(src.size(), CV_8UC1);
	
	size_t gsize[1] = {(size_t)(src.cols * src.rows)};
	cv::ocl::Kernel("adjust", icemet_math_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(src),
		cv::ocl::KernelArg::PtrWriteOnly(dst),
		a0, a1, b0, b1
	).run(1, gsize, NULL, true);
}

void Math::hist(const cv::UMat& src, cv::Mat& dst)
{
	cv::UMat tmp = cv::UMat::zeros(1, 256, CV_32SC1);
	size_t gsize[1] = {(size_t)(src.cols * src.rows)};
	cv::ocl::Kernel("imghist", icemet_math_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(src),
		cv::ocl::KernelArg::PtrReadWrite(tmp)
	).run(1, gsize, NULL, true);
	tmp.copyTo(dst);
}
