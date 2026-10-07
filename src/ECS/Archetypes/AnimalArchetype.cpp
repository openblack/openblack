/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AnimalArchetype.h"

#include <glm/vec3.hpp>

#include "Animals/AnimalRules.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity AnimalArchetype::Create(AnimalInfo type, const glm::vec3& position, float heading, float scale, PlayerNames owner)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& info = Locator::infoConstants::value().animal.at(static_cast<size_t>(type));
	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, animals::Orientation(heading, 0.0f), glm::vec3(scale));
	registry.Assign<Mesh>(entity, resources::HashIdentifier(info.high), static_cast<int8_t>(0), static_cast<int8_t>(0));
	auto& animal = registry.Assign<Animal>(entity);
	animal.type = type;
	animal.owner = owner;
	animal.heading = heading;
	animal.previousHeading = heading;
	animal.position = position;
	animal.previousPosition = position;
	animal.animation = info.defaultAnim;
	return entity;
}
