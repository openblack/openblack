/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FeatureArchetype.h"

#include <BulletCollision/CollisionShapes/btConvexShape.h>
#include <glm/gtx/euler_angles.hpp>

#include "3D/L3DMesh.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/MeshId.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity FeatureArchetype::Create(const glm::vec3& position, FeatureInfo type, float yAngleRadians, float scale)
{
	auto& registry = Game::Instance()->GetEntityRegistry();
	const auto entity = registry.Create();

	const auto& info = Game::Instance()->GetInfoConstants().feature.at(static_cast<size_t>(type));

	const auto& transform = registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.meshId, transform);
	registry.Assign<Fixed>(entity, point, radius);
	registry.Assign<Feature>(entity, type);
	const auto resourceId = resources::MeshIdToResourceId(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));

	auto l3dMesh = Locator::resources::ref().GetMeshes().Handle(resourceId);
	if (l3dMesh->HasPhysicsMesh())
	{
		auto& shape = l3dMesh->GetPhysicsMesh();
		btVector3 bodyInertia(0, 0, 0);
		shape.calculateLocalInertia(l3dMesh->GetMass(), bodyInertia);

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btVector3(transform.position.x, transform.position.y, transform.position.z));

		btRigidBody::btRigidBodyConstructionInfo rbInfo(l3dMesh->GetMass(), nullptr, &shape, bodyInertia);

		registry.Assign<RigidBody>(entity, rbInfo, startTransform);
	}

	return entity;
}
