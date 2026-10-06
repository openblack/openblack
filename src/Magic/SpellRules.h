/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

#include "Enums.h"
#include "InfoConstants.h"

// The rules of a running miracle that don't depend on the world: which kind of miracle a magic type is, what a cast
// hands it, how long it lasts, how hard its effect hits what it reaches, and what the food, wood and shield miracles
// cost. Pure functions of the tables, so they are tested on hand-made ones.

namespace openblack::magic
{

/// The kind of miracle a magic type is, which decides what it does beyond what its particles do
enum class SpellClass : uint8_t
{
	/// Fireball, lightning and the beam explosion: their particles are what they do
	General,
	Heal,
	Teleport,
	Forest,
	/// Food and wood
	Resource,
	StormAndTornado,
	Shield,
	Water,
	FlockFlying,
	FlockGround,
	/// The spells only creatures cast
	Creature,

	_Count
};

/// The kind of miracle of a magic type, by the section of the tables its record is in
[[nodiscard]] SpellClass ClassOf(MagicType type);

/// How big a miracle is cast without a gesture saying so, such as by a script
inline constexpr float k_DefaultSpellMagnitude = 40.0f;

/// What a cast hands a new miracle
struct SpellCastData
{
	/// How big: the size of the gesture that cast it, the radius a script gives
	float magnitude {k_DefaultSpellMagnitude};
	/// The prayer power it starts with
	float chants {0.0f};
	/// How long it lasts in seconds, k_NoTimeLimit for until its prayer power runs out
	float duration {-1.0f};
	/// How many objects (such as trees) it may still make, -1 for no limit
	int maxObjectsToCreate {-1};
};

/// What a miracle cast from a seed in the hand starts with: its effect's prayer power and the player's timer, both
/// scaled by the seed's multiplier (a miracle without a time limit keeps none), at the size of the gesture. A fire seed
/// always throws a fireball of the same size, whatever the gesture.
[[nodiscard]] SpellCastData SeedCastData(const InfoConstants& info, MagicType type, SpellSeedType seed, float multiplier,
                                         float gestureSize);

/// A miracle ages a turn; whether it has outlived its time
[[nodiscard]] bool AgeOneTurn(float& age, float duration, float seconds);

/// What a miracle's effect does to what it reaches, by kind
enum class EffectKind : uint8_t
{
	Burn,
	Crush,
	Hit,
	Heal,
	FlyAway,
	Alignment,
	Belief,

	_Count
};
inline constexpr size_t k_EffectKindCount = static_cast<size_t>(EffectKind::_Count);

struct EffectValues
{
	std::array<float, k_EffectKindCount> numbers {};
	/// How far round the point it acts
	float radius {1.0f};

	[[nodiscard]] float operator[](EffectKind kind) const { return numbers.at(static_cast<size_t>(kind)); }
	[[nodiscard]] float& operator[](EffectKind kind) { return numbers.at(static_cast<size_t>(kind)); }
	/// Every value times a factor; the radius stays
	void Scale(float factor);
	/// Whether it burns, crushes, hits or throws what it reaches
	[[nodiscard]] bool IsDestructive() const;
	/// A magic type's or any other effect record's values
	[[nodiscard]] static EffectValues From(const GEffectInfo& info);
};

/// How an object stands up to an effect: what each kind is multiplied by against it, and the heat at which it burns
struct EffectDefence
{
	std::array<float, k_EffectKindCount> multipliers {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
	float combustionTemperature {0.0f};

	/// An object's from its table record
	[[nodiscard]] static EffectDefence From(const GObjectInfo& info);
};

/// The share of an object's life an effect takes: the crushing and hitting it suffers, each times its defence
[[nodiscard]] float DamageFrom(const EffectValues& values, const EffectDefence& defence);
/// The share of an object's life an effect gives back
[[nodiscard]] float HealFrom(const EffectValues& values, const EffectDefence& defence);
/// Heat above an object's combustion temperature takes this share of its life for each combustion temperature of
/// heat, times its burn defence
inline constexpr float k_HeatDamageScale = 0.1f;
/// The share of an object's life a heat takes: none below its combustion temperature
[[nodiscard]] float HeatDamage(float temperature, const EffectDefence& defence);
/// An object's life, 0 to 1, after an effect has healed and hurt it
[[nodiscard]] float LifeAfter(float life, const EffectValues& values, const EffectDefence& defence);

/// What one grain or log of a food or wood miracle that lands brings and costs: the first brings the first amount,
/// each after it the amount per event, and each unit costs its prayer power
struct ResourceEventCost
{
	uint32_t units;
	float chants;
};
[[nodiscard]] ResourceEventCost ResourceEvent(const GMagicResourceInfo& info, bool firstDone);
/// Whether a food or wood miracle has prayer power enough for its first grain again, to be cast again from its seed
[[nodiscard]] bool HasEnoughChantsForResourceRecast(const GMagicResourceInfo& info, float chants);

/// A shield's radius, kept within its tables' smallest and largest
[[nodiscard]] float ClampShieldRadius(const GMagicShieldInfo& info, float radius);
/// A shield's upkeep each turn: its plain upkeep times the square of its radius over the radius that costs that much
[[nodiscard]] float ShieldCostToMaintain(float plainCost, float radius, float radiusForNormalCost);

/// The heal miracle heals no more than this many people when its tables give no number
inline constexpr uint32_t k_DefaultHealTargets = 20;

/// The water miracle rains within this radius of where it is cast: wider for its power-up
[[nodiscard]] float RainRadius(MagicType type);
/// How far from the middle a drop of rain falls, from a random number up to the rain's radius
[[nodiscard]] constexpr float DropDistance(float random)
{
	constexpr float k_Spread = 0.7f;
	constexpr float k_Inner = 0.3f;
	return random * k_Spread + k_Inner;
}
/// A drop of rain waters the objects whose edge is within this distance of it
inline constexpr float k_WaterReach = 2.5f;
/// How big the ring a drop leaves on the land grows: bigger for the power-up
[[nodiscard]] float RippleGrowth(MagicType type);
/// A drop leaves a ring once this many seconds have passed since the last
inline constexpr float k_RippleEvery = 0.1f;

} // namespace openblack::magic
