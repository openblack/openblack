/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "StreamSegmentArchetype.h"

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "ECS/Components/Stream.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity StreamSegmentArchetype::Create(const glm::vec3& from, const glm::vec3& to)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	// The meshes' x turns to point from one point to the next across the ground, and stretches to reach it
	const float angle = std::atan2(to.z - from.z, to.x - from.x);
	const float stretch = glm::distance(from, to) / StreamSegment::k_MeshLength;
	registry.Assign<Transform>(entity, from, glm::eulerAngleY(-angle), glm::vec3(stretch, 1.0f, 1.0f));
	registry.Assign<StreamSegment>(entity);

	return entity;
}
