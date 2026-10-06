/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "ECS/Components/Player.h"
#include "ECS/Registry.h"
#include "Locator.h"

// Enable this define because we use a custom locator
#define LOCATOR_IMPLEMENTATIONS
#include "ECS/Systems/Implementations/AlignmentSystem.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace std::chrono_literals;

class Alignment: public ::testing::Test
{
protected:
	void SetUp() override
	{
		Locator::entitiesRegistry::emplace<Registry>();
		auto& registry = Locator::entitiesRegistry::value();
		registry.Assign<components::Player>(registry.Create(), PlayerNames::PLAYER_ONE);
		registry.Assign<components::Player>(registry.Create(), PlayerNames::PLAYER_TWO);
	}
	void TearDown() override { Locator::entitiesRegistry::reset(); }

	systems::AlignmentSystem _alignment;
};

TEST_F(Alignment, StartsNeutral)
{
	EXPECT_EQ(_alignment.GetPlayerAlignment(PlayerNames::PLAYER_ONE), 0.0f);
	EXPECT_EQ(_alignment.GetCameraAlignment(), 0.0f);
	EXPECT_EQ(_alignment.GetSkyAlignment(), 0.0f);
}

TEST_F(Alignment, PlayersAreHeldBetweenEvilAndGood)
{
	_alignment.SetPlayerAlignment(PlayerNames::PLAYER_ONE, 3.0f);
	EXPECT_EQ(_alignment.GetPlayerAlignment(PlayerNames::PLAYER_ONE), 1.0f);
	_alignment.AddPlayerAlignment(PlayerNames::PLAYER_ONE, -0.5f);
	EXPECT_EQ(_alignment.GetPlayerAlignment(PlayerNames::PLAYER_ONE), 0.5f);
	_alignment.AddPlayerAlignment(PlayerNames::PLAYER_ONE, -4.0f);
	EXPECT_EQ(_alignment.GetPlayerAlignment(PlayerNames::PLAYER_ONE), -1.0f);
	EXPECT_EQ(_alignment.GetPlayerAlignment(PlayerNames::PLAYER_TWO), 0.0f);
}

TEST_F(Alignment, PlayersNotInTheGameHaveNone)
{
	_alignment.SetPlayerAlignment(PlayerNames::PLAYER_SIX, 1.0f);
	EXPECT_EQ(_alignment.GetPlayerAlignment(PlayerNames::PLAYER_SIX), 0.0f);
}

TEST_F(Alignment, TheCameraTakesItsPlayersAtTheTurn)
{
	_alignment.SetPlayerAlignment(PlayerNames::PLAYER_ONE, -0.75f);
	EXPECT_EQ(_alignment.GetCameraAlignment(), 0.0f);
	_alignment.UpdateTurn();
	EXPECT_FLOAT_EQ(_alignment.GetCameraAlignment(), -0.75f);
}

TEST_F(Alignment, TheSkyTurnsAWholeASecondOfGameTime)
{
	_alignment.SetPlayerAlignment(PlayerNames::PLAYER_ONE, -1.0f);
	_alignment.UpdateTurn();
	_alignment.Update(500ms);
	EXPECT_FLOAT_EQ(_alignment.GetSkyAlignment(), -0.5f);
	// Not while paused
	_alignment.Update(0ms);
	EXPECT_FLOAT_EQ(_alignment.GetSkyAlignment(), -0.5f);
	_alignment.Update(2000ms);
	EXPECT_FLOAT_EQ(_alignment.GetSkyAlignment(), -1.0f);
	// And back the other way
	_alignment.SetPlayerAlignment(PlayerNames::PLAYER_ONE, 1.0f);
	_alignment.UpdateTurn();
	_alignment.Update(1500ms);
	EXPECT_FLOAT_EQ(_alignment.GetSkyAlignment(), 0.5f);
}
