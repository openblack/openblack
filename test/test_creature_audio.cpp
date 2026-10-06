/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <array>
#include <map>
#include <vector>

#include <gtest/gtest.h>

#include "Creature/CreatureAudio.h"

using namespace openblack;
using namespace openblack::creature_audio;
using audio::SoundAction;
using audio::SoundSize;
using audio::SoundSurface;

namespace
{
constexpr SoundEvent Step(int32_t timeMs)
{
	return {.kind = EventKind::Generic, .timeMs = timeMs, .action = SoundAction::FootstepNormal, .mode = 0};
}
constexpr SoundEvent Roar(int32_t timeMs)
{
	return {.kind = EventKind::Voice, .timeMs = timeMs, .action = SoundAction::RoarShort, .mode = 0};
}

/// A walk of two steps over 2000 ms
constexpr std::array k_Walk {Step(133), Step(1366)};

std::vector<int32_t> Times(const std::vector<FiredEvent>& fired)
{
	std::vector<int32_t> times;
	for (const auto& event : fired)
	{
		times.push_back(event.event.timeMs);
	}
	return times;
}
} // namespace

TEST(CreatureAudio, EventsFireOnceAcrossFrames)
{
	// Played in 16 ms frames over a whole cycle, each step sounds once
	std::vector<int32_t> heard;
	for (float time = 0.0f; time < 2000.0f; time += 16.0f)
	{
		const auto fired = EventsPassed(k_Walk, time, time + 16.0f, 2000, true);
		for (const auto t : Times(fired))
		{
			heard.push_back(t);
		}
	}
	EXPECT_EQ(heard, (std::vector<int32_t> {133, 1366}));
}

TEST(CreatureAudio, StretchIsHalfOpen)
{
	EXPECT_EQ(Times(EventsPassed(k_Walk, 133.0f, 140.0f, 2000, true)), (std::vector<int32_t> {133}));
	EXPECT_TRUE(EventsPassed(k_Walk, 120.0f, 133.0f, 2000, true).empty());
	EXPECT_TRUE(EventsPassed(k_Walk, 500.0f, 500.0f, 2000, true).empty());
}

TEST(CreatureAudio, FractionWithinTheFrame)
{
	const auto fired = EventsPassed(k_Walk, 100.0f, 200.0f, 2000, true);
	ASSERT_EQ(fired.size(), 1u);
	EXPECT_FLOAT_EQ(fired[0].fraction, 0.33f);
}

TEST(CreatureAudio, LoopWrapsWithoutDoubleFiring)
{
	// From near the end round to past the first step: the second step, then the first, in order
	const auto fired = EventsPassed(k_Walk, 1300.0f, 200.0f, 2000, true);
	EXPECT_EQ(Times(fired), (std::vector<int32_t> {1366, 133}));
	EXPECT_LT(fired[0].fraction, fired[1].fraction);
	EXPECT_FLOAT_EQ(fired[1].fraction, (700.0f + 133.0f) / 900.0f);
	// Then on from there, nothing again until the next step
	EXPECT_TRUE(EventsPassed(k_Walk, 200.0f, 1300.0f, 2000, true).empty());
}

TEST(CreatureAudio, EventAtTheStartSoundsOnWrap)
{
	const std::array events {Step(0)};
	EXPECT_EQ(EventsPassed(events, 1990.0f, 10.0f, 2000, true).size(), 1u);
	EXPECT_TRUE(EventsPassed(events, 10.0f, 1990.0f, 2000, true).empty());
}

TEST(CreatureAudio, OncePlayedStopsAtItsEnd)
{
	const std::array events {Roar(500), Roar(2500)};
	EXPECT_EQ(Times(EventsPassed(events, 400.0f, 3000.0f, 1000, false)), (std::vector<int32_t> {500}));
	// Its time going back means it started again
	EXPECT_EQ(Times(EventsPassed(events, 900.0f, 600.0f, 1000, false)), (std::vector<int32_t> {500}));
}

TEST(CreatureAudio, MirroredAnimationPassesTheSameEvents)
{
	// Mirroring plays an animation left to right at the same times: the layer's state has no side
	const auto infoOf = [](size_t) -> std::optional<AnimationInfo> {
		return AnimationInfo {.events = k_Walk, .durationMs = 2000, .looping = true};
	};
	const auto forward = LayerEvents(Played {1, 100.0f}, Played {1, 1400.0f}, true, infoOf);
	EXPECT_EQ(Times(forward), (std::vector<int32_t> {133, 1366}));
}

TEST(CreatureAudio, LayerChangingAnimation)
{
	const std::array action {Roar(100), Roar(900)};
	const std::map<size_t, AnimationInfo> infos {
	    {1, {.events = k_Walk, .durationMs = 2000, .looping = true}},
	    {2, {.events = action, .durationMs = 1000, .looping = false}},
	};
	const auto infoOf = [&infos](size_t index) -> std::optional<AnimationInfo> {
		const auto found = infos.find(index);
		return found != infos.end() ? std::optional(found->second) : std::nullopt;
	};
	// A new action sounds from its start
	EXPECT_EQ(Times(LayerEvents(std::nullopt, Played {2, 150.0f}, true, infoOf)), (std::vector<int32_t> {100}));
	// An action that ended part way through the last frame sounds the rest of it
	EXPECT_EQ(Times(LayerEvents(Played {2, 850.0f}, std::nullopt, true, infoOf)), (std::vector<int32_t> {900}));
	// A looping one left part way does not
	EXPECT_TRUE(LayerEvents(Played {1, 1300.0f}, std::nullopt, true, infoOf).empty());
	// One brought in part way sounds nothing in its first frame
	EXPECT_TRUE(LayerEvents(std::nullopt, Played {1, 1400.0f}, false, infoOf).empty());
	// Nor does an animation the species lacks
	EXPECT_TRUE(LayerEvents(std::nullopt, Played {7, 1400.0f}, true, infoOf).empty());
}

TEST(CreatureAudio, FaceSoundsOnlyItsFirstTimeThrough)
{
	const std::array growl {Roar(132)};
	const auto infoOf = [&growl](size_t) -> std::optional<AnimationInfo> {
		return AnimationInfo {.events = growl, .durationMs = 240, .looping = true};
	};
	bool looped = false;
	std::vector<int32_t> heard;
	std::optional<Played> previous;
	// Pulled, then looping round three times in 40 ms frames
	for (int frame = 0; frame < 20; ++frame)
	{
		const Played current {18, static_cast<float>((frame * 40) % 240)};
		for (const auto t : Times(FaceEvents(previous, current, looped, infoOf)))
		{
			heard.push_back(t);
		}
		previous = current;
	}
	EXPECT_EQ(heard, (std::vector<int32_t> {132}));
	EXPECT_TRUE(looped);
	// Another face, then this one again, sounds again
	EXPECT_TRUE(FaceEvents(previous, Played {19, 0.0f}, looped, infoOf).empty());
	EXPECT_FALSE(looped);
	EXPECT_EQ(Times(FaceEvents(Played {19, 0.0f}, Played {18, 150.0f}, looped, infoOf)), (std::vector<int32_t> {132}));
}

TEST(CreatureAudio, SoundingSlotIsTheHeaviest)
{
	EXPECT_EQ(SoundingSlot(std::array {0.0f, 0.0f}), std::nullopt);
	EXPECT_EQ(SoundingSlot(std::array {1.0f, 0.0f}), 0u);
	EXPECT_EQ(SoundingSlot(std::array {0.4f, 0.6f}), 1u);
	EXPECT_EQ(SoundingSlot(std::array {0.0f, 0.2f}), 1u);
}

TEST(CreatureAudio, QueueHoldsSixteenInOrder)
{
	std::vector<FiredEvent> queue;
	for (int i = 0; i < 20; ++i)
	{
		Enqueue(queue, {.fraction = static_cast<float>(20 - i) / 20.0f, .event = Step(i)});
	}
	ASSERT_EQ(queue.size(), k_MaxQueued);
	// The first sixteen asked for, in order of fraction; the last four were dropped
	EXPECT_EQ(queue.front().event.timeMs, 15);
	EXPECT_EQ(queue.back().event.timeMs, 0);
}

TEST(CreatureAudio, SizeKey)
{
	EXPECT_EQ(SizeKey(4.0f), SoundSize::Large);
	EXPECT_EQ(SizeKey(1.4f), SoundSize::Large);
	EXPECT_EQ(SizeKey(1.33f), SoundSize::Medium);
	EXPECT_EQ(SizeKey(1.0f), SoundSize::Medium);
	EXPECT_EQ(SizeKey(0.67f), SoundSize::Medium);
	EXPECT_EQ(SizeKey(0.66f), SoundSize::Small);
	EXPECT_EQ(SizeKey(0.05f), SoundSize::Small);
}

TEST(CreatureAudio, SurfaceKey)
{
	EXPECT_EQ(SurfaceKey(std::nullopt), SoundSurface::DeepWater);
	EXPECT_EQ(SurfaceKey(Ground {.water = true, .materialSurface = 1}), SoundSurface::ShallowWater);
	EXPECT_EQ(SurfaceKey(Ground {.water = false, .materialSurface = 1}), SoundSurface::Grass);
	EXPECT_EQ(SurfaceKey(Ground {.water = false, .materialSurface = 8}), SoundSurface::LooseFoliage);
	EXPECT_EQ(SurfaceKey(Ground {.water = false, .materialSurface = 0}), SoundSurface::Hard);
	EXPECT_EQ(SurfaceKey(Ground {.water = false, .materialSurface = 9}), SoundSurface::Hard);
}

TEST(CreatureAudio, TerrainMaterial)
{
	EXPECT_EQ(TerrainMaterial(18, 0.0f), 18);
	EXPECT_EQ(TerrainMaterial(18, 27.0f), k_SnowMaterial);
	EXPECT_EQ(TerrainMaterial(18, 26.9f), 18);
	EXPECT_EQ(TerrainMaterial(0, 0.0f), 1);
	EXPECT_EQ(TerrainMaterial(std::nullopt, 0.0f), 1);
}

TEST(CreatureAudio, KeysInBankOrder)
{
	const auto keys = Keys(1.0f, 0.0f, 5, SoundSurface::Grass, SoundAction::FootstepNormal);
	EXPECT_EQ(keys.ToArray(), (std::array<int32_t, 5> {2, 2, 5, 1, 4}));
}

TEST(CreatureAudio, VoiceBank)
{
	EXPECT_EQ(VoiceBank("btiger", CreatureType::Lion), "btiger.sad");
	EXPECT_EQ(VoiceBank("BApe", CreatureType::Chimp), "bape.sad");
	EXPECT_EQ(VoiceBank("", CreatureType::Lion), "btiger.sad");
	EXPECT_EQ(VoiceBank("", CreatureType::Zebra), "bhorse.sad");
	EXPECT_EQ(VoiceBank("", CreatureType::GiantApe), "bape.sad");
	EXPECT_EQ(VoiceBank("", CreatureType::Ogre), "bgreek.sad");
	EXPECT_EQ(VoiceBank("", CreatureType::Unknown), "");
}

TEST(CreatureAudio, Gate)
{
	const Gate open {
	    .muted = false, .insideTemple = false, .wideScreen = false, .localPlayersCreature = false, .otherVoicesEnabled = false};
	// Another player's creature steps but does not roar, unless a script lets it
	EXPECT_TRUE(IsHeard(EventKind::Generic, open));
	EXPECT_FALSE(IsHeard(EventKind::Voice, open));
	auto gate = open;
	gate.otherVoicesEnabled = true;
	EXPECT_TRUE(IsHeard(EventKind::Voice, gate));
	gate = open;
	gate.localPlayersCreature = true;
	EXPECT_TRUE(IsHeard(EventKind::Voice, gate));
	// Silent in cut scenes, inside the temple and when muted
	for (auto silence : {&Gate::muted, &Gate::insideTemple, &Gate::wideScreen})
	{
		gate = open;
		gate.localPlayersCreature = true;
		gate.*silence = true;
		EXPECT_FALSE(IsHeard(EventKind::Generic, gate));
		EXPECT_FALSE(IsHeard(EventKind::Voice, gate));
	}
	EXPECT_FALSE(IsHeard(EventKind::HairGroup, open));
}
