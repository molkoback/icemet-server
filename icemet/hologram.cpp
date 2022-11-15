#include "hologram.hpp"

#include "opencl/icemet_hologram_ocl.hpp"

#include <opencv2/core/ocl.hpp>

#include <limits>
#include <map>

#define FILTER_N 6
#define FILTER_F 0.5

typedef struct _focus_param {
	int type; // OpenCV type
	ReconOutput output;
	std::function<double(const cv::UMat&)> scoreFunc;
} FocusParam;

ZRange::ZRange(float z0, float z1, float dz0, float dz1)
{
	setParam(z0, z1, dz0, dz1);
}

void ZRange::setParam(float z0, float z1, float dz0, float dz1)
{
	m_z.clear();
	m_dz.clear();
	float a = (dz0 - dz1) / (z0*z0 - z1*z1);
	float b = dz0 - a * z0*z0;
	float z = z0;
	float dz = dz0;
	while (z < z1) {
		m_z.push_back(z);
		m_dz.push_back(dz);
		z += dz;
		dz = a * z*z + b;
	}
}

int ZRange::n() const
{
	return m_z.size();
}

float ZRange::z(int i) const
{
	return m_z[i];
}

float ZRange::dz(int i) const
{
	return m_dz[i];
}

static double scoreMin(const cv::UMat& slice)
{
	double min, max;
	cv::minMaxLoc(slice, &min, &max);
	return -min;
}

static double scoreMax(const cv::UMat& slice)
{
	double min, max;
	cv::minMaxLoc(slice, &min, &max);
	return max;
}

static double scoreRange(const cv::UMat& slice)
{
	double min, max;
	cv::minMaxLoc(slice, &min, &max);
	return max - min;
}

static double scoreSTD(const cv::UMat& slice)
{
	size_t gsize[2] = {(size_t)slice.cols, (size_t)slice.rows};
	cv::UMat filt(slice.size(), CV_32FC1);
	cv::Vec<double,1> mean;
	cv::Vec<double,1> stddev;
	cv::ocl::Kernel("stdfilt_3x3", icemet_hologram_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(slice),
		cv::ocl::KernelArg::WriteOnly(filt)
	).run(2, gsize, NULL, true);
	cv::meanStdDev(filt, mean, stddev);
	return stddev[0];
}

static double scoreToG(const cv::UMat& slice)
{
	size_t gsize[2] = {(size_t)slice.cols, (size_t)slice.rows};
	cv::UMat grad(slice.size(), CV_32FC1);
	cv::Vec<double,1> mean;
	cv::Vec<double,1> stddev;
	cv::ocl::Kernel("gradient", icemet_hologram_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(slice),
		cv::ocl::KernelArg::WriteOnly(grad)
	).run(2, gsize, NULL, true);
	cv::meanStdDev(grad, mean, stddev);
	return sqrt(stddev[0] / mean[0]);
}

static double scoreICEMET(const cv::UMat& slice)
{
	size_t gsize[2] = {(size_t)slice.cols, (size_t)slice.rows};
	cv::UMat filt(slice.size(), CV_32FC1);
	cv::Vec<double,1> mean;
	cv::Vec<double,1> stddev;
	cv::ocl::Kernel("sqrt_stdfilt_3x3", icemet_hologram_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(slice),
		cv::ocl::KernelArg::WriteOnly(filt)
	).run(2, gsize, NULL, true);
	cv::meanStdDev(filt, mean, stddev);
	return stddev[0];
}

static const FocusParam focusParam[] {
	{CV_32FC1, RECON_OUTPUT_AMPLITUDE, scoreMin},
	{CV_32FC1, RECON_OUTPUT_AMPLITUDE, scoreMax},
	{CV_32FC1, RECON_OUTPUT_AMPLITUDE, scoreRange},
	{CV_32FC1, RECON_OUTPUT_AMPLITUDE, scoreSTD},
	{CV_32FC2, RECON_OUTPUT_COMPLEX, scoreToG},
	{CV_32FC1, RECON_OUTPUT_AMPLITUDE, scoreICEMET}
};

static const FocusParam* getFocusParam(FocusMethod method)
{
	return &focusParam[method];
}

static double SSearch(std::function<double(double)> f, double begin, double end, double step, cv::TermCriteria termcrit=cv::TermCriteria(cv::TermCriteria::MAX_ITER+cv::TermCriteria::EPS, 1000, 1.0))
{
	CV_Assert(step > 0.0 && step < (end-begin)/2.0);
	double nsteps = (end - begin) / step;
	int count = 0;
	while (count++ < termcrit.maxCount) {
		double fmax = -std::numeric_limits<double>::max();
		double imax = 0.0;
		for (double i = begin+step; i < end-step/2; i+=step) {
			double fsum = f(i-step) + 2*f(i) + f(std::min(end, i+step));
			if (fsum > fmax) {
				fmax = fsum;
				imax = i;
			}
		}
		begin = std::max(begin, imax - step);
		end = std::min(end, imax + step);
		if (step <= termcrit.epsilon) break;
		step = std::max(termcrit.epsilon, (end - begin) / nsteps);
	}
	return (end + begin) / 2.0;
}

void Hologram::propagate(float z)
{
	size_t gsizeProp[1] = {(size_t)(m_sizePad.width * m_sizePad.height)};
	cv::ocl::Kernel("propagate", icemet_hologram_ocl()).args(
		cv::ocl::KernelArg::PtrReadOnly(m_dft),
		cv::ocl::KernelArg::PtrReadOnly(m_prop),
		cv::ocl::KernelArg::PtrWriteOnly(m_complex),
		z * magnf(m_dist, z)
	).run(1, gsizeProp, NULL, true);
	cv::idft(m_complex, m_complex, cv::DFT_COMPLEX_INPUT|cv::DFT_COMPLEX_OUTPUT);
}

Hologram::Hologram(float psz, float lambda, float dist) :
	m_psz(psz),
	m_lambda(lambda),
	m_dist(dist) {}

void Hologram::setImg(const cv::UMat& img)
{
	CV_Assert(img.channels() == 1);
	if (img.size() != m_sizeOrig) {
		m_sizeOrig = img.size();
		m_sizePad = cv::Size2i(cv::getOptimalDFTSize(m_sizeOrig.width), cv::getOptimalDFTSize(m_sizeOrig.height));
		
		// Allocate cv::UMats
		m_prop = cv::UMat::zeros(m_sizePad, CV_32FC2);
		m_dft = cv::UMat::zeros(m_sizePad, CV_32FC2);
		m_complex = cv::UMat::zeros(m_sizePad, CV_32FC2);
		
		// Fill propagator
		size_t gsize[2] = {(size_t)m_sizePad.width, (size_t)m_sizePad.height};
		cv::ocl::Kernel("angularspectrum", icemet_hologram_ocl()).args(
			cv::ocl::KernelArg::WriteOnly(m_prop),
			cv::Vec2f(m_psz*m_sizePad.width, m_psz*m_sizePad.height),
			m_lambda
		).run(2, gsize, NULL, true);
	}
	
	cv::UMat padded(m_sizePad, CV_32FC1, cv::mean(img));
	img.convertTo(cv::UMat(padded, cv::Rect(cv::Point(0, 0), m_sizeOrig)), CV_32FC1);
	
	// FFT
	cv::dft(padded, m_dft, cv::DFT_COMPLEX_OUTPUT|cv::DFT_SCALE, m_sizeOrig.height);
}

void Hologram::recon(cv::UMat& dst, float z, ReconOutput output)
{
	size_t gsize[2] = {(size_t)m_sizePad.width, (size_t)m_sizePad.height};
	propagate(z);
	switch (output) {
	case RECON_OUTPUT_COMPLEX:
		cv::UMat(m_complex, cv::Rect(cv::Point(0, 0), m_sizeOrig)).copyTo(dst);
		return;
	case RECON_OUTPUT_AMPLITUDE:
		if (dst.empty())
			dst = cv::UMat(m_sizeOrig, CV_32FC1);
		cv::ocl::Kernel("amplitude", icemet_hologram_ocl()).args(
			cv::ocl::KernelArg::ReadOnly(m_complex),
			cv::ocl::KernelArg::WriteOnly(dst)
		).run(2, gsize, NULL, true);
		break;
	case RECON_OUTPUT_PHASE:
		if (dst.empty())
			dst = cv::UMat(m_sizeOrig, CV_32FC1);
		cv::ocl::Kernel("phase", icemet_hologram_ocl()).args(
			cv::ocl::KernelArg::ReadOnly(m_complex),
			cv::ocl::KernelArg::WriteOnly(dst),
			m_lambda,
			z * magnf(m_dist, z)
		).run(2, gsize, NULL, true);
		break;
	}
}

void Hologram::min(cv::UMat& dst, const ZRange& range)
{
	size_t gsize[2] = {(size_t)m_sizePad.width, (size_t)m_sizePad.height};
	int n = range.n();
	
	if (dst.empty())
		dst = cv::UMat(m_sizeOrig, CV_8UC1, cv::Scalar(255));
	for (int i = 0; i < n; i++) {
		propagate(range.z(i));
		cv::ocl::Kernel("min_8u", icemet_hologram_ocl()).args(
			cv::ocl::KernelArg::ReadOnly(m_complex),
			cv::ocl::KernelArg::WriteOnly(dst)
		).run(2, gsize, NULL, true);
	}
}

void Hologram::reconMin(std::vector<cv::UMat>& dst, cv::UMat& dstMin, const ZRange& range)
{
	size_t gsize[2] = {(size_t)m_sizePad.width, (size_t)m_sizePad.height};
	int n = range.n();
	
	if (dstMin.empty())
		dstMin = cv::UMat(m_sizeOrig, CV_8UC1, cv::Scalar(255));
	int empty = n - dst.size();
	for (int i = 0; i < empty; i++)
		dst.emplace_back(m_sizeOrig, CV_8UC1);
	
	for (int i = 0; i < n; i++) {
		propagate(range.z(i));
		cv::ocl::Kernel("amplitude_min_8u", icemet_hologram_ocl()).args(
			cv::ocl::KernelArg::ReadOnly(m_complex),
			cv::ocl::KernelArg::WriteOnly(dst[i]),
			cv::ocl::KernelArg::PtrReadWrite(dstMin)
		).run(2, gsize, NULL, true);
	}
}

float Hologram::focus(const ZRange& range, FocusMethod method, double step)
{
	const FocusParam* param = getFocusParam(method);
	cv::UMat slice(m_sizeOrig, param->type);
	std::map<int,double> scores;
	auto f = [&](double x) {
		int i = round(x);
		auto it = scores.find(i);
		if (it != scores.end()) {
			return it->second;
		}
		else {
			recon(slice, range.z(i), param->output);
			double newScore = param->scoreFunc(slice);
			scores[i] = newScore;
			return newScore;
		}
	};
	return range.z(SSearch(f, 0, range.n()-1, step));
}

float Hologram::focus(const ZRange& range, std::vector<cv::UMat>& src, const cv::Rect& rect, int &idx, double &score, FocusMethod method, double step)
{
	const FocusParam* param = getFocusParam(method);
	if (src.empty())
		src = std::vector<cv::UMat>(range.n());
	cv::UMat slice(rect.size(), param->type);
	std::map<int,double> scores;
	auto f = [&](double x) {
		int i = round(x);
		auto it = scores.find(i);
		if (it != scores.end()) {
			return it->second;
		}
		else {
			if (src[i].empty())
				recon(src[i], range.z(i), param->output);
			cv::UMat(src[i], rect).convertTo(slice, param->type);
			double newScore = param->scoreFunc(slice);
			scores[i] = newScore;
			return newScore;
		}
	};
	idx = SSearch(f, 0, range.n()-1, step);
	score = f(idx);
	return range.z(idx);
}

void Hologram::applyFilter(const cv::UMat& H)
{
	mulSpectrums(m_dft, H, m_dft, 0);
}

cv::UMat Hologram::createFilter(float f, FilterType type) const
{
	CV_Assert(!m_sizePad.empty());
	float sigma = f * pow(log(1.0/pow(FILTER_F, 2)), -1.0/(2.0*FILTER_N));
	cv::UMat H(m_sizePad, CV_32FC2);
	size_t gsize[2] = {(size_t)m_sizePad.width, (size_t)m_sizePad.height};
	cv::ocl::Kernel("supergaussian", icemet_hologram_ocl()).args(
		cv::ocl::KernelArg::WriteOnly(H),
		cv::Vec2f(m_psz*m_sizePad.width, m_psz*m_sizePad.height),
		type,
		cv::Vec2f(sigma, sigma),
		FILTER_N
	).run(2, gsize, NULL, true);
	return H;
}

cv::UMat Hologram::createLPF(float f) const
{
	return createFilter(f, FILTER_LOWPASS);
}

cv::UMat Hologram::createHPF(float f) const
{
	return createFilter(f, FILTER_HIGHPASS);
}

float Hologram::magnf(float dist, float z)
{
	return dist == 0.0 ? 1.0 : dist / (dist - z);
}

void Hologram::focus(std::vector<cv::UMat>& src, const cv::Rect& rect, int &idx, double &score, FocusMethod method, int begin, int end, double step)
{
	int sz = src.size();
	end = end < 0 || end > sz-1 ? sz-1 : end;
	
	const FocusParam* param = getFocusParam(method);
	cv::UMat slice(rect.height, rect.width, param->type);
	std::map<int,double> scores;
	auto f = [&](double x) {
		int i = round(x);
		auto it = scores.find(i);
		if (it != scores.end()) {
			return it->second;
		}
		else {
			cv::UMat(src[i], rect).convertTo(slice, param->type);
			double newScore = param->scoreFunc(slice);
			scores[i] = newScore;
			return newScore;
		}
	};
	idx = SSearch(f, begin, end, step);
	score = f(idx);
}
