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

#include <optional>

/// A creature's body as time passes: it ages, grows while it stands still, uses up its energy and burns fat once
/// hungry, tires from moving, gets thirsty, warms or cools by how far the weather is from what its species likes, and
/// grows stronger from hard work. Eating fills it up (and fattens it if it overeats) and builds up poo, drinking
/// quenches its thirst, and sleeping heals it and rests it. It faints when exhausted, starved or out of life.
///
/// Everything here works a game turn at a time, from the species' rates in the game's creature tables.
namespace openblack::creature_physiology
{
/// Growth asleep is this many times faster
constexpr float k_SleepGrowthFactor = 3.0f;
/// The slowest a creature ever grows, in size a game turn
constexpr float k_MinGrowth = 2.7e-7f;
/// Growth goes on at full speed until this fraction of the growing up time, then falls to its slowest
constexpr float k_YouthGrowthSlope = 8.0f;
constexpr float k_MaxYouthGrowth = 2.0f;
/// A creature grows by itself up to this size
constexpr float k_MaxGrownSize = 2.0f;
/// A bigger creature uses its energy more slowly, up to half as fast at size 2, and asleep or resting much more slowly
constexpr float k_EnergySizeFactor = 0.5f;
constexpr float k_RestingEnergyDivisor = 3.0f;
/// It burns fat while its energy is below this
constexpr float k_FatBurnBelowEnergy = 0.5f;
/// Moving tires a young creature up to four times as fast, less so as it ages, never less than the species' rate
constexpr float k_YouthExhaustion = 4.0f;
constexpr float k_ExhaustionYouthPerAge = 0.15f;
/// Walking with too little energy tires it this much faster again
constexpr float k_LowEnergyExtraExhaustion = 0.3f;
/// The weather warms or cools it once the difference from its comfort is well past this point of the sigmoid, the
/// difference counted in steps of this many degrees
constexpr float k_WarmthThreshold = 0.6f;
constexpr float k_WarmthPerDegree = 0.025f;
/// A meal counts for at least this much of the creature's size
constexpr float k_MinMealSize = 0.8f;
/// Asleep for fewer turns than this, it never wakes by itself; it wakes once fully rested in the day, or once nearly
/// rested and asleep for long enough for its size
constexpr uint32_t k_MinSleepTurns = 50;
constexpr float k_NearlyRested = 0.1f;
constexpr float k_SleepTurnsPerLength = 5.0f;
/// Woken from a faint, it is no more exhausted or thirsty than this, and has at least this much energy
constexpr float k_FaintWakeExhaustion = 0.9f;
constexpr float k_FaintWakeDehydration = 0.9f;
constexpr float k_FaintWakeEnergy = 0.1f;
/// The stages of growing up from which the body uses energy, and from which it also grows and gets thirsty
constexpr uint32_t k_BodyPhase = 1;
constexpr uint32_t k_GrowingPhase = 3;
/// Only creatures this far grown up and owned by a player faint
constexpr uint32_t k_FaintingPhase = 5;

/// What a species' body is like, from the game's creature tables
struct Species
{
	float startEnergy {1.0f};
	float startWarmth {0.0f};
	float comfortTemperature {15.0f};
	uint32_t secondsPerAgeTick {3600};
	float growUpMinutes {240.0f};
	float lowEnergyThreshold {0.2f};
	float exhaustionRate {4e-5f};
	float secondsToDehydrate {5000.0f};
	float strengthDecay {1.0f};
	float carryStrengthMinutes {6.0f};
	float energyDrain {1.162e-4f};
	float fatBurn {8e-5f};
	float overeatFatFactor {0.06f};
	float sleepHeal {0.00111f};
	float sleepRecover {0.0025f};
	float sleepLength {0.1f};
	float foodToEnergy {1000.0f};
	float pooPerEnergy {0.8f};
};

/// How the body is: all 0 to 1 but warmth, -1 (cold) to 1 (hot), and energy, which a big meal can push up to the
/// creature's size
struct Needs
{
	/// Hours of game time it has lived, by default
	uint32_t age {0};
	/// Game turns it has lived, which its age follows
	uint32_t turns {0};
	float warmth {0.0f};
	/// How full it is; it is hungry as this runs out
	float energy {1.0f};
	float itchiness {0.0f};
	float poo {0.0f};
	float exhaustion {0.0f};
	float dehydration {0.0f};
	float life {1.0f};
	/// Meals eaten
	uint32_t meals {0};
};

/// What the creature has become, which its body shows: fatness and strength 0 to 1, and its size
struct Shape
{
	float fatness {0.5f};
	float strength {0.5f};
	float size {1.0f};
};

/// What else the body goes through this turn
struct Turn
{
	/// On its way somewhere
	bool moving {false};
	/// Asleep, or resting to get better
	bool asleep {false};
	bool resting {false};
	/// How far it has grown up, which stages of the body follow
	uint32_t phase {13};
	/// What it carries, as heavy as itself at 1, while it carries something
	std::optional<float> carriedWeight;
	/// The temperature where it stands
	float temperature {0.0f};
	float turnsPerSecond {10.0f};
};

/// A new creature's body
[[nodiscard]] Needs Start(const Species& species);

/// Hunger, from 0 when full of energy
[[nodiscard]] float Hunger(const Needs& needs);

/// The game's sigmoid of how far a value is past a threshold, through its table of 41 steps
[[nodiscard]] float SigmoidThreshold(float threshold, float value);

/// How much the creature grows this turn: fast while young, then slowly, all the more for energy it has to spare
[[nodiscard]] float Growth(const Needs& needs, const Species& species, bool asleep, float turnsPerSecond);

/// One game turn of the body as time passes, before the stage of growing up from which the body changes nothing
void TickTurn(Needs& needs, Shape& shape, const Species& species, const Turn& turn);

/// Gains strength, within 0 and 1
void ModifyStrength(Shape& shape, float amount);

/// What doing an action costs the body
struct ActionCost
{
	float strengthGain {0.0f};
	float energyCost {0.0f};
	float exhaustionCost {0.0f};
};
/// The body once an action is done: stronger, less full and more tired by the action, the last two less the bigger it
/// is
void ApplyActionCost(Needs& needs, Shape& shape, const ActionCost& cost, uint32_t phase);

/// Eats something of a food value: fills up (up to its size), fattens by what it overeats and builds up poo. Returns the
/// energy it gained.
float Eat(Needs& needs, Shape& shape, const Species& species, float foodValue);
/// Drinks its fill
void Drink(Needs& needs);
/// Has a poo
void Poo(Needs& needs);

/// One game turn asleep: it heals and rests. Returns whether it wakes now, having slept some turns with this one.
[[nodiscard]] bool SleepTurn(Needs& needs, const Species& species, float size, uint32_t turnsAsleep, bool night);

/// Why a creature faints
enum class Faint : uint8_t
{
	OutOfLife,
	Starving,
	Exhausted,
};
/// Whether it faints now: only once far enough grown up and owned by a player
[[nodiscard]] std::optional<Faint> ShouldFaint(const Needs& needs, uint32_t phase, bool ownedByPlayer);
/// Coming round from a faint: no longer quite exhausted, starved or parched
void WakeFromFaint(Needs& needs);

/// The desire sources the body drives, or nothing for sources it doesn't
[[nodiscard]] std::optional<float> SourceValue(uint32_t type, const Needs& needs, bool night);
} // namespace openblack::creature_physiology
