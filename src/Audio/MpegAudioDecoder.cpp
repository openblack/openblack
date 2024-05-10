/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MpegAudioDecoder.h"

#include <spdlog/spdlog.h>

extern "C" {
#if defined(__GNUC__)
#pragma GCC diagnostic push
#if __GNUC__ >= 13
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif
#endif
#define DR_MP3_IMPLEMENTATION
#include <dr_mp3.h>
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

using namespace openblack::audio;

bool MpegAudioDecoder::Open(const std::vector<uint8_t>& buffer)
{
	const auto status = drmp3_init_memory(&_mp3, buffer.data(), buffer.size(), nullptr);
	return static_cast<bool>(status);
}

void MpegAudioDecoder::Read(std::vector<int16_t>& buffer)
{
	auto frameCount = drmp3_get_pcm_frame_count(&_mp3);
	buffer.resize(static_cast<size_t>(frameCount * _mp3.channels));
	[[maybe_unused]] const auto framesRead = drmp3_read_pcm_frames_s16(&_mp3, frameCount, buffer.data());
}

ChannelLayout MpegAudioDecoder::GetChannelLayout()
{
	switch (_mp3.channels)
	{
	case 1:
		return ChannelLayout::Mono;
	case 2:
		return ChannelLayout::Stereo;
	default:
		throw std::runtime_error("Unsupported channel layout");
	}
}
