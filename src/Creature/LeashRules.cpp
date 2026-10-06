/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LeashRules.h"

#include <algorithm>

#include <glm/geometric.hpp>

using namespace openblack;
using namespace openblack::creature_leash;
using creature_desires::Desire;
using creature_mind::Activity;

namespace
{
/// A creature of size 1 is fifteen units tall, which is what the hand's leash length goes by
constexpr float k_SizeToScale = 15.0f;
constexpr float k_HandSlackScale = 0.7f;
constexpr float k_HandSlackBase = 22.0f;
constexpr float k_HandMaxScale = 3.0f;
constexpr float k_HandMaxBase = 32.0f;
constexpr float k_TiedStaticReach = 1.5f;
constexpr float k_TiedStaticMin = 180.0f;
constexpr float k_TiedStaticMax = 360.0f;
constexpr float k_TiedMobileHeights = 7.0f;
constexpr float k_TiedMobileMax = 40.0f;
constexpr float k_SlackShare = 0.5f;
} // namespace

std::optional<size_t> creature_leash::IndexOf(LeashType type)
{
	const auto it = std::ranges::find(k_Types, type);
	if (it == k_Types.end())
	{
		return std::nullopt;
	}
	return static_cast<size_t>(std::distance(k_Types.begin(), it));
}

const char* creature_leash::Name(LeashType type)
{
	switch (type)
	{
	case LeashType::Evil:
		return "Aggression";
	case LeashType::Rope:
		return "Learning";
	case LeashType::Good:
		return "Compassion";
	case LeashType::None:
		break;
	}
	return "None";
}

Lengths creature_leash::InHand(float creatureSize)
{
	const auto s = k_SizeToScale * creatureSize;
	return {.slack = (k_HandSlackScale * s) + k_HandSlackBase, .max = (k_HandMaxScale * s) + k_HandMaxBase};
}

Lengths creature_leash::TiedToStatic(float distance)
{
	const auto max = std::clamp(distance * k_TiedStaticReach, k_TiedStaticMin, k_TiedStaticMax);
	return {.slack = k_SlackShare * max, .max = max};
}

Lengths creature_leash::TiedToMobile(float creatureHeight)
{
	const auto max = std::min(creatureHeight * k_TiedMobileHeights, k_TiedMobileMax);
	return {.slack = k_SlackShare * max, .max = max};
}

leash_rope::Look creature_leash::LookFor(LeashType type)
{
	switch (type)
	{
	case LeashType::Evil:
		return {.halfWidth = 0.15f, .v0 = 0.375f, .v1 = 0.5f, .uScale = 2.5f};
	case LeashType::Good:
		return {.halfWidth = 0.225f, .v0 = 0.25f, .v1 = 0.375f, .uScale = 2.5f};
	case LeashType::Rope:
	case LeashType::None:
		break;
	}
	return {.halfWidth = 0.15f, .v0 = 0.125f, .v1 = 0.25f, .uScale = 2.5f};
}

std::optional<Desire> creature_leash::ForcedDesireFor(LeashType type)
{
	switch (type)
	{
	case LeashType::Evil:
		return Desire::Anger;
	case LeashType::Good:
		return Desire::Compassion;
	case LeashType::Rope:
	case LeashType::None:
		break;
	}
	return std::nullopt;
}

uint32_t creature_leash::MiracleSightingWeight(bool learningLeash)
{
	return learningLeash ? 3 : 1;
}

bool creature_leash::ShouldPull(float tension)
{
	return tension > k_PullTension;
}

std::optional<float> creature_leash::RecordPull(PullMemory& memory, Desire desire)
{
	auto& count = memory.counts.at(static_cast<size_t>(desire));
	count = static_cast<uint8_t>(std::min<int>(count + 1, UINT8_MAX));
	if (count < 2)
	{
		return std::nullopt;
	}
	const auto seconds = static_cast<float>(count) * k_SuppressSecondsPerPull;
	count = 0;
	return seconds;
}

std::optional<Desire> creature_leash::DesireBehind(Activity activity, std::optional<Desire> shown)
{
	switch (activity)
	{
	case Activity::Eat:
		return Desire::Hunger;
	case Activity::Drink:
		return Desire::Water;
	case Activity::Sleep:
		return Desire::Tiredness;
	case Activity::Poo:
		return Desire::Poo;
	case Activity::Puke:
		return Desire::Puke;
	case Activity::Sit:
	case Activity::BeIdle:
		return Desire::Rest;
	case Activity::HangAround:
		return Desire::Wanderlust;
	case Activity::ShowDesire:
		return shown;
	case Activity::Told:
		return Desire::ObeyPlayer;
	case Activity::None:
	case Activity::Faint:
		break;
	}
	return std::nullopt;
}

Lead creature_leash::DecideLead(const glm::vec3& creature, const glm::vec3& hand, std::optional<glm::vec2> destination,
                                bool walking)
{
	if (glm::distance(creature, hand) < k_CloseToHand)
	{
		return Lead::Stay;
	}
	const auto handAcross = glm::vec2(hand.x, hand.z);
	const auto creatureToHand = glm::distance(glm::vec2(creature.x, creature.z), handAcross);
	if (walking && destination.has_value() && creatureToHand > 0.0f &&
	    glm::distance(*destination, handAcross) / creatureToHand < k_CloserShare)
	{
		return Lead::KeepGoing;
	}
	if (!walking || !destination.has_value() || glm::distance(*destination, handAcross) > k_HandMoved)
	{
		return Lead::GoToHand;
	}
	return Lead::KeepGoing;
}

float creature_leash::FadePull(float pull)
{
	const auto faded = pull * k_PullFade;
	return faded < k_PullGone ? 0.0f : faded;
}

bool creature_leash::FreeOfHome(float distanceFromHome, bool playerHasTemple)
{
	return playerHasTemple && distanceFromHome < k_HomeRange;
}

bool creature_leash::IsConfined(float radius, bool leashed, bool leashWorks)
{
	return radius > 0.0f && !(leashed && !leashWorks);
}

bool creature_leash::OutsideArea(const glm::vec2& position, const glm::vec2& centre, float radius)
{
	return radius > 0.0f && glm::distance(position, centre) > radius;
}

float creature_leash::AttitudeChange(LeashType type, uint32_t turnsTogether)
{
	if (turnsTogether == 0 || turnsTogether % k_AttitudeTurns != 0)
	{
		return 0.0f;
	}
	switch (type)
	{
	case LeashType::Evil:
		return -k_AttitudeStep;
	case LeashType::Good:
		return k_AttitudeStep;
	case LeashType::Rope:
	case LeashType::None:
		break;
	}
	return 0.0f;
}

std::vector<Lesson> creature_leash::LessonsFor(LeashType type, bool objectIsCreature)
{
	const auto kind = objectIsCreature ? Desire::BeFriends : Desire::Compassion;
	switch (type)
	{
	case LeashType::Evil:
		return {{.desire = Desire::Anger, .change = 1.0f}, {.desire = kind, .change = -1.0f}};
	case LeashType::Good:
		return {{.desire = Desire::Anger, .change = -1.0f}, {.desire = kind, .change = 1.0f}};
	case LeashType::Rope:
	case LeashType::None:
		break;
	}
	return {};
}
