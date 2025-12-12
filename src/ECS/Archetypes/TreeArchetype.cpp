/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TreeArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "Common/RandomNumberManager.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Swayable.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity TreeArchetype::Create([[maybe_unused]] uint32_t forestId, const glm::vec3& position, TreeInfo type,
                                   [[maybe_unused]] bool isNonScenic, float yAngleRadians, float maxSize, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Locator::infoConstants::value().tree.at(static_cast<size_t>(type));

	const auto& transform = registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.normal, transform);
	registry.Assign<Fixed>(entity, point, radius);
	// A value of 0 disables time-based swaying via wind
	auto swayOffset = Locator::rng::value().NextValue<float>(0.0f, 1.0f);
	auto swayStrength = Locator::rng::value().NextValue<float>(0.3f, 1.0f);
	registry.Assign<Tree>(entity, type, maxSize);
	const auto resourceId = resources::HashIdentifier(info.normal);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(-1));
	registry.Assign<Swayable>(entity, false, swayOffset, 0.0f, swayStrength, swayStrength, glm::vec2(1.0f, 1.0f));

	return entity;
}
