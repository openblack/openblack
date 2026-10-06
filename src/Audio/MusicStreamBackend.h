/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <deque>
#include <functional>
#include <map>
#include <optional>
#include <span>

#include "AudioPlayerInterface.h"
#include "MusicPlayer.h"
#include "SoundDecoder.h"

namespace openblack::audio
{

/// Streams music channels with OpenAL: each stream is a source centred on the listener with the decoded chunks of
/// its bank queued on it
class MusicStreamBackend final: public MusicBackend
{
public:
	/// Decodes a chunk of a bank, or chunks joined end to end, given the bank header's sample rate
	using Decoder = std::function<DecodeResult(std::span<const uint8_t> data, int expectedSampleRate)>;

	explicit MusicStreamBackend(AudioPlayerInterface& player, Decoder decoder = DecodeSound);
	~MusicStreamBackend() override;
	MusicStreamBackend(const MusicStreamBackend&) = delete;
	MusicStreamBackend& operator=(const MusicStreamBackend&) = delete;

	[[nodiscard]] Stream CreateStream(int32_t pitchPercent) override;
	bool QueueChunk(Stream stream, const MusicBank& bank, uint32_t chunk, uint32_t skipFrames) override;
	[[nodiscard]] uint32_t ReleasePlayedChunks(Stream stream) override;
	[[nodiscard]] uint32_t GetPlayPosition(Stream stream) const override;
	void Play(Stream stream) override;
	[[nodiscard]] bool IsPlaying(Stream stream) const override;
	void SetVolume(Stream stream, float volume) override;
	void Destroy(Stream stream) override;

	/// The music and master volumes, applied on top of each stream's own
	void SetOutputVolume(float volume);

private:
	struct StreamState
	{
		SourceId source;
		std::deque<BufferId> buffers;
		float volume {0.0f};
		/// The chunk queued last and its length in frames, which the next chunk is decoded after
		std::optional<uint32_t> lastChunk;
		uint64_t lastChunkFrames {0};
	};

	AudioPlayerInterface& _player;
	Decoder _decoder;
	std::map<Stream, StreamState> _streams;
	Stream _nextStream {1};
	float _outputVolume {1.0f};
};

} // namespace openblack::audio
