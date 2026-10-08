/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSight.h"

#include "3D/MapCoords.h"
#include "Common/GUtilsAngle.h"
#include "Creature/CreatureLocomotion.h"
#include "Creature/PerceivedDesires.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;

namespace
{
/// With nothing to look at, a creature looks this far ahead of its way
constexpr float k_LookAheadMetres = 10.0f;
} // namespace

CreatureMindState* creature_sight::MindSeeing(PlayerNames player, glm::vec3 point)
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::leashSystem::has_value())
	{
		return nullptr;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto creature = Locator::leashSystem::value().PlayersCreature(player);
	auto* mind = creature.has_value() && registry.Valid(*creature) ? registry.TryGet<CreatureMindState>(*creature) : nullptr;
	const auto* at = mind != nullptr ? registry.TryGet<const Transform>(*creature) : nullptr;
	if (at == nullptr)
	{
		return nullptr;
	}
	// It sees what lies within two thirds of a half turn of where it looks, its head's look if it looks at something
	const auto* animation = registry.TryGet<const CreatureAnimation>(*creature);
	const auto* moving = registry.TryGet<const CreatureLocomotion>(*creature);
	const glm::vec2 here(at->position.x, at->position.z);
	uint16_t look = 0;
	if (animation != nullptr && animation->lookAt.has_value())
	{
		look = gutils::GetAngleFromXZ(here, glm::vec2(animation->lookAt->x, animation->lookAt->z));
	}
	else if (moving != nullptr)
	{
		const auto ahead = creature_locomotion::DirectionOf(moving->heading);
		look = gutils::GetAngleFromXZ(here, here + ahead * k_LookAheadMetres);
	}
	const bool sameCell =
	    map_coords::Cell(map_coords::FromMetres(here)) == map_coords::Cell(map_coords::FromMetres(glm::vec2(point.x, point.z)));
	if (!creature_perceived_desires::CanSeePos(look, gutils::GetAngleFromXZ(here, glm::vec2(point.x, point.z)), sameCell))
	{
		return nullptr;
	}
	return mind;
}

void creature_sight::EmpathiseWithPlayer(PlayerNames player, CreatureDesires desire, float amount, glm::vec3 point)
{
	if (auto* mind = MindSeeing(player, point))
	{
		creature_perceived_desires::Increase(mind->perceivedDesires, static_cast<size_t>(desire), amount);
	}
}
