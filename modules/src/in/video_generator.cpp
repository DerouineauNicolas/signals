#include "../utils/log.hpp"
#include "../utils/tools.hpp"
#include "video_generator.hpp"
#include "../common/pcm.hpp"
#include <cmath>

auto const FRAMERATE = 25;

namespace Modules {
namespace In {

VideoGenerator::VideoGenerator()
	: m_numFrames(0) {
	pins.push_back(uptr(new PinDataDefault<Picture>));
}

void VideoGenerator::process(std::shared_ptr<Data> /*data*/) {
	auto const dim = VIDEO_RESOLUTION;
	auto pic = safe_cast<Picture>(pins[0]->getBuffer(0));

	pic->setResolution(dim);

	// generate video
	auto const p = pic->data();
	auto const FLASH_PERIOD = FRAMERATE;
	auto const flash = (m_numFrames % FLASH_PERIOD) == 0;
	auto const val = flash ? 0xCC : 0x80;
	memset(p, val, dim.yv12size());

	auto const framePeriodIn180k = IClock::Rate / FRAMERATE;
	pic->setTime(m_numFrames * framePeriodIn180k);

	if(m_numFrames % 25 < 2)
		pins[0]->emit(pic);

	++m_numFrames;
}

}
}
