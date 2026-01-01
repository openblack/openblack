/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <dr_mp3.h>

#include "AudioDecoderInterface.h"

namespace openblack::audio
{

class MpegAudioDecoder final: public AudioDecoderInterface
{
public:
	bool Open(const std::vector<uint8_t>& buffer) override;
	void Read(std::vector<int16_t>& buffer) override;
	[[nodiscard]] ChannelLayout GetChannelLayout() override;

private:
	drmp3 _mp3;
};

} // namespace openblack::audio
