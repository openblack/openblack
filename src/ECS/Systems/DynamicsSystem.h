/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <chrono>
#include <memory>
#include <optional>
#include <tuple>

#include <glm/fwd.hpp>

class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btDiscreteDynamicsWorld;
struct btDbvtBroadphase;
class btRigidBody;
class btSequentialImpulseConstraintSolver;

namespace openblack
{
class LandIsland;
namespace ecs::components
{
struct Transform;
}
} // namespace openblack

namespace openblack::ecs::systems
{
enum class RigidBodyType
{
	Terrain,
	Entity,
};

struct RigidBodyDetails
{
	RigidBodyType type;
	int id;
	const void* userData;
};

class DynamicsSystem
{
public:
	DynamicsSystem();
	virtual ~DynamicsSystem();

	void Reset();
	void Update(std::chrono::microseconds& dt);
	void AddRigidBody(btRigidBody* object);
	void RegisterRigidBodies();
	void RegisterIslandRigidBodies(LandIsland& island);
	void UpdatePhysicsTransforms();
	const std::optional<std::pair<ecs::components::Transform, RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float t_max) const;

private:
	/// collision configuration contains default setup for memory, collision setup
	std::unique_ptr<btDefaultCollisionConfiguration> _configuration;
	/// use the default collision dispatcher. For parallel processing you can use
	/// a different dispatcher (see Extras/BulletMultiThreaded)
	std::unique_ptr<btCollisionDispatcher> _dispatcher;
	std::unique_ptr<btDbvtBroadphase> _broadphase;
	/// the default constraint solver. For parallel processing you can use a
	/// different solver (see Extras/BulletMultiThreaded)
	std::unique_ptr<btSequentialImpulseConstraintSolver> _solver;
	std::unique_ptr<btDiscreteDynamicsWorld> _world;
};
} // namespace openblack::ecs::systems
