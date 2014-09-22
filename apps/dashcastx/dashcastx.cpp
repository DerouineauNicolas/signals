#include "../../tests/tests.hpp"
#include "modules.hpp"

#include "../utils/tools.hpp"
#include "libavcodec/avcodec.h" //FIXME: there should be none of the modules include at the application level

#include "decode/libav_decode.hpp"
#include "demux/libav_demux.hpp"
#include "encode/libav_encode.hpp"
#include "mux/gpac_mux_mp4.hpp"
#include "out/null.hpp"

#include <sstream>

using namespace Tests;
using namespace Modules;

namespace {
std::unique_ptr<Encode::LibavEncode> createEncoder(Pin *pPin, PropsDecoder *decoderProps) {
	auto const codecType = decoderProps ? decoderProps->getAVCodecContext()->codec_type : AVMEDIA_TYPE_UNKNOWN;
	if (codecType == AVMEDIA_TYPE_VIDEO) {
		Log::msg(Log::Info, "Found video stream");
		auto r = uptr(Encode::LibavEncode::create(Encode::LibavEncode::Video));
		ConnectPinToModule(pPin, r);
		return std::move(r);
	} else if (codecType == AVMEDIA_TYPE_AUDIO) {
		Log::msg(Log::Info, "Found audio stream");
		auto r = uptr(Encode::LibavEncode::create(Encode::LibavEncode::Audio));
		ConnectPinToModule(pPin, r);
		return std::move(r);
	} else {
		Log::msg(Log::Info, "Found unknown stream");
		return nullptr;
	}
}
}

int safeMain(int argc, char const* argv[]) {

	if(argc != 2)
		throw std::runtime_error("usage: dashcastx <URL>");

	auto const inputURL = argv[1];

	std::list<std::unique_ptr<Module>> modules;

	{
		auto demux = uptr(Demux::LibavDemux::create(inputURL));

		for (size_t i = 0; i < demux->getNumPin(); ++i) {
			auto props = demux->getPin(i)->getProps();
			PropsDecoder *decoderProps = dynamic_cast<PropsDecoder*>(props);
			ASSERT(decoderProps);

			auto decoder = uptr(Decode::LibavDecode::create(*decoderProps));
			ConnectPinToModule(demux->getPin(i), decoder);

			//TODO: add audio and video rescaling

			auto encoder = createEncoder(decoder->getPin(0), decoderProps);
			if (!encoder) {
				auto r = uptr(Out::Null::create());
				ConnectPinToModule(decoder->getPin(0), r);
				modules.push_back(std::move(decoder));
				modules.push_back(std::move(r));
				break;
			}

			//FIXME: should be fragmented according to parameters
			std::stringstream filename;
			filename << i;
			auto muxer = uptr(Mux::GPACMuxMP4::create(filename.str()));
			ConnectPinToModule(encoder->getPin(0), muxer);

			Connect(encoder->declareStream, muxer.get(), &Mux::GPACMuxMP4::declareStream);
			encoder->sendOutputPinsInfo();

			//TODO: add the "floatting" DASHer

			modules.push_back(std::move(decoder));
			modules.push_back(std::move(encoder));
			modules.push_back(std::move(muxer));
		}

		while (demux->process(nullptr)) {
		}

		foreach(i, modules) {
			(*i)->waitForCompletion();
		}
	}

	return 0;
}

int main(int argc, char const* argv[]) {
	try {
		return safeMain(argc, argv);
	} catch(std::exception const& e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}

