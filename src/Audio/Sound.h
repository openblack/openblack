/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

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
using SourceId = ALuint;
using BufferId = ALuint;

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
	int id;
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
