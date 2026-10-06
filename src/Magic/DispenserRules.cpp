/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "DispenserRules.h"

#include <cmath>

#include <array>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

using namespace openblack::magic;

namespace
{
constexpr std::array k_DispensableMiracles {
    openblack::MagicType::Fireball,
    openblack::MagicType::FireballPowerUpOne,
    openblack::MagicType::FireballPowerUpTwo,
    openblack::MagicType::LightningBolt,
    openblack::MagicType::LightningBoltPowerUpOne,
    openblack::MagicType::LightningBoltPowerUpTwo,
    openblack::MagicType::ExplosionOne,
    openblack::MagicType::ExplosionOnePuOne,
    openblack::MagicType::ExplosionOnePuTwo,
    openblack::MagicType::Heal,
    openblack::MagicType::HealPowerUpOne,
    openblack::MagicType::Teleport,
    openblack::MagicType::Forest,
    openblack::MagicType::Food,
    openblack::MagicType::FoodPowerUpOne,
    openblack::MagicType::StormWindRain,
    openblack::MagicType::StormWindRainLightning,
    openblack::MagicType::Tornado,
    openblack::MagicType::Shield,
    openblack::MagicType::PhysicalShield,
    openblack::MagicType::Wood,
    openblack::MagicType::Water,
    openblack::MagicType::WaterPowerUpOne,
    openblack::MagicType::FlockFlying,
    openblack::MagicType::FlockGround,
    openblack::MagicType::CreatureSpellFreeze,
    openblack::MagicType::CreatureSpellSmall,
    openblack::MagicType::CreatureSpellBig,
    openblack::MagicType::CreatureSpellWeak,
    openblack::MagicType::CreatureSpellStrong,
    openblack::MagicType::CreatureSpellInvisible,
    openblack::MagicType::CreatureSpellCompassion,
    openblack::MagicType::CreatureSpellAngry,
    openblack::MagicType::CreatureSpellItchy,
};
} // namespace

std::span<const openblack::MagicType> openblack::magic::DispensableMiracles()
{
	return k_DispensableMiracles;
}

DispenserStep openblack::magic::StepDispenser(DispenserTimer& timer, bool hasOrb, bool orbStillThere, bool hasMagic)
{
	if (hasOrb)
	{
		if (orbStillThere)
		{
			return DispenserStep::Wait;
		}
		timer.tick = 0;
		return DispenserStep::OrbTaken;
	}
	if (!timer.active || !hasMagic)
	{
		return DispenserStep::Wait;
	}
	if (++timer.tick >= timer.period)
	{
		timer.tick = 0;
		return DispenserStep::MakeOrb;
	}
	return DispenserStep::Wait;
}

uint32_t openblack::magic::PeriodTurns(float seconds, std::chrono::milliseconds turn)
{
	if (seconds <= 0.0f || turn.count() <= 0)
	{
		return 0;
	}
	return static_cast<uint32_t>(seconds * 1000.0f / static_cast<float>(turn.count()));
}

glm::mat3 openblack::magic::FaceTowards(const glm::vec3& direction)
{
	const float length = glm::length(direction);
	if (length <= 0.0f)
	{
		return glm::mat3(1.0f);
	}
	const auto up = direction / length;
	// Any axis not along the direction to build the other two from
	const auto reference = std::abs(up.y) < 0.99f ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(1.0f, 0.0f, 0.0f);
	const auto right = glm::normalize(glm::cross(reference, up));
	const auto forward = glm::cross(right, up);
	return {right, up, forward};
}

glm::vec3 openblack::magic::OrbPosition(const glm::vec3& base, float height)
{
	return base + glm::vec3(0.0f, height * k_OrbHeightShare, 0.0f);
}

bool openblack::magic::OrbStillThere(const glm::vec3& orb, const glm::vec3& made)
{
	return glm::distance(glm::vec2(orb.x, orb.z), glm::vec2(made.x, made.z)) <= k_OrbStillThereDistance;
}
