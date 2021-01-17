#include "WavAudioDecoder.h"

#include "spdlog/spdlog.h"

extern "C" {
#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
}

using namespace openblack::audio;

bool WavAudioDecoder::Open(const std::vector<uint8_t>& buffer)
{
	drwav_bool32 status = drwav_init_memory(&_wav, buffer.data(), buffer.size(), NULL);
	return status;
}

void WavAudioDecoder::Read(std::vector<int16_t>& buffer)
{
	drwav_uint64 frameCount;
	drwav_get_length_in_pcm_frames(&_wav, &frameCount);
	buffer.resize(frameCount * _wav.channels);
	drwav_uint64 framesRead = drwav_read_pcm_frames_s16(&_wav, frameCount, buffer.data());
}

ChannelLayout WavAudioDecoder::GetChannelLayout()
{
	switch (_wav.channels)
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
