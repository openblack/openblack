/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MusicStreamBackend.h"

#include <span>

#include <spdlog/spdlog.h>

#include "SoundDecoder.h"

using namespace openblack::audio;

namespace
{
constexpr float k_PercentToRate = 0.01f;
} // namespace

MusicStreamBackend::MusicStreamBackend(AudioPlayerInterface& player, Decoder decoder)
    : _player(player)
    , _decoder(std::move(decoder))
{
}

MusicStreamBackend::~MusicStreamBackend()
{
	while (!_streams.empty())
	{
		Destroy(_streams.begin()->first);
	}
}

MusicBackend::Stream MusicStreamBackend::CreateStream(int32_t pitchPercent)
{
	const auto source = _player.CreateSource(static_cast<float>(pitchPercent) * k_PercentToRate, true);
	const auto stream = _nextStream++;
	_streams.emplace(stream, StreamState {.source = source});
	return stream;
}

// A bank's chunks are cut from one MPEG stream at frame boundaries. A decoder starting on a chunk has nothing in its
// synthesis filter, so the first few hundred samples it gives are wrong, mostly silence, which clicks against the end
// of the chunk before. Decoding the chunk after the one that plays before it, and dropping that one's samples, gives
// exactly what decoding the whole stream would.
bool MusicStreamBackend::QueueChunk(Stream stream, const MusicBank& bank, uint32_t chunk, uint32_t skipFrames)
{
	const auto found = _streams.find(stream);
	if (found == _streams.end() || chunk >= bank.GetChunkCount())
	{
		return false;
	}
	auto& state = found->second;
	const auto sampleRate = chunk < bank.chunkSampleRates.size() ? static_cast<int>(bank.chunkSampleRates[chunk]) : 0;
	const auto decode = [this, &bank, sampleRate](std::optional<uint32_t> before, uint32_t chunk) {
		if (!before)
		{
			const auto& data = bank.chunks[chunk];
			return _decoder(std::span<const uint8_t>(data.data(), data.size()), sampleRate);
		}
		const auto& first = bank.chunks[*before];
		const auto& second = bank.chunks[chunk];
		std::vector<uint8_t> data;
		data.reserve(first.size() + second.size());
		data.insert(data.end(), first.begin(), first.end());
		data.insert(data.end(), second.begin(), second.end());
		return _decoder(std::span<const uint8_t>(data.data(), data.size()), sampleRate);
	};

	// What plays before: the chunk queued last, or the one before in the bank when starting part way through
	std::optional<uint32_t> before = state.lastChunk;
	uint64_t beforeFrames = state.lastChunkFrames;
	if (!before && chunk > 0)
	{
		if (const auto alone = decode(std::nullopt, chunk - 1); alone.sound)
		{
			before = chunk - 1;
			beforeFrames = alone.sound->frames;
		}
	}

	auto result = decode(before, chunk);
	if (result.sound && before && result.sound->frames <= beforeFrames)
	{
		result.sound.reset();
	}
	if (!result.sound)
	{
		// Fall back on the chunk by itself
		before.reset();
		beforeFrames = 0;
		result = decode(std::nullopt, chunk);
	}
	if (!result.sound)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("audio"), "Could not decode chunk {} of {}: {}", chunk, bank.path, result.error);
		state.lastChunk.reset();
		return false;
	}
	auto& sound = *result.sound;
	state.lastChunk = chunk;
	state.lastChunkFrames = sound.frames - beforeFrames;

	// Leave out the chunk decoded first, and when starting in time with another channel, the part of this one it has
	// played already
	const auto channels = sound.channelLayout == ChannelLayout::Stereo ? 2u : 1u;
	const auto skipSamples = std::min<size_t>(static_cast<size_t>(beforeFrames + skipFrames) * channels, sound.samples.size());
	sound.samples.erase(sound.samples.begin(), sound.samples.begin() + static_cast<std::ptrdiff_t>(skipSamples));
	if (sound.samples.empty())
	{
		return false;
	}
	const auto buffer = _player.CreateBuffer(sound.channelLayout, sound.samples, sound.sampleRate);
	_player.QueueBuffer(state.source, buffer);
	state.buffers.push_back(buffer);
	return true;
}

uint32_t MusicStreamBackend::ReleasePlayedChunks(Stream stream)
{
	const auto found = _streams.find(stream);
	if (found == _streams.end())
	{
		return 0;
	}
	auto& state = found->second;
	const auto played = _player.UnqueueProcessedBuffers(state.source);
	for (const auto buffer : played)
	{
		_player.DeleteBuffer(buffer);
		if (!state.buffers.empty())
		{
			state.buffers.pop_front();
		}
	}
	return static_cast<uint32_t>(played.size());
}

uint32_t MusicStreamBackend::GetPlayPosition(Stream stream) const
{
	const auto found = _streams.find(stream);
	return found != _streams.end() ? _player.GetSampleOffset(found->second.source) : 0;
}

void MusicStreamBackend::Play(Stream stream)
{
	const auto found = _streams.find(stream);
	if (found != _streams.end() && !found->second.buffers.empty())
	{
		_player.SetVolume(found->second.source, found->second.volume * _outputVolume);
		_player.StartSource(found->second.source);
	}
}

bool MusicStreamBackend::IsPlaying(Stream stream) const
{
	const auto found = _streams.find(stream);
	return found != _streams.end() && _player.GetStatus(found->second.source) == AudioStatus::Playing;
}

void MusicStreamBackend::SetVolume(Stream stream, float volume)
{
	const auto found = _streams.find(stream);
	if (found != _streams.end())
	{
		found->second.volume = volume;
		_player.SetVolume(found->second.source, volume * _outputVolume);
	}
}

void MusicStreamBackend::Destroy(Stream stream)
{
	const auto found = _streams.find(stream);
	if (found == _streams.end())
	{
		return;
	}
	auto& state = found->second;
	// A stopped source has played all of its buffers, so they can all be taken off it
	_player.StopSource(state.source);
	for (const auto buffer : _player.UnqueueProcessedBuffers(state.source))
	{
		_player.DeleteBuffer(buffer);
	}
	_player.DeleteSource(state.source);
	_streams.erase(found);
}

void MusicStreamBackend::SetOutputVolume(float volume)
{
	if (volume == _outputVolume)
	{
		return;
	}
	_outputVolume = volume;
	for (const auto& [stream, state] : _streams)
	{
		_player.SetVolume(state.source, state.volume * _outputVolume);
	}
}
