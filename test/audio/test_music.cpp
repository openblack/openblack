/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// LHAudioDLL's music player against a backend that plays a chunk per Advance()

#include <chrono>
#include <map>
#include <memory>
#include <vector>

#include <Audio/GameMusic.h>
#include <Audio/MusicPlayer.h>
#include <gtest/gtest.h>

using namespace openblack::audio;

namespace
{
class FakeMusicBackend final: public MusicBackend
{
public:
	struct StreamState
	{
		std::vector<uint32_t> queued;
		uint32_t played {0};
		uint32_t position {0};
		uint32_t firstSkip {0};
		bool playing {false};
		float volume {0.0f};
	};

	Stream CreateStream([[maybe_unused]] int32_t pitchPercent) override
	{
		streams[next] = {};
		return next++;
	}
	bool QueueChunk(Stream stream, [[maybe_unused]] const MusicBank& bank, uint32_t chunk, uint32_t skipFrames) override
	{
		auto& state = streams.at(stream);
		if (state.queued.empty() && state.played == 0)
		{
			state.firstSkip = skipFrames;
		}
		state.queued.push_back(chunk);
		heard.push_back(chunk);
		return true;
	}
	uint32_t ReleasePlayedChunks(Stream stream) override
	{
		auto& state = streams.at(stream);
		const auto played = state.played;
		state.queued.erase(state.queued.begin(), state.queued.begin() + played);
		state.played = 0;
		if (state.queued.empty())
		{
			state.playing = false;
		}
		return played;
	}
	[[nodiscard]] uint32_t GetPlayPosition(Stream stream) const override { return streams.at(stream).position; }
	void Play(Stream stream) override { streams.at(stream).playing = true; }
	[[nodiscard]] bool IsPlaying(Stream stream) const override
	{
		return streams.contains(stream) && streams.at(stream).playing;
	}
	void SetVolume(Stream stream, float volume) override { streams.at(stream).volume = volume; }
	void Destroy(Stream stream) override { streams.erase(stream); }

	/// Every stream finishes the chunk it is playing
	void Advance()
	{
		for (auto& [stream, state] : streams)
		{
			if (state.playing && state.played < state.queued.size())
			{
				++state.played;
			}
		}
	}

	std::map<Stream, StreamState> streams;
	std::vector<uint32_t> heard;
	Stream next {1};
};

std::shared_ptr<MusicBank> MakeBank(uint32_t chunks, int32_t group)
{
	auto bank = std::make_shared<MusicBank>();
	bank->groupId = group;
	bank->chunks.resize(chunks, std::vector<uint8_t>(1));
	return bank;
}

constexpr auto k_Frame = std::chrono::microseconds(16'000);
} // namespace

TEST(MusicPlayer, PlaysEveryChunkInOrderThenTheLoopsThenFinishes)
{
	FakeMusicBackend backend;
	MusicPlayer music(backend);
	bool finished = false;
	music.Play({.bank = MakeBank(6, 1), .loops = 1, .onFinished = [&finished]() { finished = true; }});
	for (int i = 0; i < 40 && !finished; ++i)
	{
		backend.Advance();
		music.Update(k_Frame);
	}
	EXPECT_TRUE(finished);
	EXPECT_EQ(backend.heard, (std::vector<uint32_t> {0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5}));
	EXPECT_FALSE(music.IsActive());
}

TEST(MusicPlayer, KeepsFourChunksQueued)
{
	FakeMusicBackend backend;
	MusicPlayer music(backend);
	music.Play({.bank = MakeBank(20, 1)});
	EXPECT_EQ(backend.streams.begin()->second.queued.size(), MusicPlayer::k_QueuedChunks);
	backend.Advance();
	music.Update(k_Frame);
	EXPECT_EQ(backend.streams.begin()->second.queued.size(), MusicPlayer::k_QueuedChunks);
	EXPECT_EQ(music.GetChannels()[0].playingChunk, 2u);
}

TEST(MusicPlayer, NewMusicFadesInWhileTheOldFadesOut)
{
	FakeMusicBackend backend;
	MusicPlayer music(backend);
	const auto first = MakeBank(500, 1);
	const auto second = MakeBank(500, 2);
	music.Play({.bank = first, .volume = 80});
	EXPECT_EQ(music.GetChannels()[0].volume, 80);

	music.Play({.bank = second, .volume = 80, .fadeIn = true});
	EXPECT_EQ(music.GetCurrentChannel(), 1);
	EXPECT_EQ(music.GetChannels()[1].volume, 0);

	music.Update(MusicPlayer::k_Tick);
	EXPECT_EQ(music.GetChannels()[0].volume, 80 - MusicPlayer::k_FadeOutStep);
	EXPECT_EQ(music.GetChannels()[1].volume, MusicPlayer::k_FadeInStep);

	// The old music is released once silent, 80 / 3 ticks on
	for (int i = 0; i < 30; ++i)
	{
		music.Update(MusicPlayer::k_Tick);
	}
	EXPECT_FALSE(music.GetChannels()[0].active);
	EXPECT_EQ(music.GetChannels()[1].volume, 80);
}

TEST(MusicPlayer, SyncedMusicStartsWhereItsGroupHasGotTo)
{
	FakeMusicBackend backend;
	MusicPlayer music(backend);
	music.Play({.bank = MakeBank(100, 1)});
	for (int i = 0; i < 7; ++i)
	{
		backend.Advance();
		music.Update(k_Frame);
	}
	backend.streams.begin()->second.position = 1234;
	music.Play({.bank = MakeBank(100, 1), .startChunk = 50, .sync = true, .fadeIn = true});
	EXPECT_EQ(music.GetChannels()[1].playingChunk, music.GetChannels()[0].playingChunk);
	EXPECT_EQ(backend.streams.rbegin()->second.firstSkip, 1234u);
}

TEST(MusicPlayer, PlayingTheSameBankAgainRetargetsIt)
{
	FakeMusicBackend backend;
	MusicPlayer music(backend);
	const auto bank = MakeBank(100, 4);
	music.Play({.bank = bank, .volume = 127});
	music.Stop(true);
	music.Update(MusicPlayer::k_Tick);
	EXPECT_EQ(music.GetChannels()[0].volume, 127 - MusicPlayer::k_FadeOutStep);
	music.Play({.bank = bank, .volume = 127, .fadeIn = true});
	music.Update(MusicPlayer::k_Tick);
	// Back up to its target, which it does not go past
	EXPECT_EQ(music.GetChannels()[0].volume, 127);
	EXPECT_FALSE(music.GetChannels()[1].active);
}

TEST(GameMusic, AlignmentPicksEvilNeutralOrGood)
{
	EXPECT_EQ(GameMusic::GetAlignmentIndex(-1.0f), 0);
	EXPECT_EQ(GameMusic::GetAlignmentIndex(0.0f), 1);
	EXPECT_EQ(GameMusic::GetAlignmentIndex(1.0f), 2);
}

TEST(GameMusic, TownsNearTheCameraPlayTheirTribesMusic)
{
	GameMusic music;
	GameMusic::TurnInputs inputs {
	    .turn = 100,
	    .camera = {1000.0f, 100.0f, 1000.0f},
	    .groundHeight = 0.0f,
	    .inCitadel = false,
	    .alignment = 0.0f,
	    .towns = {{.position = {1200.0f, 0.0f, 1000.0f}, .tribe = 5, .id = 1},
	              {.position = {560.0f, 0.0f, 1000.0f}, .tribe = 2, .id = 2}},
	};
	EXPECT_EQ(music.SelectLandType(inputs), MusicType::EgyptianTownNeutral);
	// Another town is nearer but not within 300: the town heard last carries on while within 400
	inputs.camera.x = 870.0f;
	EXPECT_EQ(music.SelectLandType(inputs), MusicType::EgyptianTownNeutral);
	// Within 300 of the other town, its music takes over
	inputs.camera.x = 820.0f;
	EXPECT_EQ(music.SelectLandType(inputs), MusicType::AztecTownNeutral);
	// Away from the towns, and high above the land, the player's alignment music plays
	inputs.camera.x = 2000.0f;
	EXPECT_EQ(music.SelectLandType(inputs), MusicType::GenericNeutral);
	inputs.camera = {1000.0f, 500.0f, 1000.0f};
	EXPECT_EQ(music.SelectLandType(inputs), MusicType::GenericNeutral);
}
