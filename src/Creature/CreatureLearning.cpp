/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureLearning.h"

#include <cmath>

#include <algorithm>
#include <limits>

#include <fmt/format.h>

using namespace openblack;
using namespace openblack::creature_learning;

namespace
{
constexpr size_t k_LikesLevels = 11;
constexpr std::array<std::string_view, k_LikesLevels> k_LikesWords {
    "extremely", "very much", "a lot",  "quite a lot", "fairly",     "moderately",
    "a little",  "not much",  "hardly", "barely",      "not at all",
};
} // namespace

float creature_learning::LessonFactor(float divisor, float feedback)
{
	return (1.0f / std::max(divisor, std::numeric_limits<float>::epsilon()) - 1.0f) * feedback + 1.0f;
}

void creature_learning::LearnDesireLesson(creature_desires::Desires& desires, Desire desire, std::optional<size_t> source,
                                          float feedback, const AllDesireRules& rules, const Dependencies& dependencies)
{
	const auto index = static_cast<size_t>(desire);
	const auto& rule = rules.at(index);
	auto& state = desires[desire];
	const auto factor = LessonFactor(rule.lessonDivisor, feedback);
	if (factor <= 0.0f)
	{
		return;
	}
	state.increaseSeconds = std::clamp(state.increaseSeconds * factor, k_MinIncreaseSeconds, k_MaxIncreaseSeconds);
	if (source.has_value() && *source < state.sources.size())
	{
		auto& driver = state.sources.at(*source);
		driver.threshold = std::clamp(driver.threshold - k_ThresholdStep * feedback, k_MinThreshold, k_MaxThreshold);
	}
	const auto lowest = std::max(rule.initialMax - k_MaxStep, 0.0f);
	const auto highest = std::min(rule.initialMax + k_MaxStep, k_MaxMax);
	state.max = std::clamp(state.max + k_MaxStep * feedback, std::min(lowest, highest), highest);
	if (rule.learnsWeight)
	{
		state.weight = std::clamp(state.weight + rule.initialWeight / 3.0f * feedback, k_MinWeight, k_MaxWeight);
	}
	state.decay = std::clamp(state.decay + k_DecayStep * feedback, std::min(rule.decayMin, rule.decayMax),
	                         std::max(rule.decayMin, rule.decayMax));
	// The desires that go with it grow faster with it, the opposed ones slower
	for (size_t other = 0; other < k_DesireCount; ++other)
	{
		const auto dependency = dependencies.at(index).at(other);
		auto& otherState = desires.desires.at(other);
		if (other == index || dependency == 0.0f)
		{
			continue;
		}
		const auto changed = dependency > 0.0f ? otherState.increaseSeconds * factor : otherState.increaseSeconds / factor;
		otherState.increaseSeconds = std::clamp(changed, k_MinIncreaseSeconds, k_MaxIncreaseSeconds);
	}
}

float creature_learning::OpinionAfter(float opinion, float feedback)
{
	return std::clamp(opinion + k_OpinionRate * (feedback - opinion), -1.0f, 1.0f);
}

float creature_learning::Strengthened(float feedback)
{
	if (feedback == 0.0f)
	{
		return 0.0f;
	}
	return std::copysign(std::max(std::abs(feedback), k_MinFeedback), feedback);
}

std::vector<std::pair<Desire, float>> creature_learning::Spread(Desire desire, float feedback, const Dependencies& dependencies)
{
	std::vector<std::pair<Desire, float>> spread {{desire, feedback}};
	const auto index = static_cast<size_t>(desire);
	for (size_t other = 0; other < k_DesireCount; ++other)
	{
		const auto dependency = dependencies.at(index).at(other);
		if (other != index && dependency != 0.0f)
		{
			spread.emplace_back(static_cast<Desire>(other), dependency * feedback);
		}
	}
	return spread;
}

float creature_learning::LearningPriority(const Context& context)
{
	if (!context.learnable)
	{
		return 0.0f;
	}
	if (context.running)
	{
		return 1.0f;
	}
	if (context.windowSeconds > 0.0f && context.secondsSince < context.windowSeconds)
	{
		return 1.0f - context.secondsSince / context.windowSeconds;
	}
	return 0.0f;
}

std::optional<size_t> creature_learning::BestContext(std::span<const Context> contexts)
{
	std::optional<size_t> best;
	float bestPriority = 0.0f;
	for (size_t i = 0; i < contexts.size(); ++i)
	{
		const auto priority = LearningPriority(contexts[i]);
		if (priority > bestPriority)
		{
			best = i;
			bestPriority = priority;
		}
	}
	return best;
}

void creature_learning::Remember(std::vector<Context>& contexts, Context context)
{
	// Whatever was running has finished now
	for (auto& old : contexts)
	{
		old.running = false;
	}
	contexts.push_back(std::move(context));
	if (contexts.size() > k_MaxContexts)
	{
		contexts.erase(contexts.begin(), contexts.begin() + static_cast<std::ptrdiff_t>(contexts.size() - k_MaxContexts));
	}
}

void creature_learning::Age(std::vector<Context>& contexts, float seconds)
{
	for (auto& context : contexts)
	{
		if (!context.running)
		{
			context.secondsSince += seconds;
		}
	}
}

void creature_learning::SuppressOpposed(creature_desires::Desires& desires, Desire desire, const Dependencies& dependencies,
                                        float turnsPerSecond)
{
	const auto index = static_cast<size_t>(desire);
	for (size_t other = 0; other < k_DesireCount; ++other)
	{
		const auto dependency = dependencies.at(index).at(other);
		if (other != index && dependency < 0.0f)
		{
			creature_desires::Suppress(desires, static_cast<Desire>(other), -dependency * k_OpposedSuppressSeconds,
			                           turnsPerSecond);
		}
	}
}

void creature_learning::AfterAction(creature_desires::Desires& desires, Desire desire, float multiplier)
{
	auto& state = desires[desire];
	if (state.activated)
	{
		state.value = std::clamp(state.value * multiplier, 0.0f, std::max(state.max, 0.0f));
	}
}

void creature_learning::MakeLeastDominant(creature_desires::Desires& desires, Desire desire, float factor)
{
	float weakest = std::numeric_limits<float>::max();
	for (size_t i = 0; i < k_DesireCount; ++i)
	{
		const auto& other = desires.desires.at(i);
		if (i != static_cast<size_t>(desire) && other.activated)
		{
			weakest = std::min(weakest, other.value);
		}
	}
	auto& state = desires[desire];
	state.value = weakest == std::numeric_limits<float>::max() ? 0.0f : std::max(weakest / factor, 0.0f);
}

void creature_learning::MakeFullyDominant(creature_desires::Desires& desires, Desire desire)
{
	float strongest = 0.0f;
	for (size_t i = 0; i < k_DesireCount; ++i)
	{
		const auto& other = desires.desires.at(i);
		if (i != static_cast<size_t>(desire) && other.activated)
		{
			strongest = std::max(strongest, other.value);
		}
	}
	auto& state = desires[desire];
	state.activated = true;
	state.suppressedTurns = 0;
	state.value = std::max(state.max, strongest * k_LeastDominantFactor);
}

std::optional<size_t> creature_learning::DominantSource(const creature_desires::DesireState& desire)
{
	std::optional<size_t> best;
	float bestDrive = -1.0f;
	for (size_t i = 0; i < desire.sources.size(); ++i)
	{
		if (desire.sources[i].drive > bestDrive)
		{
			best = i;
			bestDrive = desire.sources[i].drive;
		}
	}
	return best;
}

void creature_learning::ResetDrives(creature_desires::DesireState& desire)
{
	for (auto& source : desire.sources)
	{
		source.drive = 0.0f;
	}
}

CreatureAttitude& creature_learning::AttitudeTo(std::vector<CreatureAttitude>& attitudes, uint32_t creature)
{
	const auto found = std::ranges::find(attitudes, creature, &CreatureAttitude::creature);
	if (found != attitudes.end())
	{
		return *found;
	}
	return attitudes.emplace_back(CreatureAttitude {.creature = creature});
}

std::array<std::pair<Desire, float>, 2> creature_learning::ChangeHowNice(CreatureAttitude& attitude, float change)
{
	attitude.howNice = std::clamp(attitude.howNice + change, -1.0f, 1.0f);
	const auto sign = change >= 0.0f ? 1.0f : -1.0f;
	return {{{Desire::BeFriends, 0.5f * sign}, {Desire::Anger, -0.5f * sign}}};
}

std::string creature_learning::DesireLessonText(float feedback, std::string_view desire)
{
	return fmt::format("I've learnt to {} my {}", feedback >= 0.0f ? "increase" : "decrease", desire);
}

std::string creature_learning::ObjectLessonText(float feedback, std::string_view object, std::string_view trying)
{
	return fmt::format("I've learnt to {} {} when I {}", feedback >= 0.0f ? "choose" : "avoid", object, trying);
}

std::string creature_learning::ActionLessonText(float feedback, std::string_view action, std::string_view trying,
                                                std::string_view object)
{
	return fmt::format("I've learnt to {} {} when I {} {}", feedback >= 0.0f ? "like" : "dislike", action, trying, object);
}

size_t creature_learning::LikesLevel(Desire desire, const creature_desires::DesireState& state,
                                     std::span<const float> initialThresholds)
{
	if (desire == Desire::Fear || desire == Desire::Tiredness)
	{
		const auto share = state.max > 0.0f ? 10.0f * state.value / state.max : 0.0f;
		return 9 - std::min<size_t>(9, static_cast<size_t>(std::max(share, 0.0f)));
	}
	float change = 0.0f;
	for (size_t i = 0; i < state.sources.size() && i < initialThresholds.size(); ++i)
	{
		change += state.sources[i].threshold - initialThresholds[i];
	}
	const auto level = static_cast<size_t>((std::clamp(change, -1.0f, 1.0f) + 1.0f) * 5.0f);
	return std::min(level, k_LikesLevels - 1);
}

std::string_view creature_learning::LikesWords(size_t level)
{
	return k_LikesWords.at(std::min(level, k_LikesLevels - 1));
}
