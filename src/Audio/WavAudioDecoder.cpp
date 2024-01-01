/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "WavAudioDecoder.h"

#include <spdlog/spdlog.h>

extern "C" {
#define DR_WAV_IMPLEMENTATION
#include <dr_wav.h>
}

using namespace openblack::audio;

bool WavAudioDecoder::Open(const std::vector<uint8_t>& buffer)
{
	const auto status = drwav_init_memory(&_wav, buffer.data(), buffer.size(), nullptr);
	return static_cast<bool>(status);
}

void WavAudioDecoder::Read(std::vector<int16_t>& buffer)
{
	drwav_uint64 frameCount;
	drwav_get_length_in_pcm_frames(&_wav, &frameCount);
	buffer.resize(static_cast<size_t>(frameCount * _wav.channels));
	[[maybe_unused]] auto framesRead = drwav_read_pcm_frames_s16(&_wav, frameCount, buffer.data());
}

ChannelLayout WavAudioDecoder::GetChannelLayout()
{
	switch (_wav.channels)
	{
	case 1:
		return ChannelLayout::Mono;
	case 2:
		return ChannelLayout::Stereo;
	default:
		throw std::runtime_error("Unsupported channel layout");
	}
}
