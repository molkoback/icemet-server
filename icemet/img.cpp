#include "img.hpp"

#include "opencl/icemet_bgsub_ocl.hpp"

#include <opencv2/core/ocl.hpp>
#include <opencv2/imgcodecs.hpp>

#include <stdexcept>

#define BGSUBSTACK_LEN_MAX 25

Image::Image(const std::string& name) : File(name) {}

Image::Image(const fs::path& p) : File(p)
{
	cv::Mat mat = cv::imread(p.string(), cv::IMREAD_GRAYSCALE);
	if (!mat.data) {
		throw(std::runtime_error(std::string("Couldn't open image '") + p.string() + "'"));
	}
	mat.getUMat(cv::ACCESS_READ).copyTo(original);
}

BGSubStack::BGSubStack(size_t len) :
	m_len(len),
	m_idx(0),
	m_full(false),
	m_images(len)
{
	if (len < 3 || len > BGSUBSTACK_LEN_MAX || len%2 == 0)
		throw(std::invalid_argument("Invalid BGSubStack length"));
}

bool BGSubStack::push(const ImgPtr& img)
{
	if (m_stack.empty()) {
		m_size = img->preproc.size();
		m_stack = cv::UMat(1, m_len * m_size.width * m_size.height, CV_8UC1);
		m_means = cv::Mat(1, m_len, CV_32FC1);
	}
	
	int idx = m_idx;
	size_t gsize[1] = {(size_t)(m_size.width * m_size.height)};
	cv::ocl::Kernel("push", icemet_bgsub_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(img->preproc),
		cv::ocl::KernelArg::PtrWriteOnly(m_stack),
		m_size.width * m_size.height, idx
	).run(1, gsize, NULL, true);
	
	cv::Scalar imgMean = cv::mean(img->preproc);
	m_means.at<float>(0, m_idx) = imgMean[0];
	m_images[m_idx] = img;
	
	// Increment index
	m_idx = (m_idx+1) % m_len;
	if (m_idx == 0)
		m_full = true;
	return m_full;
}

ImgPtr BGSubStack::get(size_t idx)
{
	return m_images[idx];
}

ImgPtr BGSubStack::meddiv()
{
	int idx = (m_idx + m_len/2) % m_len;
	int len = m_len;
	size_t gsize[1] = {(size_t)(m_size.width * m_size.height)};
	cv::ocl::Kernel("meddiv", icemet_bgsub_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(m_stack),
		cv::ocl::KernelArg::PtrReadOnly(m_means.getUMat(cv::ACCESS_READ)),
		cv::ocl::KernelArg::PtrWriteOnly(m_images[idx]->preproc),
		m_size.width * m_size.height, len, idx
	).run(1, gsize, NULL, true);
	return m_images[idx];
}
