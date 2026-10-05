/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "AnimatedStaticArchetype.h"

#include <ECS/Components/Feature.h>
#include <glm/gtx/euler_angles.hpp>

#include "3D/AllMeshes.h"
#include "ECS/Archetypes/VillageLightArchetype.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

namespace
{
/// The Norse gate holds up two town lanterns, 15 units either side of it and 30 up, each with a village light. They
/// stand unturned and aren't part of the gate. The gate is taken by value, as making the lanterns can move the
/// registry's storage.
void CreateNorseGateLanterns(const Transform gate)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto mesh = resources::HashIdentifier(MeshId::ObjectTownLight);
	for (const float side : {-15.0f, 15.0f})
	{
		const glm::vec3 position = gate.position + gate.rotation * (gate.scale * glm::vec3(side, 30.0f, 0.0f));
		const auto lantern = registry.Create();
		registry.Assign<Transform>(lantern, position, glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<Mesh>(lantern, mesh, static_cast<int8_t>(0), static_cast<int8_t>(1));
		VillageLightArchetype::Create(position, VillageLight::Kind::Town);
	}
}
} // namespace

entt::entity AnimatedStaticArchetype::Create(const glm::vec3& position, AnimatedStaticInfo type, float yAngleRadians,
                                             float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Locator::infoConstants::value().animatedStatic.at(static_cast<size_t>(type));

	// The exact same as Feature but info is different and type is a different enum
	const auto& transform = registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.meshId, transform);
	registry.Assign<Fixed>(entity, point, radius);
	// const auto& feature = registry.Assign<Feature>(entity, type);
	const auto resourceId = resources::HashIdentifier(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));

	registry.Assign<AnimatedStatic>(entity, type);

	if (info.meshId == MeshId::BuildingNorseGate)
	{
		CreateNorseGateLanterns(transform);
	}

	return entity;
}
