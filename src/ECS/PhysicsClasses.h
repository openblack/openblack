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
};

/// The kind of an object, from its components and its table row
[[nodiscard]] ClassFacts Classify(const Registry& registry, entt::entity entity, const InfoConstants& info,
                                  const ClassInputs& inputs);

/// An object's weight: its table weight times its scale cubed
[[nodiscard]] float Weight(float infoWeight, float scale);
/// A body's mass from its object's weight: never below a hundredth
[[nodiscard]] float BodyMass(float weight);

} // namespace openblack::ecs::physics_classes
