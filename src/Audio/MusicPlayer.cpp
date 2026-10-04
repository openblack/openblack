/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MusicPlayer.h"

#include <algorithm>
#include <utility>

using namespace openblack::audio;

MusicPlayer::MusicPlayer(MusicBackend& backend)
    : _backend(backend)
{
}

MusicPlayer::~MusicPlayer()
{
	Stop(false);
}

bool MusicPlayer::Play(const MusicPlayOptions& options)
{
	if (!options.bank || options.bank->GetChunkCount() == 0)
	{
		Stop(true);
		return false;
	}

	// A channel already playing the bank is told its new volume, and becomes the one heard if it was fading out
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		auto& channel = _channels.at(i);
		if (!channel.active || channel.bank != options.bank)
		{
			continue;
		}
		channel.fadeIn = options.fadeIn;
		channel.targetVolume = options.volume;
		channel.sync = options.sync;
		_current = static_cast<int>(i);
		return true;
	}

	const auto free = std::ranges::find_if(_channels, [](const Channel& c) { return !c.active; });
	if (free == _channels.end())
	{
		return false;
	}
	auto& channel = *free;
	channel = Channel {};
	channel.active = true;
	channel.bank = options.bank;
	channel.groupId = options.bank->groupId;
	channel.targetVolume = options.volume;
	channel.fadeIn = options.fadeIn;
	channel.volume = options.fadeIn ? 0 : options.volume;
	channel.loops = options.bank->loopOverride.value_or(options.loops);
	channel.sync = options.sync;
	channel.pitchPercent = options.pitchPercent;
	channel.onFinished = options.onFinished;
	Start(static_cast<size_t>(free - _channels.begin()), options.startChunk);
	return true;
}

// The music thread starting a channel: from the requested chunk, or where a channel of the same music group has got
// to, and from then on it is the channel heard
void MusicPlayer::Start(size_t index, uint32_t startChunk)
{
	auto& channel = _channels.at(index);
	const auto chunkCount = channel.bank->GetChunkCount();
	auto chunk = (startChunk >= 1 && startChunk <= chunkCount) ? startChunk : 1;
	uint32_t skipFrames = 0;
	if (channel.sync && channel.groupId != -1)
	{
		for (size_t i = 0; i < _channels.size(); ++i)
		{
			auto& other = _channels.at(i);
			if (i == index || !other.active || other.groupId != channel.groupId || other.queued.empty())
			{
				continue;
			}
			// Only what has not been heard yet is still queued
			for (auto played = _backend.ReleasePlayedChunks(other.stream); played > 0 && !other.queued.empty(); --played)
			{
				other.queued.pop_front();
			}
			if (!other.queued.empty() && other.queued.front() < chunkCount)
			{
				chunk = other.queued.front() + 1;
				skipFrames = _backend.GetPlayPosition(other.stream);
			}
			break;
		}
	}

	channel.stream = _backend.CreateStream(channel.pitchPercent);
	channel.nextChunk = chunk - 1;
	channel.playingChunk = chunk;
	if (channel.stream != MusicBackend::k_InvalidStream &&
	    _backend.QueueChunk(channel.stream, *channel.bank, channel.nextChunk, skipFrames))
	{
		channel.queued.push_back(channel.nextChunk);
	}
	++channel.nextChunk;
	Feed(channel);
	ApplyVolume(channel);
	_backend.Play(channel.stream);
	_current = static_cast<int>(index);
}

// Keeps four chunks queued, going round the bank again while it has loops left
void MusicPlayer::Feed(Channel& channel)
{
	const auto chunkCount = channel.bank->GetChunkCount();
	while (!channel.ending && channel.queued.size() < k_QueuedChunks)
	{
		if (channel.nextChunk >= chunkCount)
		{
			if (channel.loops == 0)
			{
				channel.ending = true;
				break;
			}
			--channel.loops;
			channel.nextChunk = 0;
		}
		if (_backend.QueueChunk(channel.stream, *channel.bank, channel.nextChunk, 0))
		{
			channel.queued.push_back(channel.nextChunk);
		}
		++channel.nextChunk;
	}
}

void MusicPlayer::Release(size_t index)
{
	auto& channel = _channels.at(index);
	if (channel.stream != MusicBackend::k_InvalidStream)
	{
		_backend.Destroy(channel.stream);
	}
	channel = Channel {};
	if (std::cmp_equal(_current, index))
	{
		_current = k_NoChannel;
	}
}

void MusicPlayer::Stop(bool fadeOut)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		if (!_channels.at(i).active)
		{
			continue;
		}
		if (fadeOut)
		{
			_channels.at(i).targetVolume = 0;
		}
		else
		{
			Release(i);
		}
	}
}

bool MusicPlayer::IsActive() const
{
	return std::ranges::any_of(_channels, [](const Channel& c) { return c.active; });
}

void MusicPlayer::Update(std::chrono::microseconds dt)
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		auto& channel = _channels.at(i);
		if (!channel.active)
		{
			continue;
		}
		for (auto played = _backend.ReleasePlayedChunks(channel.stream); played > 0 && !channel.queued.empty(); --played)
		{
			channel.queued.pop_front();
		}
		Feed(channel);
		if (!channel.queued.empty())
		{
			channel.playingChunk = channel.queued.front() + 1;
			if (!_backend.IsPlaying(channel.stream))
			{
				_backend.Play(channel.stream);
			}
			continue;
		}
		if (channel.ending && !_backend.IsPlaying(channel.stream))
		{
			// Played to the end
			auto onFinished = std::move(channel.onFinished);
			Release(i);
			if (onFinished)
			{
				onFinished();
			}
		}
	}

	_tickTime += dt;
	while (_tickTime >= k_Tick)
	{
		_tickTime -= k_Tick;
		VolumeTick();
	}
}

// The current channel rises to its target, at once unless it fades in. Every other channel, and the current one
// above its target, falls by 3 a tick, and is released when it reaches silence.
void MusicPlayer::VolumeTick()
{
	for (size_t i = 0; i < _channels.size(); ++i)
	{
		auto& channel = _channels.at(i);
		if (!channel.active)
		{
			continue;
		}
		const bool current = std::cmp_equal(_current, i);
		if (channel.volume < channel.targetVolume && current)
		{
			if (!channel.fadeIn)
			{
				channel.volume = channel.targetVolume;
				channel.fadeIn = true;
			}
			else
			{
				channel.volume = std::min(channel.volume + k_FadeInStep, channel.targetVolume);
			}
			ApplyVolume(channel);
		}
		else if (channel.volume > channel.targetVolume || !current)
		{
			channel.volume = std::max(channel.volume - k_FadeOutStep, 0);
			ApplyVolume(channel);
			if (channel.volume == 0)
			{
				Release(i);
			}
		}
	}
}

void MusicPlayer::ApplyVolume(const Channel& channel)
{
	const auto bankVolume = static_cast<float>(channel.bank->bankVolume) / static_cast<float>(k_MaxVolume);
	_backend.SetVolume(channel.stream, static_cast<float>(channel.volume) / static_cast<float>(k_MaxVolume) * bankVolume);
}
