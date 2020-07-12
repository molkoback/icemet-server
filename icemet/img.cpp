#include "img.hpp"

#include <opencv2/imgcodecs.hpp>

#include <stdexcept>

Image::Image(const std::string& name) : File(name) {}

Image::Image(const fs::path& p) : File(p)
{
	cv::Mat mat = cv::imread(p.string(), cv::IMREAD_GRAYSCALE);
	if (!mat.data) {
		throw(std::runtime_error(std::string("Couldn't open image '") + p.string() + "'"));
	}
	mat.getUMat(cv::ACCESS_READ).copyTo(original);
}
