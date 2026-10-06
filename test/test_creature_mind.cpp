/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cmath>

#include <array>
#include <numbers>
#include <optional>
#include <vector>

#include <gtest/gtest.h>

#include "Creature/CreatureDesires.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLook.h"

using namespace openblack;
using creature_desires::Desire;
using creature_desires::Desires;
namespace animations = creature_layers::animations;

namespace
{
constexpr float k_Tolerance = 1e-4f;
constexpr float k_Turn = 0.1f;
constexpr float k_TurnsPerSecond = 10.0f;

/// Always draws the same number, less than the range
creature_mind::Random Always(uint32_t value)
{
	return [value](uint32_t range) { return range == 0 ? 0u : std::min(value, range - 1); };
}

/// A body that plays each action it is told to for some turns
struct FakeBody
{
	int turnsLeft {0};
	bool looping {false};
	std::vector<size_t> played;
	int sits {0};

	[[nodiscard]] creature_mind::Senses Senses() const
	{
		return {.seconds = k_Turn,
		        .bodyBusy = turnsLeft > 0 || looping,
		        .bodyLooping = looping,
		        .strongest = std::nullopt,
		        .feedbackSeconds = std::nullopt,
		        .feedbackWasStroke = false};
	}

	void Obey(const creature_mind::Commands& commands, int actionTurns)
	{
		if (turnsLeft > 0)
		{
			--turnsLeft;
		}
		if (commands.endSit)
		{
			looping = false;
		}
		if (commands.playOnce.has_value() && turnsLeft == 0 && !looping)
		{
			played.push_back(*commands.playOnce);
			turnsLeft = actionTurns;
		}
		if (commands.startSequence && turnsLeft == 0)
		{
			looping = true;
			++sits;
		}
	}
};

Desires OneDesire(Desire desire, float value, float threshold, float increaseSeconds, float max = 1.0f)
{
	Desires desires;
	for (auto& state : desires.desires)
	{
		state.activated = false;
	}
	auto& state = desires[desire];
	state.activated = true;
	state.max = max;
	state.decay = 0.9f;
	state.increaseSeconds = increaseSeconds;
	state.sources.push_back({.type = 1, .value = value, .threshold = threshold, .multiplier = 1.0f});
	return desires;
}
} // namespace

TEST(CreatureDesires, TheSigmoidSteps)
{
	EXPECT_FLOAT_EQ(creature_desires::Sigmoid(0.5f, 0.0f), 0.0f);
	EXPECT_FLOAT_EQ(creature_desires::Sigmoid(1.0f, 0.9f), 0.0f);
	EXPECT_FLOAT_EQ(creature_desires::Sigmoid(0.5f, 0.5f), 0.5f);
	EXPECT_FLOAT_EQ(creature_desires::Sigmoid(0.0f, 1.0f), 1.0f);
	EXPECT_LT(creature_desires::Sigmoid(0.9f, 0.1f), 1e-6f);
}

TEST(CreatureDesires, ADesireGrowsByItsDriveOverItsIncreaseTime)
{
	// A source fully past its threshold drives the desire up by 1 in two seconds
	auto desires = OneDesire(Desire::Hunger, 1.0f, 0.0f, 2.0f);
	for (int i = 0; i < 10; ++i)
	{
		creature_desires::UpdateDesires(desires, k_TurnsPerSecond);
	}
	EXPECT_NEAR(desires[Desire::Hunger].value, 0.5f, k_Tolerance);
	EXPECT_NEAR(desires.sum, 0.5f, k_Tolerance);
	for (int i = 0; i < 20; ++i)
	{
		creature_desires::UpdateDesires(desires, k_TurnsPerSecond);
	}
	EXPECT_NEAR(desires[Desire::Hunger].value, 1.0f, k_Tolerance);
}

TEST(CreatureDesires, ADesireWithoutDriveFades)
{
	auto desires = OneDesire(Desire::Play, 0.0f, 0.5f, 5.0f);
	desires[Desire::Play].value = 1.0f;
	creature_desires::UpdateDesires(desires, k_TurnsPerSecond);
	EXPECT_NEAR(desires[Desire::Play].value, 0.9f, k_Tolerance);
}

TEST(CreatureDesires, ASuppressedDesireOnlyFades)
{
	auto desires = OneDesire(Desire::Anger, 1.0f, 0.0f, 1.0f);
	desires[Desire::Anger].value = 0.5f;
	creature_desires::Suppress(desires, Desire::Anger, 1.0f, k_TurnsPerSecond);
	for (int i = 0; i < 9; ++i)
	{
		creature_desires::UpdateDesires(desires, k_TurnsPerSecond);
	}
	const auto faded = desires[Desire::Anger].value;
	EXPECT_LT(faded, 0.5f);
	creature_desires::UpdateDesires(desires, k_TurnsPerSecond);
	EXPECT_NEAR(desires[Desire::Anger].value, faded + 0.1f, k_Tolerance);
	// A shorter suppression doesn't cut a longer one short
	creature_desires::Suppress(desires, Desire::Anger, 2.0f, k_TurnsPerSecond);
	creature_desires::Suppress(desires, Desire::Anger, 0.5f, k_TurnsPerSecond);
	EXPECT_EQ(desires[Desire::Anger].suppressedTurns, 20u);
}

TEST(CreatureDesires, SourcesReadTheirStateThenFade)
{
	auto desires = OneDesire(Desire::Tiredness, 0.4f, 0.5f, 2.0f);
	desires[Desire::Tiredness].sources.push_back({.type = 2, .value = 1.0f, .threshold = 0.5f, .multiplier = 0.5f});
	creature_desires::UpdateSources(desires, [](uint32_t type, const Desires&) -> std::optional<float> {
		return type == 1 ? std::optional(0.8f) : std::nullopt;
	});
	EXPECT_NEAR(desires[Desire::Tiredness].sources[0].value, 0.8f, k_Tolerance);
	EXPECT_NEAR(desires[Desire::Tiredness].sources[1].value, 0.5f, k_Tolerance);
	EXPECT_NEAR(*creature_desires::SourceValue(desires[Desire::Tiredness], 2), 0.5f, k_Tolerance);
	EXPECT_FALSE(creature_desires::SourceValue(desires[Desire::Tiredness], 3).has_value());
}

TEST(CreatureDesires, CreatedFromTheSetup)
{
	std::array<creature_desires::DesireSetup, creature_desires::k_DesireCount> setup {};
	setup.at(static_cast<size_t>(Desire::Hunger)) = {
	    .max = 2.0f,
	    .decayMin = 0.9f,
	    .decayMax = 1.0f,
	    .increaseSeconds = 20.0f,
	    .sources = {{.type = 14, .value = 0.0f, .threshold = 0.4f, .multiplier = 1.0f},
	                {.type = creature_desires::k_NoSource, .value = 0.0f, .threshold = 0.0f, .multiplier = 1.0f}},
	};
	const auto desires = creature_desires::Create(setup, [](float low, float high) { return (low + high) / 2.0f; });
	const auto& hunger = desires[Desire::Hunger];
	EXPECT_FLOAT_EQ(hunger.max, 2.0f);
	EXPECT_FLOAT_EQ(hunger.decay, 0.95f);
	EXPECT_FLOAT_EQ(hunger.increaseSeconds, 20.0f);
	ASSERT_EQ(hunger.sources.size(), 1u);
	EXPECT_EQ(hunger.sources[0].type, 14u);
}

TEST(CreatureDesires, GrowingUpBringsAndTakesDesires)
{
	Desires desires;
	desires[Desire::Fear].suppressedTurns = 50;
	const std::vector<creature_desires::PhaseDesires> phases {
	    {.add = {Desire::Fear, Desire::Tiredness}, .remove = {}},
	    {.add = {Desire::Hunger}, .remove = {Desire::Tiredness}},
	};
	creature_desires::ActivateForPhase(desires, phases, 0);
	EXPECT_TRUE(desires[Desire::Fear].activated);
	EXPECT_TRUE(desires[Desire::Tiredness].activated);
	EXPECT_FALSE(desires[Desire::Hunger].activated);
	EXPECT_EQ(desires[Desire::Fear].suppressedTurns, 0u);
	creature_desires::ActivateForPhase(desires, phases, 1);
	EXPECT_TRUE(desires[Desire::Hunger].activated);
	EXPECT_FALSE(desires[Desire::Tiredness].activated);
}

TEST(CreatureDesires, EachShowableDesireHasItsEmote)
{
	EXPECT_EQ(creature_desires::EmoteFor(Desire::Hunger), animations::k_Hungry);
	EXPECT_EQ(creature_desires::EmoteFor(Desire::AttractAttention), animations::k_FriendlyWave);
	EXPECT_EQ(creature_desires::EmoteFor(Desire::Tiredness), animations::k_Tired);
	EXPECT_EQ(creature_desires::EmoteFor(Desire::Impress), animations::k_Summon);
	EXPECT_EQ(creature_desires::EmoteFor(Desire::GetColder), animations::k_Hot);
	EXPECT_FALSE(creature_desires::EmoteFor(Desire::Wanderlust).has_value());
}

TEST(CreatureDesires, TheStrongestShowableDesire)
{
	Desires desires;
	desires[Desire::Wanderlust].value = 0.9f;
	desires[Desire::Hunger].value = 0.5f;
	desires[Desire::Play].value = 0.6f;
	desires[Desire::Play].activated = false;
	desires[Desire::Sadness].value = 0.3f;
	EXPECT_EQ(creature_desires::StrongestShowable(desires, 0.2f), Desire::Hunger);
	EXPECT_FALSE(creature_desires::StrongestShowable(desires, 0.55f).has_value());
}

TEST(CreatureIdleMind, BeingIdleIsWaitingThenYawningTwice)
{
	creature_mind::IdleMind mind;
	FakeBody body;
	// Draws of 1: waits of 1.001 seconds, faces from the grimace, mirrored actions, and no sitting
	const auto random = Always(1);
	std::vector<int> actionTurns;
	std::vector<creature_mind::Eyes> eyes;
	for (int turn = 0; turn < 200 && body.played.size() < 2; ++turn)
	{
		const auto commands = creature_mind::Think(mind, body.Senses(), random);
		if (turn == 0)
		{
			EXPECT_EQ(mind.activity, creature_mind::Activity::BeIdle);
			EXPECT_EQ(mind.agenda.size(), 4u);
			EXPECT_EQ(commands.face, std::optional<size_t>(animations::k_FirstFace + 1));
			EXPECT_TRUE(commands.lookAbout);
		}
		if (commands.playOnce.has_value())
		{
			actionTurns.push_back(turn);
			EXPECT_EQ(*commands.playOnce, animations::k_Tired);
			EXPECT_TRUE(commands.mirrored);
		}
		if (commands.eyes != creature_mind::Eyes::Unchanged)
		{
			eyes.push_back(commands.eyes);
		}
		body.Obey(commands, 30);
	}
	ASSERT_EQ(actionTurns.size(), 2u);
	// The wait starts on the first turn and lasts 11 turns more, then the yawn starts
	EXPECT_EQ(actionTurns[0], 12);
	// The yawn keeps the body busy 30 turns; the mind sees it free the turn after, starts the next wait on the one after
	// that, and yawns again 12 turns on
	EXPECT_EQ(actionTurns[1], 12 + 31 + 1 + 12);
	ASSERT_GE(eyes.size(), 3u);
	EXPECT_EQ(eyes[0], creature_mind::Eyes::Sleepy);
	EXPECT_EQ(eyes[1], creature_mind::Eyes::Normal);
	EXPECT_EQ(eyes[2], creature_mind::Eyes::Sleepy);
}

TEST(CreatureIdleMind, AFaceIsHeldForThreeSeconds)
{
	creature_mind::IdleMind mind;
	FakeBody body;
	std::optional<int> relaxed;
	for (int turn = 0; turn < 60 && !relaxed; ++turn)
	{
		const auto commands = creature_mind::Think(mind, body.Senses(), Always(1));
		if (turn > 0 && commands.face.has_value() && !commands.face->has_value())
		{
			relaxed = turn;
		}
		body.Obey(commands, 100);
	}
	// The yawn starts on turn 12 with a new face, which relaxes three seconds later, give or take a turn's rounding
	ASSERT_TRUE(relaxed.has_value());
	EXPECT_GE(*relaxed, 42);
	EXPECT_LE(*relaxed, 43);
}

TEST(CreatureIdleMind, NothingStartsWhileTheBodyIsBusy)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::Told,
	                    {{.kind = creature_mind::Step::Kind::Action, .seconds = 0.0f, .animation = 60, .sleepyEyes = false}});
	auto senses = FakeBody {}.Senses();
	senses.bodyBusy = true;
	for (int i = 0; i < 5; ++i)
	{
		EXPECT_FALSE(creature_mind::Think(mind, senses, Always(0)).playOnce.has_value());
	}
	senses.bodyBusy = false;
	EXPECT_EQ(creature_mind::Think(mind, senses, Always(0)).playOnce, 60u);
}

TEST(CreatureIdleMind, SometimesItSitsForAWhile)
{
	creature_mind::IdleMind mind;
	FakeBody body;
	// Draws of 0 choose to sit, for ten seconds
	std::optional<int> stood;
	for (int turn = 0; turn < 300 && !stood; ++turn)
	{
		const auto commands = creature_mind::Think(mind, body.Senses(), Always(0));
		if (turn == 0)
		{
			EXPECT_EQ(mind.activity, creature_mind::Activity::Sit);
			EXPECT_TRUE(commands.startSequence);
		}
		if (commands.endSit)
		{
			stood = turn;
		}
		body.Obey(commands, 1);
	}
	EXPECT_EQ(body.sits, 1);
	ASSERT_TRUE(stood.has_value());
	EXPECT_EQ(*stood, 100);
}

TEST(CreatureIdleMind, ItShowsItsStrongestDesireOnceAMinute)
{
	creature_mind::IdleMind mind;
	FakeBody body;
	std::vector<int> shown;
	for (int turn = 0; turn < 1300; ++turn)
	{
		auto senses = body.Senses();
		senses.strongest = Desire::Hunger;
		const auto commands = creature_mind::Think(mind, senses, Always(1));
		if (commands.playOnce == animations::k_Hungry)
		{
			shown.push_back(turn);
		}
		body.Obey(commands, 20);
	}
	ASSERT_GE(shown.size(), 2u);
	EXPECT_EQ(shown[0], 0);
	// Not again until a minute has passed, at the next choice after it
	EXPECT_GE(shown[1], 600);
	EXPECT_LT(shown[1], 700);
	EXPECT_EQ(mind.shown, Desire::Hunger);
}

TEST(CreatureIdleMind, AStrokeOrASlapIsShownFirst)
{
	creature_mind::IdleMind mind;
	auto senses = FakeBody {}.Senses();
	senses.strongest = Desire::Hunger;
	senses.feedbackSeconds = 4.0f;
	senses.feedbackWasStroke = true;
	EXPECT_EQ(creature_mind::Think(mind, senses, Always(1)).playOnce, animations::k_Happy);

	creature_mind::IdleMind slapped;
	senses.feedbackWasStroke = false;
	EXPECT_EQ(creature_mind::Think(slapped, senses, Always(1)).playOnce, animations::k_Sad);

	// Long after, the desire is shown instead
	creature_mind::IdleMind later;
	senses.feedbackSeconds = 20.0f;
	EXPECT_EQ(creature_mind::Think(later, senses, Always(1)).playOnce, animations::k_Hungry);
}

TEST(CreatureIdleMind, AnAbandonedSitEnds)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::BeIdle,
	                    {{.kind = creature_mind::Step::Kind::Wait, .seconds = 5.0f, .animation = 0, .sleepyEyes = false}});
	auto senses = FakeBody {}.Senses();
	senses.bodyBusy = true;
	senses.bodyLooping = true;
	EXPECT_TRUE(creature_mind::Think(mind, senses, Always(1)).endSit);
}

TEST(CreatureLook, InterestByKind)
{
	EXPECT_FLOAT_EQ(creature_look::InterestOf(creature_look::Interest::Creature), 0.85f);
	EXPECT_FLOAT_EQ(creature_look::InterestOf(creature_look::Interest::Villager), 0.5f);
	EXPECT_FLOAT_EQ(creature_look::InterestOf(creature_look::Interest::Tree), 0.3f);
	EXPECT_FLOAT_EQ(creature_look::InterestOf(creature_look::Interest::Fixed), 0.25f);
}

TEST(CreatureLook, BiggerCreaturesLookFurther)
{
	EXPECT_FLOAT_EQ(creature_look::LookRange(1.0f), 150.0f);
	EXPECT_FLOAT_EQ(creature_look::LookRange(0.5f), 130.0f);
	EXPECT_FLOAT_EQ(creature_look::LookRange(2.0f), 170.0f);
}

TEST(CreatureLook, NearerIsMoreInteresting)
{
	EXPECT_FLOAT_EQ(creature_look::DistanceFactor(10.0f, 100.0f), 1.0f);
	EXPECT_FLOAT_EQ(creature_look::DistanceFactor(49.0f, 100.0f), 1.0f);
	EXPECT_FLOAT_EQ(creature_look::DistanceFactor(75.0f, 100.0f), 0.25f);
}

TEST(CreatureLook, ItSeesWhatIsInFront)
{
	const creature_look::Viewer viewer {.position = {0.0f, 0.0f, 0.0f}, .ahead = {0.0f, 0.0f, -1.0f}, .size = 1.0f};
	EXPECT_TRUE(creature_look::CanSee(viewer, {20.0f, 0.0f, -50.0f}));
	EXPECT_FALSE(creature_look::CanSee(viewer, {50.0f, 0.0f, -20.0f}));
	EXPECT_FALSE(creature_look::CanSee(viewer, {0.0f, 0.0f, 50.0f}));
	EXPECT_FALSE(creature_look::CanSee(viewer, {0.0f, 0.0f, -200.0f}));
	// Anything right by it
	EXPECT_TRUE(creature_look::CanSee(viewer, {0.0f, 0.0f, 5.0f}));
}

TEST(CreatureLook, ItWatchesTheMostInterestingThing)
{
	const creature_look::Viewer viewer {.position = {0.0f, 0.0f, 0.0f}, .ahead = {0.0f, 0.0f, -1.0f}, .size = 1.0f};
	const std::array<creature_look::Candidate, 3> candidates {{
	    {.id = 1, .kind = creature_look::Interest::Tree, .point = {0.0f, 0.0f, -30.0f}},
	    {.id = 2, .kind = creature_look::Interest::Creature, .point = {10.0f, 0.0f, -40.0f}},
	    {.id = 3, .kind = creature_look::Interest::Villager, .point = {0.0f, 0.0f, 40.0f}},
	}};
	const auto target = creature_look::LookAbout({}, candidates, viewer, k_TurnsPerSecond);
	EXPECT_EQ(target.id, 2u);
	EXPECT_EQ(target.watchedTurns, 0u);
	const auto later = creature_look::LookAbout(target, candidates, viewer, k_TurnsPerSecond);
	EXPECT_EQ(later.id, 2u);
	EXPECT_EQ(later.watchedTurns, 1u);
}

TEST(CreatureLook, TheLongerItWatchesTheLessElseCatchesItsEye)
{
	const creature_look::Viewer viewer {.position = {0.0f, 0.0f, 0.0f}, .ahead = {0.0f, 0.0f, -1.0f}, .size = 1.0f};
	const std::array<creature_look::Candidate, 2> candidates {{
	    {.id = 1, .kind = creature_look::Interest::Tree, .point = {0.0f, 0.0f, -30.0f}},
	    {.id = 2, .kind = creature_look::Interest::Creature, .point = {10.0f, 0.0f, -40.0f}},
	}};
	// Watching the tree for a while: the creature, more than twice as interesting, still takes over at first
	creature_look::Target tree {
	    .id = 1, .kind = creature_look::Interest::Tree, .point = {0.0f, 0.0f, -30.0f}, .watchedTurns = 50};
	EXPECT_EQ(creature_look::LookAbout(tree, candidates, viewer, k_TurnsPerSecond).id, 2u);
	// After twenty seconds, nothing else does
	tree.watchedTurns = 200;
	EXPECT_EQ(creature_look::LookAbout(tree, candidates, viewer, k_TurnsPerSecond).id, 1u);
}

TEST(CreatureLook, WhatGoesOutOfSightIsDropped)
{
	const creature_look::Viewer viewer {.position = {0.0f, 0.0f, 0.0f}, .ahead = {0.0f, 0.0f, -1.0f}, .size = 1.0f};
	const std::array<creature_look::Candidate, 1> behind {{
	    {.id = 7, .kind = creature_look::Interest::Creature, .point = {0.0f, 0.0f, 60.0f}},
	}};
	const creature_look::Target watching {
	    .id = 7, .kind = creature_look::Interest::Creature, .point = {0.0f, 0.0f, -60.0f}, .watchedTurns = 3};
	EXPECT_FALSE(creature_look::LookAbout(watching, behind, viewer, k_TurnsPerSecond).id.has_value());
	EXPECT_FALSE(creature_look::LookAbout(watching, {}, viewer, k_TurnsPerSecond).id.has_value());
}

TEST(CreatureLook, WithNothingToSeeItLooksAhead)
{
	const creature_look::Viewer viewer {.position = {10.0f, 0.0f, 0.0f}, .ahead = {1.0f, 0.0f, 0.0f}, .size = 2.0f};
	const auto point = creature_look::PointAhead(viewer);
	EXPECT_NEAR(point.x, 60.0f, k_Tolerance);
	EXPECT_NEAR(point.y, 30.0f, k_Tolerance);
	EXPECT_NEAR(point.z, 0.0f, k_Tolerance);
}

TEST(CreatureIdleMind, HangingAroundWalksSomewhereNearbyThenSits)
{
	creature_mind::IdleMind mind;
	// Draws of 5: the last lot, which is hanging around, 5 degrees round and 25 units away
	const auto random = Always(5);
	auto senses = FakeBody {}.Senses();
	senses.position = {100.0f, 200.0f};
	const auto first = creature_mind::Think(mind, senses, random);
	EXPECT_EQ(mind.activity, creature_mind::Activity::HangAround);
	ASSERT_TRUE(first.move.has_value());
	EXPECT_EQ(first.move->kind, creature_mind::Movement::Kind::ToPoint);
	const auto angle = 5.0f * std::numbers::pi_v<float> / 180.0f;
	EXPECT_NEAR(first.move->point.x, 100.0f + (25.0f * std::cos(angle)), k_Tolerance);
	EXPECT_NEAR(first.move->point.y, 200.0f + (25.0f * std::sin(angle)), k_Tolerance);

	// On its way, it keeps going
	senses.moving = true;
	senses.bodyBusy = true;
	for (int i = 0; i < 10; ++i)
	{
		const auto commands = creature_mind::Think(mind, senses, random);
		EXPECT_FALSE(commands.move.has_value());
		EXPECT_FALSE(commands.startSequence);
		EXPECT_EQ(mind.step, 0u);
	}
	// Arrived, it sits down
	senses.moving = false;
	senses.bodyBusy = false;
	EXPECT_FALSE(creature_mind::Think(mind, senses, random).startSequence);
	EXPECT_EQ(mind.step, 1u);
	EXPECT_TRUE(creature_mind::Think(mind, senses, random).startSequence);
}

TEST(CreatureIdleMind, AFollowStepStopsWhenItsTimeIsUp)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::Told,
	                    {{.kind = creature_mind::Step::Kind::Move,
	                      .seconds = 1.0f,
	                      .animation = 0,
	                      .sleepyEyes = false,
	                      .movement = {.kind = creature_mind::Movement::Kind::Follow,
	                                   .point = {0.0f, 0.0f},
	                                   .object = 7u,
	                                   .run = false,
	                                   .minDistance = 0.0f,
	                                   .maxDistance = 10.0f}}});
	auto senses = FakeBody {}.Senses();
	const auto first = creature_mind::Think(mind, senses, Always(0));
	ASSERT_TRUE(first.move.has_value());
	EXPECT_EQ(first.move->object, 7u);
	senses.moving = true;
	senses.bodyBusy = true;
	int stoppedAt = -1;
	for (int turn = 1; turn < 30 && stoppedAt < 0; ++turn)
	{
		if (creature_mind::Think(mind, senses, Always(0)).stopMoving)
		{
			stoppedAt = turn;
		}
	}
	EXPECT_GE(stoppedAt, 10);
	EXPECT_LE(stoppedAt, 11);
}

TEST(CreatureNeedsMind, TheStrongestNeedWithTheMeansAtHandIsSeenTo)
{
	creature_mind::Wants wants {.hunger = 0.9f, .tiredness = 0.5f, .poo = 0.4f, .water = 0.6f};
	// Hungry with nothing to eat and thirsty with no water, it sleeps
	auto plan = creature_mind::ChooseNeed(wants, Always(1));
	ASSERT_TRUE(plan.has_value());
	EXPECT_EQ(plan->activity, creature_mind::Activity::Sleep);
	wants.waterSpot = {.shore = glm::vec2(10.0f, 20.0f), .water = glm::vec2(10.0f, 30.0f)};
	plan = creature_mind::ChooseNeed(wants, Always(1));
	ASSERT_TRUE(plan.has_value());
	EXPECT_EQ(plan->activity, creature_mind::Activity::Drink);
	wants.food = 42u;
	plan = creature_mind::ChooseNeed(wants, Always(1));
	ASSERT_TRUE(plan.has_value());
	EXPECT_EQ(plan->activity, creature_mind::Activity::Eat);
	// Nothing pressing, nothing to see to
	EXPECT_FALSE(creature_mind::ChooseNeed({.hunger = 0.2f, .tiredness = 0.1f, .poo = 0.29f, .water = 0.0f}, Always(1)));
}

TEST(CreatureNeedsMind, ANeedComesBeforeIdling)
{
	creature_mind::IdleMind mind;
	auto senses = FakeBody {}.Senses();
	senses.wants = {.hunger = 0.0f, .tiredness = 0.0f, .poo = 0.8f, .water = 0.0f};
	creature_mind::ChooseNext(mind, senses, Always(1));
	EXPECT_EQ(mind.activity, creature_mind::Activity::Poo);
}

TEST(CreatureNeedsMind, ItSleepsWithItsEyesClosedUntilRested)
{
	creature_mind::IdleMind mind;
	// Draws of 1: no yawn first
	creature_mind::Plan(mind, creature_mind::Activity::Sleep, creature_mind::Sleep(Always(1)));
	ASSERT_EQ(mind.agenda.size(), 2u);
	FakeBody body;
	auto commands = creature_mind::Think(mind, body.Senses(), Always(1));
	ASSERT_TRUE(commands.startSequence.has_value());
	EXPECT_EQ((*commands.startSequence)[1], animations::k_Sleep);
	EXPECT_EQ(commands.eyes, creature_mind::Eyes::Closed);
	EXPECT_TRUE(creature_mind::IsAsleep(mind));
	body.Obey(commands, 1);
	// However long it sleeps, it sleeps on until rested
	for (int turn = 0; turn < 1000; ++turn)
	{
		commands = creature_mind::Think(mind, body.Senses(), Always(1));
		EXPECT_FALSE(commands.endSit);
		EXPECT_FALSE(commands.lookAbout);
		body.Obey(commands, 1);
	}
	auto senses = body.Senses();
	senses.rested = true;
	commands = creature_mind::Think(mind, senses, Always(1));
	EXPECT_TRUE(commands.endSit);
	EXPECT_EQ(commands.effect, creature_mind::Effect::Slept);
	body.Obey(commands, 1);
	commands = creature_mind::Think(mind, body.Senses(), Always(1));
	EXPECT_EQ(commands.eyes, creature_mind::Eyes::Normal);
	EXPECT_FALSE(creature_mind::IsAsleep(mind));
	// Then a dazed look about with sleepy eyes
	commands = creature_mind::Think(mind, body.Senses(), Always(1));
	EXPECT_EQ(commands.playOnce, animations::k_Confused);
	EXPECT_EQ(commands.eyes, creature_mind::Eyes::Sleepy);
}

TEST(CreatureNeedsMind, APooTakesFourSecondsAndDropsAsItEnds)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::Poo, creature_mind::Poo(Always(1)));
	ASSERT_EQ(mind.agenda.size(), 1u);
	FakeBody body;
	std::optional<int> dropped;
	for (int turn = 0; turn < 100 && !dropped; ++turn)
	{
		const auto commands = creature_mind::Think(mind, body.Senses(), Always(1));
		if (turn == 0)
		{
			ASSERT_TRUE(commands.startSequence.has_value());
			EXPECT_EQ(*commands.startSequence,
			          (std::array<size_t, 3> {animations::k_StartPoo, animations::k_Poo, animations::k_EndPoo}));
		}
		if (commands.effect == creature_mind::Effect::Poo)
		{
			EXPECT_TRUE(commands.endSit);
			dropped = turn;
		}
		body.Obey(commands, 1);
	}
	// Four seconds of tenths, give or take a turn's rounding
	ASSERT_TRUE(dropped.has_value());
	EXPECT_GE(*dropped, 40);
	EXPECT_LE(*dropped, 41);
	// A third of the time it shows it needs one first
	EXPECT_EQ(creature_mind::Poo(Always(0)).front().animation, animations::k_NeedAPoo);
}

TEST(CreatureNeedsMind, ItPicksFoodUpAndEatsIt)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::Eat, creature_mind::Eat(7u));
	FakeBody body;
	auto senses = body.Senses();
	// Its hands go and pick the food up
	auto commands = creature_mind::Think(mind, senses, Always(1));
	ASSERT_TRUE(commands.object.has_value());
	EXPECT_EQ(commands.object->kind, creature_mind::ObjectOrder::Kind::PickUp);
	EXPECT_EQ(commands.object->object, 7u);
	senses.hands = creature_mind::HandsState::Busy;
	EXPECT_EQ(creature_mind::Think(mind, senses, Always(1)).effect, creature_mind::Effect::None);
	// Having it, it examines it, then eats it; eaten, the desire is satisfied
	senses.hands = creature_mind::HandsState::Done;
	static_cast<void>(creature_mind::Think(mind, senses, Always(1)));
	commands = creature_mind::Think(mind, senses, Always(1));
	ASSERT_TRUE(commands.object.has_value());
	EXPECT_EQ(commands.object->kind, creature_mind::ObjectOrder::Kind::Keep);
	static_cast<void>(creature_mind::Think(mind, senses, Always(1)));
	commands = creature_mind::Think(mind, senses, Always(1));
	ASSERT_TRUE(commands.object.has_value());
	EXPECT_EQ(commands.object->kind, creature_mind::ObjectOrder::Kind::Eat);
	commands = creature_mind::Think(mind, senses, Always(1));
	EXPECT_EQ(commands.effect, creature_mind::Effect::Eat);
}

TEST(CreatureNeedsMind, ItDrinksAtTheWatersEdge)
{
	const auto agenda = creature_mind::Drink({100.0f, 200.0f}, {100.0f, 230.0f});
	ASSERT_EQ(agenda.size(), 3u);
	EXPECT_EQ(agenda[0].movement.kind, creature_mind::Movement::Kind::ToPoint);
	EXPECT_EQ(agenda[0].movement.point, glm::vec2(100.0f, 200.0f));
	EXPECT_FLOAT_EQ(agenda[0].movement.maxDistance, creature_mind::k_DrinkReach);
	EXPECT_EQ(agenda[1].movement.kind, creature_mind::Movement::Kind::TurnToFace);
	EXPECT_EQ(agenda[1].movement.point, glm::vec2(100.0f, 230.0f));
	EXPECT_EQ(agenda[2].animation, animations::k_Drink);
	EXPECT_EQ(agenda[2].effect, creature_mind::Effect::Drink);
}

TEST(CreatureNeedsMind, FaintedItLiesStillThenComesRound)
{
	creature_mind::IdleMind mind;
	creature_mind::Plan(mind, creature_mind::Activity::Faint, creature_mind::Faint());
	FakeBody body;
	auto commands = creature_mind::Think(mind, body.Senses(), Always(1));
	ASSERT_TRUE(commands.startSequence.has_value());
	EXPECT_TRUE(commands.holdLoop);
	EXPECT_EQ((*commands.startSequence)[0], animations::k_Faint);
	EXPECT_EQ((*commands.startSequence)[2], animations::k_GetUp);
	EXPECT_TRUE(creature_mind::IsUnconscious(mind));
	body.Obey(commands, 1);
	std::optional<int> cameRound;
	for (int turn = 1; turn < 200 && !cameRound; ++turn)
	{
		commands = creature_mind::Think(mind, body.Senses(), Always(1));
		if (commands.effect == creature_mind::Effect::CameRound)
		{
			cameRound = turn;
		}
		body.Obey(commands, 1);
	}
	ASSERT_TRUE(cameRound.has_value());
	EXPECT_EQ(*cameRound, static_cast<int>(creature_mind::k_FaintSeconds * k_TurnsPerSecond));
}
