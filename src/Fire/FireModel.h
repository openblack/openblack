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

/// How hot things get and how they burn. Anything hotter than the air has a temperature. Each kind of object has a
/// combustion temperature, a heat capacity and a defence against burning. At or above its combustion temperature it
/// burns: it slowly heats itself up to twice that temperature, loses life by how far above it is, chars as its life runs
/// low, and heats everything within reach. Below it, it cools by its surface over its heat capacity, fifty times faster in
/// the water and faster in the rain.
///
/// These are the formulas only, free of the world, so they can be tested on their own.
namespace openblack::fire
{

/// The air's temperature, everywhere
constexpr float k_AmbientTemperature = 24.7f;
/// Nothing catches below this, whatever its table says
constexpr float k_LowestCombustion = 40.0f;
/// The living react to something this hot, even when it doesn't burn
constexpr float k_ReactionTemperature = 100.0f;
/// A fire this little above the air is gone, once nothing is charred
constexpr float k_GoneAboveAmbient = 0.1f;
/// Heat passed for each degree of difference
constexpr float k_HeatPerDegree = 0.1f * 100.0f;
/// A fire burns as far as its object's radius, this much further at its fiercest
constexpr float k_FireRadiusScale = 1.25f;
/// The flames stand this much taller than the object at their fiercest
constexpr float k_FlameHeightScale = 1.25f;
/// A fire this many times its combustion temperature is very hot
constexpr float k_VeryHotScale = 3.0f;
/// A fire's share of its fiercest starts at this share of its combustion temperature
constexpr float k_FractionStart = 0.8f;
/// The fire reaches objects this much further than its radius, cell by cell
constexpr float k_SpreadMargin = 10.0f;
/// Two things above this height over the land only heat each other when the flames reach across the gap
constexpr float k_LowFireHeight = 3.0f;
/// Something this low over a watery cell is in the water
constexpr float k_InWaterHeight = 2.0f;
/// It cools this many times faster in the water
constexpr float k_WaterCooling = 50.0f;
/// Each of the rain's 0..127 cools at this share, unless the object says otherwise
constexpr float k_RainCooling = 0.01f;
/// Below this share of its life a burning thing chars, this much a turn; once out it loses this much a turn
constexpr float k_CharLife = 0.6f;
constexpr float k_CharRate = 0.04f;
constexpr float k_UncharRate = 0.02f;
/// A burning thing's damage each turn at twice its combustion temperature, by its defence
constexpr float k_DamageScale = 0.1f;
/// A fire this hot or more by its share of its fiercest makes its sound
constexpr float k_SoundFraction = 0.1f;

/// What an object is made of, for burning: from its kind's table, and how big it is
struct Material
{
	/// From the table; the combustion temperature is raised to the lowest any thing has
	float combustion {0.0f};
	float capacity {1.0f};
	/// How much a burning turn hurts it; 0 never hurts it, and it never counts as burning for heating itself
	float defence {0.0f};
	/// How urgently the villagers put it out
	float burningPriority {0.0f};
	/// How much the rain cools it, by its 0..127
	float rainCooling {k_RainCooling};
	/// Its radius across the ground and its height
	float radius {1.0f};
	float height {1.0f};
	/// How far its fire reaches at its fiercest, before the fire's own scale
	float fireRadius {1.0f};
};

/// The fire's flags for one turn
enum Flags : uint8_t
{
	/// It caught this turn
	k_JustIgnited = 1u << 0u,
	/// Hotter than three times its combustion temperature
	k_VeryHot = 1u << 1u,
	/// It cooled this turn, or the water or rain cooled it
	k_Cooling = 1u << 2u,
	/// It went out this turn
	k_JustExtinguished = 1u << 3u,
	/// The flags that last only a turn
	k_TurnFlags = k_JustIgnited | k_VeryHot | k_Cooling | k_JustExtinguished,
};

/// A fire's temperature now and at the end of the last turn, how charred its object is (0..1) and its flags
struct State
{
	float temperature {k_AmbientTemperature};
	float previous {k_AmbientTemperature};
	float charring {0.0f};
	uint8_t flags {0};
};

/// What is round a fire this turn
struct Surroundings
{
	/// In the water or on the coast, low enough to be wet
	bool inWater {false};
	/// The heaviest rain or snow on it, 0..127
	float rain {0.0f};
	/// Its object's life, 0..1
	float life {1.0f};
};

/// What one turn of a fire does to its object
struct TurnOutcome
{
	/// The fire has died away: nothing left of its heat and nothing charred
	bool gone {false};
	/// The life the burning takes this turn, before its object's own defences
	float damage {0.0f};
	/// The rain cooled it
	bool rainedOn {false};
};

// The material

[[nodiscard]] float CombustionTemperature(const Material& material);
/// Twice the combustion temperature: a fire heats itself no hotter
[[nodiscard]] float MaxTemperature(const Material& material);
/// Never less than 1
[[nodiscard]] float Capacity(const Material& material);
/// The surface that cools it: four times its height by its radius
[[nodiscard]] float CoolingArea(const Material& material);

// The temperature

[[nodiscard]] bool IsOnFire(float temperature, const Material& material);
[[nodiscard]] bool IsAboveReactionTemperature(float temperature, const Material& material);
/// How fierce it is, 0..1: from 0.8 of its combustion temperature to twice it, and no more than twice its life
[[nodiscard]] float FireFraction(float temperature, const Material& material, float life);
/// How far its fire reaches now, at its fiercest, and the most it can reach plus a safe margin
[[nodiscard]] float FireRadius(const Material& material, float fraction);
[[nodiscard]] float MaxFireRadius(const Material& material);
[[nodiscard]] float SafeFireRadius(float radius, float maxRadius);
/// How tall its flames stand above it
[[nodiscard]] float FlameHeight(float temperature, const Material& material);
/// The heat it holds above the air's
[[nodiscard]] float HeatContent(float temperature, const Material& material);
/// Heat added, changing the temperature by heat over capacity but no more than the limit; the new temperature
[[nodiscard]] float AddHeat(float temperature, const Material& material, float heat, float limit);
/// The heat a difference of temperatures passes
[[nodiscard]] float HeatFromDifference(float difference);
/// A burn (such as a miracle's 200, or water's -4000) brings the temperature towards the air's plus the burn
[[nodiscard]] float ApplyBurn(float temperature, const Material& material, float burn);
/// What a fire is set to when set alight at a speed: its combustion temperature and that much more of twice it
[[nodiscard]] float SetOnFireTemperature(const Material& material, float speed);
/// What a turn of burning at a temperature takes from its object's life
[[nodiscard]] float BurnDamage(float temperature, const Material& material);

/// One fire heating another object: the heat is ten for each degree between them, no more than half of what the fire
/// holds, and changes the target by no more than the difference. A fire not burning loses what it gives.
struct Transfer
{
	float sourceTemperature;
	float targetTemperature;
};
[[nodiscard]] Transfer HeatTransfer(float sourceTemperature, const Material& source, float targetTemperature,
                                    const Material& target);

/// One turn of a fire, before it spreads: its flags, the water or the rain cooling it, burning heating it, cooling
/// otherwise, the damage and the charring. The new temperature is in the state; the previous is left for the end of the
/// turn.
[[nodiscard]] TurnOutcome Step(State& state, const Material& material, const Surroundings& surroundings);

} // namespace openblack::fire
