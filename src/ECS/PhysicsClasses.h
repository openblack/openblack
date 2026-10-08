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

#include <entt/entity/fwd.hpp>

#include "3D/AllMeshes.h"
#include "Physics/Materials.h"

namespace openblack
{
namespace v120
{
struct InfoConstants;
}
using InfoConstants = v120::InfoConstants;
namespace ecs
{
class Registry;
}
} // namespace openblack

/// What the physics makes of each kind of object: which material it is made of, how heavy it is, whether it can fly,
/// whether flying things hit it, and the shape of its body.
namespace openblack::ecs::physics_classes
{

/// The shape a kind of object's body takes
enum class BodyKind : uint8_t
{
	/// No body: the physics passes it by
	None,
	/// The model's own points and triangles
	Model,
	/// A spindle along the trunk
	Tree,
	VillagerBox,
	AnimalBox,
	/// The creature's skeleton, as ellipsoids
	Creature,
	/// The physical shield's dome, as it stands
	ShieldDome,
	/// A gate, the piper's cave or the phone box: a collision model of its own, picked by its state
	CollisionModel,
	/// A piece broken off a building: a slab of its triangles
	BuildingPiece,
};

/// What the physics knows of an object's kind
struct ClassFacts
{
	BodyKind body {BodyKind::None};
	physics::MaterialRow row {physics::MaterialRow::DefaultUnmovable};
	/// It can be thrown, dropped or knocked into flight
	bool canBecomePhysicsObject {false};
	/// Flying bodies hit it: it becomes a resting obstacle when something moves near it
	bool interacts {false};
	/// Its own points are tested against other bodies' faces; buildings and the temple's heart only take hits
	bool checksPoints {true};
	/// It stays in the physics whether or not anything moves near it (the physical shield)
	bool alwaysStays {false};
	/// Thrown, it breaks buildings: rocks and the bowling ball
	bool physicallyDestroysAbodes {false};
	/// A standing tree, whose centre of mass is lower and whose spindle reaches below its base
	bool rooted {false};
	/// The weight of a kind whose body has a weight of its own, whatever its size: buildings, the temple's heart and the
	/// creature
	std::optional<float> fixedMass;
	/// Moved by bones: drawn turned a quarter about its up axis from its body
	bool animated {false};
	/// Turned by the physics only about its up axis: it stands upright wherever it comes to rest
	bool upright {false};
	/// A script holds it where it is: it is never thrown, dropped or knocked into flight
	bool immovable {false};
	/// Its body moves: a villager's, an animal's, a tree's and a building piece's always do; a model's only when its object
	/// can become a physics object and isn't held immovable
	bool dynamic {false};
	/// The weight of its body is never raised to the least a body weighs (a tree's isn't)
	bool unclampedMass {false};
	/// The model a gate, the piper's cave or the phone box collides with
	std::optional<uint32_t> collisionMesh;
};

/// What deciding an object's kind needs from the rest of the game
struct ClassInputs
{
	/// Its life, 0 to 1
	float life {1.0f};
	/// How much of a building is built, 0 to 1
	float percentBuilt {1.0f};
	/// A villager that can be reached: not at home, not in a hand, not hiding in a building
	bool villagerReachable {true};
	/// A pile that is part of a storage pit
	bool partOfStoragePit {false};
	/// A script holds it where it is: it never becomes a physics object, and a model's body never moves
	bool immovable {false};
};

/// The kind of an object, from its components and its table row
[[nodiscard]] ClassFacts Classify(const Registry& registry, entt::entity entity, const InfoConstants& info,
                                  const ClassInputs& inputs);

/// Whether a static's model is one of the toys the hand plays with
[[nodiscard]] bool IsToyModel(MeshId mesh);
/// Whether a static's model is a fence
[[nodiscard]] bool IsFenceModel(MeshId mesh);

/// An object's weight: its table weight times its scale cubed
[[nodiscard]] float Weight(float infoWeight, float scale);
/// A body's mass from its object's weight: never below a hundredth
[[nodiscard]] float BodyMass(float weight);

} // namespace openblack::ecs::physics_classes
