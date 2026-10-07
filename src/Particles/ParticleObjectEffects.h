/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

/// What the rules of the destructive miracles do to the world's objects: the burning object a fireball carries, and a
/// blast's spot visuals, its wave through the objects round it, the pieces it breaks them into and the marks it leaves.
/// The game backs it with its systems; tests use a fake.
namespace openblack::particles
{

/// One object a blast's wave may reach
struct WaveTarget
{
	entt::entity object {entt::null};
	/// Its point: where it stands, at its height above the land
	glm::vec3 point {0.0f};
	/// Its radius across the ground
	float radius {0.0f};
};

/// A model to break into pieces: its triangles in the world, and where the blast throws them from and how fast
struct ExplodeRecord
{
	entt::id_type mesh {0};
	glm::mat4 transform {1.0f};
	glm::vec3 origin {0.0f};
	float speed {0.0f};
};

class ObjectEffectsInterface
{
public:
	ObjectEffectsInterface() = default;
	ObjectEffectsInterface(const ObjectEffectsInterface&) = delete;
	ObjectEffectsInterface& operator=(const ObjectEffectsInterface&) = delete;
	ObjectEffectsInterface(ObjectEffectsInterface&&) = delete;
	ObjectEffectsInterface& operator=(ObjectEffectsInterface&&) = delete;
	virtual ~ObjectEffectsInterface() = default;

	// The fireball

	/// The burning object a fireball's atom carries, as hot as its miracle is strong; none when it can't be made
	virtual std::optional<entt::entity> AttachFireBall(glm::vec3 position, float strength, float radius, bool affectedByRain,
	                                                   std::optional<PlayerNames> player) = 0;
	enum class FireBallState : uint8_t
	{
		/// Its fire has gone, and the ball with it
		Gone,
		/// It has cooled below the heat that keeps a ball going
		Cooled,
		Burning,
	};
	/// The burning object follows its atom, its heat capacity by the miracle's strength now; how its fire is
	/// The hand may take hold of it at a point, none while the particle is too faint
	virtual FireBallState FollowFireBall(entt::entity ball, glm::vec3 position, float radius, float strength,
	                                     std::optional<glm::vec3> handTarget) = 0;

	// The blast

	/// A spot visual for some turns at a point, at a magnitude; 0 when there is none
	virtual uint32_t StartSpotVisual(SpotVisualType type, glm::vec3 position, float magnitude, int turns) = 0;
	virtual void CloseSpotVisual(uint32_t id) = 0;
	/// The fixed objects within a range of a centre, each with its home in a spiral of the land's cells out from the
	/// centre's, roughly nearest first
	[[nodiscard]] virtual std::vector<WaveTarget> FixedObjectsNear(glm::vec3 centre, float range) const = 0;
	/// Where an object still in the world stands; none once it has gone
	[[nodiscard]] virtual std::optional<WaveTarget> Available(entt::entity object) const = 0;
	/// Whether a miracle may destroy it: not a creature, nor a field, nor a part of a citadel
	[[nodiscard]] virtual bool CanBeDestroyedBySpell(entt::entity object) const = 0;
	[[nodiscard]] virtual bool IsCreature(entt::entity object) const = 0;
	/// The object breaks into pieces thrown from the origin at the speed
	virtual void ExplodeObject(entt::entity object, glm::vec3 origin, float speed) = 0;
	/// A model, placed so, breaks into pieces thrown from the origin at the speed
	virtual void ExplodeMesh(entt::id_type mesh, const glm::mat4& transform, glm::vec3 origin, float speed) = 0;
	/// The pieces waiting to be made in a queue, taken by the effect that throws them. The blasts fill the first.
	[[nodiscard]] virtual std::vector<ExplodeRecord> TakeExplodeRecords(int queue) = 0;
	/// A beam destroys the object: a building is left with no life, anything else is removed
	virtual void DestroyByBeam(entt::entity object) = 0;
	/// The heap of rubble a blast leaves on the land, with a puff of dust
	virtual void AddRubbleMark(glm::vec3 centre) = 0;
	/// A ring on the water growing to a size
	virtual void AddWaterRing(glm::vec3 centre, float size) = 0;
	/// The camera shakes while within a radius of a point, from a strength down to none over the seconds
	virtual void ShakeCamera(glm::vec3 position, float radius, float strength, float seconds) = 0;
	[[nodiscard]] virtual bool IsDryLand(glm::vec3 point) const = 0;
};

} // namespace openblack::particles
