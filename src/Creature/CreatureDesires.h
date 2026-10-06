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
#include <span>
#include <string_view>
#include <vector>

/// What a creature wants. Each desire grows while its sources (hunger from low energy, loneliness, and so on) are past
/// their thresholds, and otherwise fades; it can be suppressed for a while, during which it only fades. The strongest
/// desires are what the creature shows the player.
namespace openblack::creature_desires
{
/// The desires, in the order of the game's creature tables
enum class Desire : uint8_t
{
	Impress,
	Compassion,
	Anger,
	Play,
	Hunger,
	Fear,
	Curiosity,
	Poo,
	Tiredness,
	IdleWithPlayer,
	Wanderlust,
	Puke,
	BuildHome,
	BringStuffHome,
	Water,
	RestoreHealth,
	BeFriends,
	AttractAttention,
	ManifestState,
	GetWarmer,
	GetColder,
	Scratch,
	RunAwayFromPlayer,
	Rest,
	ObeyPlayer,
	Illness,
	ObeyCreature,
	Sadness,
	StayNearHome,
	TellPlayer,
	PlayWithPlayer,
	TellCreature,
	EducateFriend,
	FollowPlayerDesire,
	GetHigh,
	HangAroundAtHome,
	MentalIllness,
	MissFriend,
	LookAround,
	Steal,
	_Count
};
constexpr size_t k_DesireCount = static_cast<size_t>(Desire::_Count);
/// The kinds of source a desire grows from, by their place in the game's source table; this one means none
constexpr uint32_t k_SourceCount = 61;
constexpr uint32_t k_NoSource = k_SourceCount;
/// A desire grows from up to this many sources
constexpr size_t k_MaxSources = 8;

[[nodiscard]] std::string_view Name(Desire desire);

/// The sources whose values follow the creature's state each turn; all others are pushed by events and fade
namespace sources
{
constexpr uint32_t k_InnateKindness = 5;
constexpr uint32_t k_AngerFromSadness = 10;
constexpr uint32_t k_InnateAggression = 11;
constexpr uint32_t k_HungerFromLowEnergy = 14;
constexpr uint32_t k_PlayFromSadness = 16;
constexpr uint32_t k_FearFromDark = 17;
constexpr uint32_t k_TirednessFromExhaustion = 22;
constexpr uint32_t k_PooFromAmountOfPoo = 21;
constexpr uint32_t k_TirednessFromNight = 24;
constexpr uint32_t k_TirednessFromSadness = 25;
constexpr uint32_t k_InnateLethargy = 26;
constexpr uint32_t k_WaterFromDehydration = 32;
constexpr uint32_t k_RestoreHealthFromLife = 33;
constexpr uint32_t k_InnateFriendliness = 35;
constexpr uint32_t k_AttentionFromLoneliness = 36;
constexpr uint32_t k_AttentionFromLackOfInteraction = 37;
constexpr uint32_t k_ManifestState = 38;
constexpr uint32_t k_InnateCommunicativeness = 39;
constexpr uint32_t k_GetWarmer = 40;
constexpr uint32_t k_GetColder = 41;
constexpr uint32_t k_Scratch = 42;
constexpr uint32_t k_Sadness = 48;
} // namespace sources

/// The game's sigmoid of how far a source's value is past its threshold, 0 to 1 by a table of 41 steps; 0 for a value
/// of 0 or less, or a threshold of 1
[[nodiscard]] float Sigmoid(float threshold, float value);
/// The same sigmoid without the cut off: a value of 0 still reads its step of the table
[[nodiscard]] float SigmoidStep(float threshold, float value);

struct Source
{
	uint32_t type {k_NoSource};
	float value {0.0f};
	float threshold {1.0f};
	/// Each turn the value is multiplied by this, so that values nothing renews fade
	float multiplier {1.0f};
};

struct DesireState
{
	bool activated {true};
	float value {0.0f};
	float max {1.0f};
	/// Each turn the desire doesn't grow, it is multiplied by this
	float decay {1.0f};
	/// The seconds the desire takes to grow by 1 with all its drive
	float increaseSeconds {1.0f};
	/// Turns left during which the desire can't grow
	uint32_t suppressedTurns {0};
	std::vector<Source> sources;
};

struct Desires
{
	std::array<DesireState, k_DesireCount> desires {};
	/// All the activated desires' values added up, as of the last update
	float sum {0.0f};

	[[nodiscard]] DesireState& operator[](Desire desire) { return desires.at(static_cast<size_t>(desire)); }
	[[nodiscard]] const DesireState& operator[](Desire desire) const { return desires.at(static_cast<size_t>(desire)); }
};

/// How a species' desires start, from the game's creature tables
struct DesireSetup
{
	float max {1.0f};
	float decayMin {1.0f};
	float decayMax {1.0f};
	float increaseSeconds {1.0f};
	std::vector<Source> sources;
};

/// A creature's desires as it starts, its decays picked at random in their ranges by uniform(min, max)
[[nodiscard]] Desires Create(const std::array<DesireSetup, k_DesireCount>& setup,
                             const std::function<float(float, float)>& uniform);

/// The desires a stage of a creature's growing up brings, and those it takes away
struct PhaseDesires
{
	std::vector<Desire> add;
	std::vector<Desire> remove;
};
/// As a creature reaches a stage of growing up: no desire is held back any more, and only those the stages so far have
/// brought and not taken away again are active
void ActivateForPhase(Desires& desires, std::span<const PhaseDesires> phases, size_t phase);

/// A source's value from the creature's state, or nothing for sources that keep their own value
using SourceReader = std::function<std::optional<float>(uint32_t type, const Desires& desires)>;

/// One game turn of the sources: each that follows the creature's state reads it, then every one is multiplied by its
/// multiplier
void UpdateSources(Desires& desires, const SourceReader& read);
/// One game turn of the desires: each grows by its sources' drive, unless suppressed, else fades; clamped to 0 and its
/// maximum
void UpdateDesires(Desires& desires, float turnsPerSecond);
/// The desire can't grow for some seconds, or longer if it already couldn't
void Suppress(Desires& desires, Desire desire, float seconds, float turnsPerSecond);

/// The value of the first of a desire's sources of a type
[[nodiscard]] std::optional<float> SourceValue(const DesireState& desire, uint32_t type);

/// The action a creature plays to show a desire, for the desires it can show
[[nodiscard]] std::optional<size_t> EmoteFor(Desire desire);
/// The strongest activated desire the creature can show, if any is above a minimum
[[nodiscard]] std::optional<Desire> StrongestShowable(const Desires& desires, float minimum);
} // namespace openblack::creature_desires
