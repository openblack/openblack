/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "Creature/CreatureSpellMind.h"

using namespace openblack;
using namespace openblack::creature_spell_mind;
using openblack::creature_desires::Desire;

namespace
{
creature_desires::Desires Some()
{
	creature_desires::Desires desires;
	for (size_t i = 0; i < creature_desires::k_DesireCount; ++i)
	{
		auto& state = desires.desires.at(i);
		state.activated = true;
		state.max = 1.0f;
		state.value = 0.2f + 0.01f * static_cast<float>(i);
	}
	return desires;
}
} // namespace

constexpr float k_TurnsPerSecond = 10.0f;
constexpr uint32_t k_HeldTurns = 200000;

TEST(CreatureSpellMind, ASpellsDesireIsWantedAboveAllAndMostOthersHeldDown)
{
	auto desires = Some();
	const auto cheat = SetCheatDominant(desires, Desire::Anger, true, k_TurnsPerSecond);
	EXPECT_EQ(cheat.desire, Desire::Anger);
	EXPECT_GE(desires[Desire::Anger].value, desires[Desire::Anger].max);
	EXPECT_EQ(desires[Desire::Anger].suppressedTurns, 0u);
	EXPECT_EQ(desires[Desire::Curiosity].suppressedTurns, k_HeldTurns);
	// The body's needs only because all are to be held down
	EXPECT_EQ(desires[Desire::Hunger].suppressedTurns, k_HeldTurns);
	// Never these
	for (const auto never : {Desire::IdleWithPlayer, Desire::RestoreHealth, Desire::BeFriends, Desire::ManifestState,
	                         Desire::Rest, Desire::PlayWithPlayer, Desire::HangAroundAtHome, Desire::LookAround})
	{
		EXPECT_EQ(desires[never].suppressedTurns, 0u);
	}
	EXPECT_FALSE(HeldDownByCheat(Desire::Water, false));
	EXPECT_TRUE(HeldDownByCheat(Desire::Water, true));
}

TEST(CreatureSpellMind, CompassionAlsoWantsToMakeFriendsAboveAll)
{
	auto desires = Some();
	(void)SetCheatDominant(desires, Desire::Compassion, true, k_TurnsPerSecond);
	EXPECT_EQ(desires[Desire::BeFriends].suppressedTurns, 0u);
	EXPECT_GE(desires[Desire::BeFriends].value, desires[Desire::BeFriends].max);
	EXPECT_EQ(desires[Desire::Anger].suppressedTurns, k_HeldTurns);
}

TEST(CreatureSpellMind, TheCheatLastsItsTimeThenLetsEverythingGo)
{
	auto desires = Some();
	auto cheat = SetCheatDominant(desires, Desire::Anger, true, k_TurnsPerSecond);
	desires[Desire::Anger].suppressedTurns = 5;
	EXPECT_TRUE(StepCheat(desires, cheat, k_TurnsPerSecond));
	// Its own desire is let go each turn
	EXPECT_EQ(desires[Desire::Anger].suppressedTurns, 0u);
	cheat.turns = static_cast<uint32_t>(k_CheatSeconds * k_TurnsPerSecond) + 10;
	EXPECT_FALSE(StepCheat(desires, cheat, k_TurnsPerSecond));
	EXPECT_EQ(desires[Desire::Curiosity].suppressedTurns, 0u);
}

TEST(CreatureSpellMind, WearingOffItIsWantedLeast)
{
	auto desires = Some();
	(void)SetCheatDominant(desires, Desire::Scratch, true, k_TurnsPerSecond);
	MakeLeastDominant(desires, Desire::Scratch);
	EXPECT_NEAR(desires[Desire::Scratch].value, 0.2f / k_LeastDominantFactor, 1e-5f);
	ClearCheatDominance(desires);
	for (const auto& state : desires.desires)
	{
		EXPECT_EQ(state.suppressedTurns, 0u);
	}
}
