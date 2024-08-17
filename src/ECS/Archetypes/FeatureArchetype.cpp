/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FeatureArchetype.h"

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
#include "Resources/ResourcesInterface.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity FeatureArchetype::Create(const glm::vec3& position, FeatureInfo type, float yAngleRadians, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Game::Instance()->GetInfoConstants().feature.at(static_cast<size_t>(type));

	const auto& transform = registry.Assign<Transform>(entity, position, glm::eulerAngleY(-yAngleRadians), glm::vec3(scale));
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.meshId, transform);
	registry.Assign<Fixed>(entity, point, radius);
	registry.Assign<Feature>(entity, type);
	const auto resourceId = resources::MeshIdToResourceId(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));

	auto l3dMesh = Locator::resources::value().GetMeshes().Handle(resourceId);
	if (l3dMesh->HasPhysicsMesh())
	{
		const auto& templateShape = l3dMesh->GetPhysicsMesh();
		auto shape = std::make_unique<btConvexHullShape>(reinterpret_cast<const btScalar*>(templateShape.getUnscaledPoints()),
		                                                 templateShape.getNumVertices());
		btVector3 bodyInertia(0, 0, 0);
		shape->calculateLocalInertia(l3dMesh->GetMass(), bodyInertia);
		shape->setLocalScaling(btVector3(scale, scale, scale));

		btTransform startTransform;
		startTransform.setIdentity();
		startTransform.setOrigin(btVector3(position.x, position.y, position.z));
		const auto quaternion = btQuaternion(btVector3(0.0f, 1.0f, 0.0f), -yAngleRadians);
		startTransform.setRotation(quaternion);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(l3dMesh->GetMass(), nullptr, shape.get(), bodyInertia);

		registry.Assign<RigidBody>(entity, rbInfo, startTransform, std::move(shape));
	}

	return entity;
}
