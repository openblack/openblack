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
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureDesires.h"

/// How a creature learns from the player's strokes and slaps. Feedback is credited to what it did lately: fully to
/// what it is doing, and to what it did a little while ago less the longer ago, within each action's learning window.
/// From the action it learns four lessons: to want the desire behind it more or less (how fast it grows, how far, and
/// how readily its source drives it, spread to the desires that depend on it), to choose that kind of thing more or less
/// for the desire, to use that kind of thing more or less, and its opinion of the action itself.
namespace openblack::creature_learning
{
using creature_desires::Desire;
using creature_desires::k_DesireCount;

/// Feedback weaker than this counts as this strong, either way
constexpr float k_MinFeedback = 0.5f;
/// How a desire lesson moves its source's threshold, its maximum, and its decay, at full feedback
constexpr float k_ThresholdStep = 0.12f;
constexpr float k_MaxStep = 0.1f;
constexpr float k_DecayStep = 0.001f;
/// The bounds of a desire's growing time, in seconds, of its sources' thresholds, its weight, and its maximum
constexpr float k_MinIncreaseSeconds = 1.0f;
constexpr float k_MaxIncreaseSeconds = 1200.0f;
constexpr float k_MinThreshold = 0.2f;
constexpr float k_MaxThreshold = 1.0f;
constexpr float k_MinWeight = 1.0f;
constexpr float k_MaxWeight = 8.0f;
constexpr float k_MaxMax = 2.0f;
/// An opinion moves this share of the way to the feedback
constexpr float k_OpinionRate = 0.8f;
/// Deciding on a desire holds back those it opposes, for this many seconds for each unit of opposition
constexpr float k_OpposedSuppressSeconds = 240.0f;
/// After a slap, the desire is made this much weaker than the weakest other
constexpr float k_LeastDominantFactor = 1.3f;
/// The most recent actions remembered for feedback
constexpr size_t k_MaxContexts = 8;

/// How a species' desire starts and what feedback can do to it, from the game's tables
struct DesireRules
{
	float initialMax {1.0f};
	float decayMin {1.0f};
	float decayMax {1.0f};
	float initialWeight {1.0f};
	/// Whether feedback changes the weight
	bool learnsWeight {false};
	/// How strongly feedback changes how fast it grows: by (1/divisor - 1) times the feedback, plus 1
	float lessonDivisor {4.0f};
	/// Whether the creature can learn from feedback about it at all
	bool learnable {false};
	/// How much distance counts against a goal
	float distanceWeight {0.02f};
};
using AllDesireRules = std::array<DesireRules, k_DesireCount>;

/// How much each desire depends on each other: positive for desires that go together, negative for opposed ones
using Dependencies = std::array<std::array<float, k_DesireCount>, k_DesireCount>;

/// The factor a desire's growing time is multiplied by for feedback
[[nodiscard]] float LessonFactor(float divisor, float feedback);
/// Learns to want a desire more (positive feedback) or less: it grows faster, further, its source drives it more
/// readily, it fades more slowly; desires that go with it grow faster too, and opposed ones slower
void LearnDesireLesson(creature_desires::Desires& desires, Desire desire, std::optional<size_t> source, float feedback,
                       const AllDesireRules& rules, const Dependencies& dependencies);
/// The opinion of an action after feedback for it
[[nodiscard]] float OpinionAfter(float opinion, float feedback);
/// Feedback made at least the weakest that counts
[[nodiscard]] float Strengthened(float feedback);
/// The desires an object lesson is learnt for: the desire itself fully, and every other in proportion to how much it
/// depends on it
[[nodiscard]] std::vector<std::pair<Desire, float>> Spread(Desire desire, float feedback, const Dependencies& dependencies);

/// Something the creature did lately, which feedback can be credited to
struct Context
{
	uint32_t action {0};
	Desire desire {Desire::Impress};
	/// What it acted on, and what it knew of it
	std::optional<uint32_t> object;
	std::optional<creature_tree::Belief> belief;
	/// What it used, and what it knew of that
	std::optional<uint32_t> used;
	std::optional<creature_tree::Belief> usedBelief;
	/// Still doing it, or how long ago it finished
	bool running {true};
	float secondsSince {0.0f};
	/// Whether the action can be learnt from, and for how many seconds after it finishes
	bool learnable {true};
	float windowSeconds {0.0f};
	/// The feedback last credited to it, for the debug readouts
	std::optional<float> credited;
};
/// How much feedback now counts for a context: fully while running, then less until its window closes; nothing for an
/// action that can't be learnt from
[[nodiscard]] float LearningPriority(const Context& context);
/// The context feedback is credited to, if any counts
[[nodiscard]] std::optional<size_t> BestContext(std::span<const Context> contexts);
/// Remembers a new action, the newest last, forgetting the oldest beyond the most kept
void Remember(std::vector<Context>& contexts, Context context);
/// Time passes for the contexts that have finished
void Age(std::vector<Context>& contexts, float seconds);

/// Deciding on a desire holds back the desires it opposes for a while
void SuppressOpposed(creature_desires::Desires& desires, Desire desire, const Dependencies& dependencies, float turnsPerSecond);
/// Having done an action, the desire it serves is multiplied down, if active
void AfterAction(creature_desires::Desires& desires, Desire desire, float multiplier);
/// The desire becomes weaker than every other active desire by a factor, or stronger than every one up to its maximum
void MakeLeastDominant(creature_desires::Desires& desires, Desire desire, float factor);
void MakeFullyDominant(creature_desires::Desires& desires, Desire desire);
/// The source that has driven a desire most since it was last decided on
[[nodiscard]] std::optional<size_t> DominantSource(const creature_desires::DesireState& desire);
/// Starts counting the sources' drive afresh
void ResetDrives(creature_desires::DesireState& desire);

/// How the creature feels about another creature
struct CreatureAttitude
{
	uint32_t creature {0};
	/// -1 to 1
	float howNice {0.0f};
	/// 0 to 1
	float howImpressive {0.0f};
	float attention {0.0f};
};
/// Finds or adds the attitude to a creature
[[nodiscard]] CreatureAttitude& AttitudeTo(std::vector<CreatureAttitude>& attitudes, uint32_t creature);
/// Finds the other creature nicer or nastier; returns the friendship and anger lessons it learns about creatures like
/// it: nicer teaches befriending (+0.5) over anger (-0.5), nastier the reverse
[[nodiscard]] std::array<std::pair<Desire, float>, 2> ChangeHowNice(CreatureAttitude& attitude, float change);

/// The creature's thought about a lesson, as the game words it
[[nodiscard]] std::string DesireLessonText(float feedback, std::string_view desire);
[[nodiscard]] std::string ObjectLessonText(float feedback, std::string_view object, std::string_view trying);
[[nodiscard]] std::string ActionLessonText(float feedback, std::string_view action, std::string_view trying,
                                           std::string_view object);

/// How much the creature likes a desire, 0 (extremely) to 10 (not at all), by how far its sources' thresholds have moved
/// from where they started; fear and tiredness by how strong they are against their maximum
[[nodiscard]] size_t LikesLevel(Desire desire, const creature_desires::DesireState& state,
                                std::span<const float> initialThresholds);
[[nodiscard]] std::string_view LikesWords(size_t level);

} // namespace openblack::creature_learning
