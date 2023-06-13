/*******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <optional>
#include <tuple>

#include <glm/fwd.hpp>

class btRigidBody;

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

} // namespace openblack::ecs::systems
