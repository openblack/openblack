#include "MpegAudioDecoder.h"

#include "spdlog/spdlog.h"

extern "C" {
#define DR_MP3_IMPLEMENTATION
#include "dr_mp3.h"
}

using namespace openblack::audio;

bool MpegAudioDecoder::Open(const std::vector<uint8_t>& buffer)
{
	drmp3_bool32 status = drmp3_init_memory(&_mp3, buffer.data(), buffer.size(), NULL);
	return status;
}

void MpegAudioDecoder::Read(std::vector<int16_t>& buffer)
{
	auto frameCount = drmp3_get_pcm_frame_count(&_mp3);
	buffer.resize(frameCount * _mp3.channels);
	drmp3_uint64 framesRead = drmp3_read_pcm_frames_s16(&_mp3, frameCount, buffer.data());
}

ChannelLayout MpegAudioDecoder::GetChannelLayout()
{
	switch (_mp3.channels)
	{
	case 1:
		return ChannelLayout::Mono;
		break;
	case 2:
		return ChannelLayout::Stereo;
		break;
	default:
		throw std::runtime_error("Unsupported channel layout");
	}
}
