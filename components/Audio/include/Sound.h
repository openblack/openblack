/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <queue>
#include <string>
#include <vector>

extern "C" {
#include <AL/al.h>
#include <AL/alc.h>
}

namespace openblack::audio
{
typedef ALuint SourceId; // NOLINT(modernize-use-using)
typedef ALuint BufferId; // NOLINT(modernize-use-using)

enum class AudioStatus
{
	Initial,
	Playing,
	Paused,
	Stopped
};

enum class ChannelLayout
{
	Mono,
	Stereo
};

enum class PlayType
{
	Repeat,
	Once,
	Overlap
};

class Sound
{
public:
	std::string name;
	int sampleRate;
	int priority;
	int bitRate;
	float volume;
	int pitch;
	int pitchDeviation;
	ChannelLayout channelLayout;
	PlayType playType;
	BufferId bufferId;
	float duration;
	std::vector<std::vector<uint8_t>> buffer;
	size_t sizeInBytes;
};
} // namespace openblack::audio
