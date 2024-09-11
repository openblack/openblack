/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include "ECS/Systems/DynamicsSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#pragma message("Locator interface implementations should only be included in Locator.cpp, use interface instead.")
#endif

class btCollisionDispatcher;
class btDefaultCollisionConfiguration;
class btDiscreteDynamicsWorld;
struct btDbvtBroadphase;
class btSequentialImpulseConstraintSolver;

namespace openblack
{
class LandIslandInterface;
namespace ecs::components
{
struct Transform;
}
} // namespace openblack

namespace openblack::ecs::systems
{

class DynamicsSystem final: public DynamicsSystemInterface
{
public:
	DynamicsSystem();
	virtual ~DynamicsSystem();

	void Reset() override;
	void Update(std::chrono::microseconds& dt) override;
	void AddRigidBody(btRigidBody* object) override;
	void RegisterRigidBodies() override;
	void RegisterIslandRigidBodies(LandIslandInterface& island) override;
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
