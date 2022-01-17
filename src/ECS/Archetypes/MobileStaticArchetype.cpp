/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "MobileStaticArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "3D/L3DMesh.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/MeshId.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity MobileStaticArchetype::Create(const glm::vec3& position, MobileStaticInfo type, float altitude,
                                           float xAngleRadians, float yAngleRadians, float zAngleRadians, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();

	const auto& info = Game::Instance()->GetInfoConstants().mobileStatic.at(static_cast<size_t>(type));

	const glm::vec3 offset(0.0f, altitude, 0.0f);

	registry.Assign<Transform>(entity, position + offset, glm::eulerAngleXYZ(-xAngleRadians, -yAngleRadians, -zAngleRadians),
	                           glm::vec3(scale));
	registry.Assign<Mobile>(entity);
	registry.Assign<MobileStatic>(entity, type);
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
		const auto quaternion =                                         // EulerAngleXYZ
		    btQuaternion(btVector3(1.0f, 0.0f, 0.0f), -xAngleRadians) * //
		    btQuaternion(btVector3(0.0f, 1.0f, 0.0f), -yAngleRadians) * //
		    btQuaternion(btVector3(0.0f, 0.0f, 1.0f), -zAngleRadians);
		startTransform.setRotation(quaternion);

		btRigidBody::btRigidBodyConstructionInfo rbInfo(l3dMesh->GetMass(), nullptr, shape.get(), bodyInertia);

		registry.Assign<RigidBody>(entity, rbInfo, startTransform, std::move(shape));
	}

	return entity;
}
