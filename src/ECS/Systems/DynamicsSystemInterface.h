/*******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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
class LandIslandInterface;
namespace ecs::components
{
struct Transform;
}

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

} // namespace openblack

namespace openblack::ecs::systems
{
class DynamicsSystemInterface
{
public:
	virtual void Reset() = 0;
	virtual void Update(std::chrono::microseconds& dt) = 0;
	virtual void AddRigidBody(btRigidBody* object) = 0;
	virtual void RegisterRigidBodies() = 0;
	virtual void RegisterIslandRigidBodies(LandIslandInterface& island) = 0;
	virtual void UpdatePhysicsTransforms() = 0;
	[[nodiscard]] virtual std::optional<std::pair<ecs::components::Transform, RigidBodyDetails>>
	RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float tMax) const = 0;
};

} // namespace openblack::ecs::systems
