/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>
#include <vector>

#include "AudioPlayerInterface.h"

namespace openblack::audio
{
class AudioDecoderInterface
{
public:
	virtual bool Open(const std::vector<uint8_t>& buffer) = 0;
	virtual void Read(std::vector<int16_t>& buffer) = 0;
	[[nodiscard]] virtual ChannelLayout GetChannelLayout() = 0;
};
} // namespace openblack::audio
