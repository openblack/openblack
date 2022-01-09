/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FieldArchetype.h"

#include "AbodeArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Town.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "InfoConstants.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity FieldArchetype::Create(int townId, const glm::vec3& position, FieldTypeInfo type, float yAngleRadians)
{
	auto& registry = Game::instance()->GetEntityRegistry();

	[[maybe_unused]] const auto& info = Game::instance()->GetInfoConstants().fieldType[static_cast<size_t>(type)];

	auto townTribe = registry.Get<Tribe>(registry.Context().towns[townId]);
	auto abodeInfo = GAbodeInfo::Find(townTribe, AbodeNumber::Field);

	auto entity = AbodeArchetype::Create(townId, position, abodeInfo, yAngleRadians, 1.0f, 0, 0);
	registry.Assign<Field>(entity, townId);

	return entity;
}
