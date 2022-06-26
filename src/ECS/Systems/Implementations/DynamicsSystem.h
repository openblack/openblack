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

#include "glm/fwd.hpp"

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

class DynamicsSystemInterface
{
public:
	virtual void Reset() = 0;
	virtual void Update(std::chrono::microseconds& dt) = 0;
	virtual void AddRigidBody(btRigidBody* object) = 0;
	virtual void RegisterRigidBodies() = 0;
	virtual void RegisterIslandRigidBodies(LandIsland& island) = 0;
	virtual void UpdatePhysicsTransforms() = 0;
	[[nodiscard]] virtual std::optional<std::pair<ecs::components::Transform, RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float tMax) const = 0;
};

class DynamicsSystem final: public DynamicsSystemInterface
{
public:
	DynamicsSystem();
	virtual ~DynamicsSystem();

	void Reset() override;
	void Update(std::chrono::microseconds& dt) override;
	void AddRigidBody(btRigidBody* object) override;
	void RegisterRigidBodies() override;
	void RegisterIslandRigidBodies(LandIsland& island) override;
	void UpdatePhysicsTransforms() override;
	[[nodiscard]] std::optional<std::pair<ecs::components::Transform, RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float tMax) const override;

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
