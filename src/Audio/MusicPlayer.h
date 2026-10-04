/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace openblack::audio
{

/// A music bank: a sound bank whose samples are chunks of one piece of music, sect0000.mpg onwards, played back to
/// back. Banks of one music group are variations of the same piece with the same chunk lengths, so the game can switch
/// between them in time.
struct MusicBank
{
	std::string path;
	/// LHBankGetMusicGroupId: the group of the first sample, 0 for none
	int32_t groupId {0};
	/// Volume of the bank, 0 to 127: the first sample's header volume where it overrides it
	uint32_t bankVolume {127};
	/// Loop count from the first sample's header where it overrides the caller's
	std::optional<int32_t> loopOverride;
	std::vector<std::vector<uint8_t>> chunks;
	std::vector<uint32_t> chunkSampleRates;

	[[nodiscard]] uint32_t GetChunkCount() const { return static_cast<uint32_t>(chunks.size()); }
};

/// Where LHAudioDLL's music channels are streamed
class MusicBackend
{
public:
	using Stream = uint32_t;
	static constexpr Stream k_InvalidStream = 0;

	virtual ~MusicBackend() = default;
	/// A stream for a channel, playing at pitchPercent of the chunks' rate
	[[nodiscard]] virtual Stream CreateStream(int32_t pitchPercent) = 0;
	/// Decodes a chunk of a bank, 0-based, and queues it behind the stream's queued chunks, leaving out its first
	/// skipFrames frames. False if the chunk could not be decoded.
	virtual bool QueueChunk(Stream stream, const MusicBank& bank, uint32_t chunk, uint32_t skipFrames) = 0;
	/// Releases the queued chunks that have finished playing and says how many there were
	[[nodiscard]] virtual uint32_t ReleasePlayedChunks(Stream stream) = 0;
	/// Frames played of the oldest queued chunk
	[[nodiscard]] virtual uint32_t GetPlayPosition(Stream stream) const = 0;
	/// Plays the queued chunks, or picks up again after running out of them
	virtual void Play(Stream stream) = 0;
	[[nodiscard]] virtual bool IsPlaying(Stream stream) const = 0;
	/// Volume from 0 to 1, before the music and master volumes
	virtual void SetVolume(Stream stream, float volume) = 0;
	/// Stops the stream and releases it, the handle is invalid afterwards
	virtual void Destroy(Stream stream) = 0;
};

/// LH_MusicPlayOptions
struct MusicPlayOptions
{
	std::shared_ptr<const MusicBank> bank;
	/// Target volume, 0 to 127
	int32_t volume {127};
	/// 1-based chunk to start from, the first one if out of range
	uint32_t startChunk {1};
	/// Times to play the bank again after the first, -1 for ever. The bank header can override it.
	int32_t loops {0};
	/// Start in time with a channel already playing a bank of the same music group
	bool sync {false};
	/// Fade in from silence rather than starting at full volume
	bool fadeIn {false};
	int32_t pitchPercent {100};
	/// Called when the bank has played to its end
	std::function<void()> onFinished;
};

/// Port of the music part of LHAudioDLL ver7.0 (LHMusicPlay, LHMusicStop, LHMusicIsActive and the music thread).
///
/// Six channels each stream a bank chunk by chunk, keeping four chunks queued. A channel plays its bank through,
/// starting again from the first chunk while it has loops left. One channel at a time is current: it fades in to its
/// target volume while the others fade out, so a new piece crossfades with the old. Volumes are LHAudio units, 0 to
/// 127, and change once per tick of the music thread.
class MusicPlayer
{
public:
	static constexpr size_t k_ChannelCount = 6;
	static constexpr auto k_Tick = std::chrono::milliseconds(120);
	static constexpr uint32_t k_QueuedChunks = 4;
	static constexpr int32_t k_MaxVolume = 127;
	static constexpr int32_t k_FadeInStep = 4;
	static constexpr int32_t k_FadeOutStep = 3;
	static constexpr int k_NoChannel = -1;

	struct Channel
	{
		bool active {false};
		std::shared_ptr<const MusicBank> bank;
		int32_t groupId {0};
		/// 0 to 127
		int32_t targetVolume {0};
		int32_t volume {0};
		bool fadeIn {false};
		int32_t loops {0};
		bool sync {false};
		int32_t pitchPercent {100};
		std::function<void()> onFinished;
		MusicBackend::Stream stream {MusicBackend::k_InvalidStream};
		/// 0-based chunks queued on the stream, oldest first
		std::deque<uint32_t> queued;
		/// 0-based chunk to queue next
		uint32_t nextChunk {0};
		/// The bank has no more chunks to queue
		bool ending {false};
		/// 1-based chunk being heard
		uint32_t playingChunk {1};
	};

	explicit MusicPlayer(MusicBackend& backend);
	~MusicPlayer();
	MusicPlayer(const MusicPlayer&) = delete;
	MusicPlayer& operator=(const MusicPlayer&) = delete;

	/// LHMusicPlay: plays a bank, or retargets the channel already playing it. Without a bank, fades all music out.
	bool Play(const MusicPlayOptions& options);
	/// LHMusicStop: fades every channel out, or stops them at once
	void Stop(bool fadeOut);
	/// LHMusicIsActive
	[[nodiscard]] bool IsActive() const;
	/// Keeps the streams fed and changes volumes once per tick
	void Update(std::chrono::microseconds dt);

	[[nodiscard]] const std::array<Channel, k_ChannelCount>& GetChannels() const { return _channels; }
	[[nodiscard]] int GetCurrentChannel() const { return _current; }

private:
	void Start(size_t index, uint32_t startChunk);
	void Feed(Channel& channel);
	void Release(size_t index);
	void VolumeTick();
	void ApplyVolume(const Channel& channel);

	MusicBackend& _backend;
	std::array<Channel, k_ChannelCount> _channels;
	int _current {k_NoChannel};
	std::chrono::microseconds _tickTime {0};
};

} // namespace openblack::audio
