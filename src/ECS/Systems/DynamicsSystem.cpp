/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "DynamicsSystem.h"

#include <vector>

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "ECS/Components/RigidBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Game.h"

#include "3D/LandBlock.h"
#include "3D/LandIsland.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

DynamicsSystem::DynamicsSystem()
    : _configuration(std::make_unique<btDefaultCollisionConfiguration>())
    , _dispatcher(std::make_unique<btCollisionDispatcher>(_configuration.get()))
    , _broadphase(std::make_unique<btDbvtBroadphase>())
    , _solver(std::make_unique<btSequentialImpulseConstraintSolver>())
    , _world(
          std::make_unique<btDiscreteDynamicsWorld>(_dispatcher.get(), _broadphase.get(), _solver.get(), _configuration.get()))
{
	_world->setGravity(btVector3(0, -10, 0));
}

void DynamicsSystem::Reset()
{
	std::vector<btRigidBody*> to_remove;
	for (int i = 0; i < _world->getNumCollisionObjects(); ++i)
	{
		auto obj = _world->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body)
		{
			to_remove.push_back(body);
		}
	}
	for (auto& obj : to_remove)
	{
		_world->removeRigidBody(obj);
	}
}

DynamicsSystem::~DynamicsSystem() = default;

void DynamicsSystem::Update(std::chrono::microseconds& dt)
{
	std::chrono::duration<float> seconds = dt;
	_world->stepSimulation(seconds.count());
}

void DynamicsSystem::AddRigidBody(btRigidBody* object)
{
	_world->addRigidBody(object);
}

void DynamicsSystem::RegisterRigidBodies()
{
	auto& registry = Game::instance()->GetEntityRegistry();
	registry.Each<RigidBody>([this](RigidBody& body) {
		body.handle.setUserIndex(static_cast<int>(ecs::systems::RigidBodyType::Entity));
		body.handle.setUserIndex2(0); // TODO
		body.handle.setUserPointer(this);
		AddRigidBody(&body.handle);
	});
}

void DynamicsSystem::RegisterIslandRigidBodies(LandIsland& island)
{
	auto& landBlocks = island.GetBlocks();
	for (uint32_t i = 0; i < landBlocks.size(); ++i)
	{
		auto& rigidBody = landBlocks[i].GetRigidBody();
		rigidBody->setUserIndex(static_cast<int>(ecs::systems::RigidBodyType::Terrain));
		rigidBody->setUserIndex2(i);
		rigidBody->setUserPointer(reinterpret_cast<void*>(this));
		AddRigidBody(rigidBody.get());
	}
}

void DynamicsSystem::UpdatePhysicsTransforms()
{
	auto& registry = Game::instance()->GetEntityRegistry();
	registry.Each<Transform, const RigidBody>([&registry](Transform& transform, const RigidBody& body) {
		btTransform trans;
		body.motionState->getWorldTransform(trans);

		transform.position.x = trans.getOrigin().getX();
		transform.position.y = trans.getOrigin().getY();
		transform.position.z = trans.getOrigin().getZ();

		glm::quat quaternion(trans.getRotation().getW(), trans.getRotation().getX(), trans.getRotation().getY(),
		                     trans.getRotation().getZ());

		transform.rotation = glm::mat3_cast(quaternion);

		registry.SetDirty();
	});
}

const std::optional<std::pair<Transform, RigidBodyDetails>>
DynamicsSystem::RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float t_max) const
{
	auto from = btVector3(origin.x, origin.y, origin.z);
	auto to = from + t_max * btVector3(direction.x, direction.y, direction.z);

	btCollisionWorld::ClosestRayResultCallback callback(from, to);

	_world->rayTest(from, to, callback);

	if (!callback.hasHit())
		return std::nullopt;

	auto translation = glm::vec3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());
	auto normal = glm::vec3(callback.m_hitNormalWorld.x(), callback.m_hitNormalWorld.y(), callback.m_hitNormalWorld.z());
	const auto up = glm::vec3(0, 1, 0);
	auto rotation = glm::orientation(normal, up);

	return std::make_optional(std::make_pair(
	    Transform {translation, rotation, glm::vec3(1.0f)},
	    RigidBodyDetails {static_cast<RigidBodyType>(callback.m_collisionObject->getUserIndex()),
	                      callback.m_collisionObject->getUserIndex2(), callback.m_collisionObject->getUserPointer()}));
}
