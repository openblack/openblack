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
#include <functional>
#include <optional>

// How fast a villager goes in a state, as the game works it out each time the villager's state changes, in the game's
// whole speed units (a 65536th of ten metres a turn).
//
// The state's speed from the villager's tables is scaled by its player: the land's speed balance, times the player's
// Indian tribal power (at least a tenth; 1 without a player), and for a town of a player other than the neutral one by
// how far the town believes in the player beyond the neutral belief (a tenth of it, or the whole belief when not
// beyond it) times the belief speed scale (the land's own, or else the story land's from the town tables, or the
// multiplayer one). Badly wounded it crawls at a random 0.4 to 0.6 of its fifth speed; wounded it walks at a random 0.5
// to 0.75 of its usual speed; in a town in a state of emergency it runs at a random 0.75 to 1.25 of its fleeing speed.
// Otherwise its state's speed is slowed by the wood and food it carries and quickened by its town's needs. A villager
// sped up by magic food goes faster still, except wounded.
//
// Pure, tested on made-up villagers.

namespace openblack::ecs::villager_speed
{

/// What the speed of a state comes from
struct Inputs
{
	/// The villager's speeds from its tables, and the place of the state's speed among them
	std::array<int32_t, 6> speeds {};
	uint32_t speedIndex {0};
	/// Its life, 0 to 1, and below what it walks or crawls wounded
	float life {1.0f};
	float lifeWhenWalksWounded {0.0f};
	float lifeWhenCrawlsWounded {0.0f};
	/// The land's speed balance and belief speed scale (1 unless the land sets them)
	float landSpeedBalance {1.0f};
	float landBeliefSpeedScale {1.0f};
	/// Its player's Indian tribal power, none without a player
	std::optional<float> indianPower;
	/// For a town of a player but the neutral one: the town's belief in the player and in the neutral player
	struct Belief
	{
		float player;
		float neutral;
	};
	std::optional<Belief> belief;
	/// The town tables' belief speed scales, and whether this is a multiplayer or skirmish game and the land's number
	float beliefSpeedScaleMultiPlayer {1.0f};
	std::array<float, 6> beliefSpeedScaleStory {};
	bool multiplayer {false};
	uint32_t landNumber {0};
	/// Its town's needs, 0 to 1, none without a town, and how they quicken it
	std::optional<float> townNeeds;
	float baseForTownNeedsSpeedMod {1.0f};
	float divisorForTownNeedsSpeedMod {1.0f};
	bool townInEmergency {false};
	/// The wood and food it carries, the most it carries of each, and how a full load slows it
	float woodHeld {0.0f};
	float foodHeld {0.0f};
	float maxWood {1.0f};
	float maxFood {1.0f};
	float speedModWhenFullLoadOfWood {0.0f};
	float speedModWhenFullLoadOfFood {0.0f};
	/// Sped up by magic food, and by how much
	bool foodSpeedUp {false};
	float foodPowerupIncrease {1.0f};
};

/// Random numbers from 0 to a maximum, on the game's shared stream
using FloatRandom = std::function<float(float)>;

/// The speed of the state, before the villager's own way of going
[[nodiscard]] int32_t StateSpeed(const Inputs& inputs, const FloatRandom& random);

/// The villager's own way of going, a factor of its speed: a little faster or slower by when it was made, slower young
/// or old, and as an adult slower the hungrier it is, by its life and for a woman
struct Person
{
	uint32_t creationIndex {0};
	uint32_t age {0};
	uint32_t grownUpAge {0};
	uint32_t oldAge {0};
	float desireForFood {0.0f};
	float life {1.0f};
	bool female {false};
};
[[nodiscard]] float PersonalFactor(const Person& person);
/// The speed it goes at, in whole units from 0 to 65535
[[nodiscard]] uint16_t FinalSpeed(int32_t speed, float factor);

} // namespace openblack::ecs::villager_speed
