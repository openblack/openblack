/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureWatching.h"

#include <cmath>

#include <algorithm>

using namespace openblack;
using namespace openblack::creature_watching;

namespace
{
/// Short of learning a skill, the meter shows it half learnt
constexpr float k_SkillMeterShare = 0.5f;
/// A miracle seen this many times or more beyond those it needed no longer fills the learning meter
constexpr float k_LearntMeterSightings = 3.0f;
/// Seen this share of the times it needs, a miracle is nearly learnt
constexpr float k_NearlyLearntShare = 0.75f;

uint32_t StageLength(const MimicRule& rule, const std::function<uint32_t(uint32_t)>& random)
{
	return rule.stageSteps + random(2);
}
} // namespace

Knowledge creature_watching::StartKnowledge(std::span<const SkillRule> skills, std::span<const MiracleRule> miracles)
{
	Knowledge knowledge {
	    .skillsSeen = std::vector<Sighting>(skills.size()),
	    .miraclesSeen = std::vector<Sighting>(miracles.size()),
	    .skillsKnown = std::vector<bool>(skills.size(), false),
	    .miraclesKnown = std::vector<bool>(miracles.size(), false),
	};
	return knowledge;
}

Progress creature_watching::SeeSkill(Knowledge& knowledge, size_t skill, std::span<const SkillRule> rules, uint32_t phase,
                                     uint32_t turn, float turnsPerSecond)
{
	if (skill >= rules.size() || skill >= knowledge.skillsSeen.size() || skill >= knowledge.skillsKnown.size())
	{
		return {.ignored = true};
	}
	if (knowledge.skillsKnown[skill])
	{
		return {.learnt = true, .share = 1.0f, .ignored = true};
	}
	if (phase < rules[skill].minPhase)
	{
		return {.ignored = true};
	}
	auto& seen = knowledge.skillsSeen[skill];
	++seen.count;
	if (!seen.turn.has_value())
	{
		seen.turn = turn;
	}
	const auto seconds = static_cast<float>(turn - *seen.turn) / std::max(turnsPerSecond, 1.0f);
	if (seconds >= rules[skill].watchSeconds)
	{
		knowledge.skillsKnown[skill] = true;
		return {.learnt = true, .share = 1.0f};
	}
	return {.share = k_SkillMeterShare};
}

float creature_watching::TimesNeeded(uint32_t timesToSee, float speciesMultiplier)
{
	return static_cast<float>(timesToSee) * speciesMultiplier;
}

std::optional<Prerequisite> creature_watching::MiraclePrerequisite(size_t miracle)
{
	using Kind = Prerequisite::Kind;
	switch (miracle)
	{
	case 2: // the fireball's power-ups
	case 3:
		return Prerequisite {.kind = Kind::Miracle, .index = 1};
	case 5: // the lightning bolt's
	case 6:
		return Prerequisite {.kind = Kind::Miracle, .index = 4};
	case 8: // the explosion's first power-up, then its second
		return Prerequisite {.kind = Kind::Miracle, .index = 7};
	case 9:
		return Prerequisite {.kind = Kind::Miracle, .index = 8};
	case 11: // the heal's
		return Prerequisite {.kind = Kind::Miracle, .index = 10};
	case 17: // the storm with lightning, then the tornado
		return Prerequisite {.kind = Kind::Miracle, .index = 16};
	case 18:
		return Prerequisite {.kind = Kind::Miracle, .index = 17};
	case 40: // the thirst and itch spells need the skill of building
	case 41:
		return Prerequisite {.kind = Kind::Skill, .index = 0};
	default:
		return std::nullopt;
	}
}

Progress creature_watching::SeeMiracle(Knowledge& knowledge, size_t miracle, std::span<const MiracleRule> rules, uint32_t phase,
                                       uint32_t turn, uint32_t weight, float speciesMultiplier)
{
	if (miracle >= rules.size() || miracle >= knowledge.miraclesSeen.size() || miracle >= knowledge.miraclesKnown.size())
	{
		return {.ignored = true};
	}
	if (const auto needs = MiraclePrerequisite(miracle))
	{
		const auto& known = needs->kind == Prerequisite::Kind::Skill ? knowledge.skillsKnown : knowledge.miraclesKnown;
		if (needs->index >= known.size() || !known[needs->index])
		{
			return {.ignored = true};
		}
	}
	const auto& rule = rules[miracle];
	if (phase < rule.minPhase)
	{
		return {.ignored = true, .event = LearningEvent::TooYoung};
	}
	auto& seen = knowledge.miraclesSeen[miracle];
	// A sighting soon after the last doesn't count, but is remembered as the last
	if (!seen.turn.has_value() || turn - *seen.turn > k_MiracleSightingTurns)
	{
		seen.count += weight;
	}
	seen.turn = turn;
	knowledge.miraclesKnown[miracle] = true;
	const auto needed = TimesNeeded(rule.timesToSee, speciesMultiplier);
	const auto count = static_cast<float>(seen.count);
	if (needed <= count)
	{
		// The meter shows it full for the first few sightings beyond those needed
		Progress progress {.learnt = true, .share = 1.0f, .event = LearningEvent::Learnt};
		if (count - needed < k_LearntMeterSightings)
		{
			progress.meter = 1.0f;
		}
		return progress;
	}
	const float share = count / needed;
	return {.share = share,
	        .event = share >= k_NearlyLearntShare ? std::optional(LearningEvent::NearlyLearnt) : std::nullopt,
	        .meter = share};
}

const char* creature_watching::Name(MimicStage stage)
{
	switch (stage)
	{
	case MimicStage::Notice:
		return "noticing";
	case MimicStage::CopyAction:
		return "copying the action";
	case MimicStage::CopyDesire:
		return "copying the desire";
	}
	return "?";
}

bool creature_watching::StartMimicry(std::optional<Mimicry>& mimicry, size_t rule, std::span<const MimicRule> rules,
                                     const MimicConditions& conditions, std::optional<uint32_t> object,
                                     const std::function<float()>& random)
{
	if (rule >= rules.size())
	{
		return false;
	}
	const auto& wanted = rules[rule];
	if (conditions.phase < k_MinMimicPhase || (wanted.needsLearningLeash && !conditions.learningLeashInHand) ||
	    conditions.reactionPriority >= k_MaxReactionPriorityToMimic || !conditions.canSee || wanted.chance <= 0.0f)
	{
		return false;
	}
	if (mimicry.has_value() && (mimicry->rule == rule || rules[mimicry->rule].chance > wanted.chance))
	{
		return false;
	}
	if (random() >= wanted.chance)
	{
		return false;
	}
	mimicry = Mimicry {.rule = rule, .stage = MimicStage::Notice, .stepsLeft = wanted.stageSteps, .object = object};
	return true;
}

void creature_watching::StepMimicry(std::optional<Mimicry>& mimicry, std::span<const MimicRule> rules,
                                    const std::function<uint32_t(uint32_t)>& random)
{
	if (!mimicry.has_value() || mimicry->rule >= rules.size())
	{
		mimicry.reset();
		return;
	}
	if (mimicry->stepsLeft > 0)
	{
		--mimicry->stepsLeft;
		return;
	}
	const auto& rule = rules[mimicry->rule];
	switch (mimicry->stage)
	{
	case MimicStage::Notice:
		mimicry->stage = MimicStage::CopyAction;
		mimicry->stepsLeft = StageLength(rule, random);
		break;
	case MimicStage::CopyAction:
		if (rule.copiesDesire)
		{
			mimicry->stage = MimicStage::CopyDesire;
			mimicry->stepsLeft = StageLength(rule, random);
		}
		else
		{
			mimicry.reset();
		}
		break;
	case MimicStage::CopyDesire:
		mimicry.reset();
		break;
	}
}

void creature_watching::StrokedWhileMimicking(std::optional<Mimicry>& mimicry, std::span<const MimicRule> rules)
{
	if (!mimicry.has_value() || mimicry->rule >= rules.size())
	{
		return;
	}
	mimicry->stage = MimicStage::CopyAction;
	mimicry->stepsLeft = rules[mimicry->rule].stageSteps;
}
