/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AnimalRules.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "3D/MapCoords.h"
#include "Common/GUtilsAngle.h"

using namespace openblack;

namespace
{
constexpr float k_Pi = std::numbers::pi_v<float>;
constexpr float k_TwoPi = 2.0f * k_Pi;
/// The tables count a full turn as this
constexpr float k_FullTurn = 2048.0f;
} // namespace

float animals::TurnAngleRadians(int turnAngle)
{
	return static_cast<float>(turnAngle) * k_TwoPi / k_FullTurn;
}

std::optional<AnimId> animals::DyingClip(AnimalInfo type)
{
	switch (type)
	{
	case AnimalInfo::Lion:
		return AnimId::ALionDie;
	case AnimalInfo::Leopard:
		return AnimId::ALeopardDie;
	case AnimalInfo::Tiger:
		return AnimId::ATigerDie;
	case AnimalInfo::Wolf:
		return AnimId::AWolfDie;
	case AnimalInfo::Cow:
		return AnimId::ACowDie;
	case AnimalInfo::Sheep:
		return AnimId::ASheepFallDeadlhs;
	case AnimalInfo::Horse:
		return AnimId::AHorseDeadlhs;
	case AnimalInfo::Pig:
		return AnimId::APigFallDeadlhs;
	case AnimalInfo::Tortoise:
		return AnimId::ATortoiseStand;
	default:
		return std::nullopt;
	}
}

std::optional<AnimId> animals::DeadClip(AnimalInfo type)
{
	// The big cats and wolves lie in their sleeping clip; the farm animals killed lie on their left, the one of their two
	// dead clips the killed take
	switch (type)
	{
	case AnimalInfo::Lion:
		return AnimId::ALionSleep;
	case AnimalInfo::Leopard:
		return AnimId::ALeopardSleep;
	case AnimalInfo::Tiger:
		return AnimId::ATigerSleep;
	case AnimalInfo::Wolf:
		return AnimId::AWolfSleep;
	case AnimalInfo::Cow:
		return AnimId::ACowDeadOnLhs;
	case AnimalInfo::Sheep:
		return AnimId::ASheepDeadLhs;
	case AnimalInfo::Horse:
		return AnimId::AHorseDeadlhs;
	case AnimalInfo::Pig:
		return AnimId::APigDeadlhs;
	case AnimalInfo::Tortoise:
		return AnimId::ATortoiseStand;
	default:
		return std::nullopt;
	}
}

float animals::BirthScale(uint32_t age, uint32_t grownUpAge, std::span<const float> ageToScale,
                          const std::function<float(float)>& random)
{
	constexpr float k_GrownSize = 0.9f;
	constexpr float k_GrownSpread = 0.05f;
	constexpr float k_GrownSpreadRange = 0.1f;
	constexpr double k_GrowingShare = 0.75;
	if (age < grownUpAge && age >= 1 && age + 1 < ageToScale.size())
	{
		// The table's size of its age, grown up to three quarters of the way to the size two years on
		const float size = ageToScale[age - 1];
		const auto towards = static_cast<float>(static_cast<double>(ageToScale[age + 1] - size) * k_GrowingShare);
		return size + random(towards);
	}
	// Grown: a size about a whole, which beats nine tenths, so it is drawn again
	float size = k_GrownSize;
	const float first = (k_GrownSpread - random(k_GrownSpreadRange)) + 1.0f;
	if (!(first <= size))
	{
		size = (k_GrownSpread - random(k_GrownSpreadRange)) + 1.0f;
	}
	return size;
}

glm::mat3 animals::Orientation(float heading, float bank)
{
	return glm::mat3(glm::eulerAngleY(-(heading + (k_Pi * 0.5f))) * glm::eulerAngleZ(bank));
}

animals::FormationSlot animals::FormationSlotOf(int place)
{
	int k = place;
	int b = 1;
	if (k >= 1)
	{
		do
		{
			++b;
		} while (k >= b * b);
	}
	int sign = 1;
	if (k % 2 == 0)
	{
		k -= 1;
		sign = -1;
	}
	return {.row = b - 5, .column = (((b * b) - k) * sign + 1) / 2};
}

glm::vec2 animals::FormationGoal(glm::vec2 leader, glm::vec2 follower, FormationSlot slot)
{
	// The slot's angle plus the way from the leader to the follower, in game angles
	const auto slotAngle = gutils::GetAngleFromDXDZ(slot.row, slot.column);
	const auto away = gutils::GetAngleFromXZ(follower, leader) + 0x400;
	const auto angle = static_cast<double>(gutils::ConvertGameAngleTo3D(static_cast<int32_t>(slotAngle + away)));
	// The row's spacing along the angle's x and the column's along its z, each truncated to a map position
	const double spacing = k_FormationSpacing;
	const auto x =
	    static_cast<float>(map_coords::ToMetres(map_coords::ToFixed(leader.x)) + (std::cos(angle) * slot.row * spacing));
	const auto z =
	    static_cast<float>(map_coords::ToMetres(map_coords::ToFixed(leader.y)) + (std::sin(angle) * slot.column * spacing));
	return {map_coords::ToMetres(map_coords::ToFixedGUtils(x)), map_coords::ToMetres(map_coords::ToFixedGUtils(z))};
}
