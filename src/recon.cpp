#include "recon.hpp"

#include "util/measure.hpp"
#include "util/sleep.hpp"

#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

#include <algorithm>
#include <queue>

#define FOCUS_POINTS 20

Recon::Recon(FileQueue* input, FileQueue* output) :
	Worker(COLOR_GREEN "RECON" COLOR_RESET),
	m_input(input),
	m_output(output)
{
	m_hologram = cv::icemet::Hologram::create(
		m_cfg->img().size,
		m_cfg->hologram().psz, m_cfg->hologram().dist, m_cfg->hologram().lambda
	);
	
	// Allocate image stack
	for (int i = 0; i < m_cfg->hologram().step; i++)
		m_stack.emplace_back(m_cfg->img().size, CV_8UC1);
	
	// Create filters
	if (m_cfg->lpf().enabled)
		m_lpf = m_hologram->createLPF(m_cfg->lpf().f);
}

void Recon::process(cv::Ptr<File> file)
{
	cv::Size2i size = m_cfg->img().size;
	cv::Size2i border = m_cfg->img().border;
	cv::Rect crop(
		border.width, border.height,
		size.width-2*border.width, size.height-2*border.height
	);
	
	int segmNMax = m_cfg->segment().nMax;
	int segmSizeMin = m_cfg->segment().sizeMin;
	int segmSizeSmall = m_cfg->segment().sizeSmall;
	int pad = m_cfg->segment().pad;
	
	float z0 = m_cfg->hologram().z0;
	float z1 = m_cfg->hologram().z1;
	float ldz = m_cfg->hologram().dz;
	float dz = m_cfg->hologram().step*ldz;
	
	int th = m_cfg->segment().thFact * file->param.bgVal;
	
	// Set our image and apply filters
	m_hologram->setImg(file->preproc);
	if (m_lpf.u)
		m_hologram->applyFilter(m_lpf);
	
	// Reconstruct whole range in steps
	int count = 0;
	int ncontours = 0;
	for (float lz0 = z0; lz0 < z1; lz0 += dz) {
		cv::UMat imgMin(size, CV_8UC1, cv::Scalar(255));
		float lz1 = std::min(lz0 + dz, z1);
		int last = roundf((lz1 - lz0) / ldz) - 1;
		m_hologram->recon(m_stack, imgMin, lz0, lz1, ldz);
		
		// Threshold
		cv::UMat imgTh;
		cv::threshold(cv::UMat(imgMin, crop), imgTh, th, 255, cv::THRESH_BINARY_INV);
		
		// Find all contours and process them
		std::vector<std::vector<cv::Point>> contours;
		std::vector<cv::Vec4i> hierarchy;
		cv::findContours(
			imgTh,
			contours, hierarchy,
			cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE,
			cv::Point(border.width, border.height)
		);
		
		// Create rects from contours
		ncontours += contours.size();
		for (const auto& cnt : contours) {
			cv::Rect rect = cv::boundingRect(cnt);
			if (rect.width < segmSizeMin || rect.height < segmSizeMin)
				continue;
			
			// Select our focus method
			cv::icemet::FocusMethod method = (
				rect.width > segmSizeSmall ||
				rect.height > segmSizeSmall
			) ? cv::icemet::FOCUS_STD : cv::icemet::FOCUS_MIN;
			
			// Grow rect
			rect.x = std::max(rect.x-pad, border.width);
			rect.y = std::max(rect.y-pad, border.height);
			rect.width = std::min(rect.width+2*pad, size.width-border.width-rect.x-1);
			rect.height = std::min(rect.height+2*pad, size.height-border.height-rect.y-1);
			
			// Focus
			int idx = 0;
			double score = 0.0;
			cv::icemet::Hologram::focus(m_stack, rect, idx, score, method, 0, last, FOCUS_POINTS);
			
			// Create segment
			cv::Ptr segm = cv::makePtr<Segment>();
			segm->z = lz0 + idx*ldz;
			segm->score = score;
			segm->method = method;
			segm->rect = rect;
			cv::UMat(m_stack[idx], rect).copyTo(segm->img);
			file->segments.push_back(segm);
			
			if (++count >= segmNMax) {
				m_log.warning("The maximum number of segments reached");
				goto end;
			}
		}
	}
	if (!count)
		file->setEmpty(true);
end:
	m_log.debug("Segments: %d, Contours: %d", count, ncontours);
}

bool Recon::cycle()
{
	// Collect files
	std::queue<cv::Ptr<File>> files;
	m_input->collect(files);
	
	// Process
	while (!files.empty()) {
		cv::Ptr<File> file = files.front();
		if (!file->empty()) {
			m_log.debug("Reconstructing %s", file->name().c_str());
			Measure m;
			process(file);
			m_log.debug("Done %s (%.2f s)", file->name().c_str(), m.time());
		}
		m_output->pushWait(file);
		files.pop();
	}
	msleep(1);
	return true;
}

void Recon::start(FileQueue* input, FileQueue* output)
{
	Recon(input, output).run();
}
