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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// How the hand rests over what the interface picked under the cursor: a villager or an animal it hovers before, at the
/// thing's distance; a model it feels the triangles of, standing up off the face it meets; anything else it rests over
/// the land, turned to the land's slope.
namespace openblack::hand_feel
{

/// What the hand does with the thing picked
enum class Feel : uint8_t
{
	/// It is no thing to the hand: nothing picked, a vortex, or what the hand already holds
	Nothing,
	/// The hand hovers before it, on the line of sight, at its distance less its radius
	AtPosition,
	/// The hand feels its model's triangles along the line of sight
	OnModel,
};

/// What the hand needs to know of the thing picked
struct Picked
{
	/// A villager, an animal or a creature
	bool living {false};
	bool creature {false};
	/// Its model is posed by bones
	bool posedModel {false};
	/// A gate, plinth, cave or other animated static, felt on its own collision model
	bool animatedStatic {false};
	/// A vortex, or the thing the hand holds
	bool ignored {false};
};
[[nodiscard]] Feel FeelOf(const Picked& picked);

/// The direction the hand feels along: the line of sight through the cursor, or, holding something, the line to the
/// cursor's point on the land raised by the hand's lift for what it holds
[[nodiscard]] glm::vec3 FeelDirection(glm::vec3 camera, glm::vec3 cursorDirection, std::optional<glm::vec3> raisedCursorGround);

/// A creature the hand offers to: its middle across the ground and its radius
struct CreatureReach
{
	glm::vec2 centre;
	float radius;
};
/// What the hand holds while it feels a model: the held thing's radius across the ground, and the creature it may be
/// offered to
struct Holding
{
	float radius {0.0f};
	std::optional<CreatureReach> creature;
};

/// Where the hand rests on a model and the up it turns to
struct Rest
{
	glm::vec3 point;
	/// None turns the hand to the land's slope instead
	std::optional<glm::vec3> up;
};
/// The hand meeting a model: the point and the face's normal along the direction felt. The point is brought onto the
/// line of sight at the same distance when the hand holds something; nearer than a unit from the camera it is a unit
/// along the line of sight. The hand stands up off the face, leaning a quarter towards the camera and half upwards;
/// holding something, it is kept out from the face by half the held thing's radius, and further by how deep in the
/// creature's reach it is. A model that isn't felt (trees, dead trees, totem statues, vortices, seeds and shields) gives
/// the point but turns the hand to the land.
[[nodiscard]] Rest RestOnModel(glm::vec3 camera, glm::vec3 cursorDirection, glm::vec3 feltDirection, glm::vec3 hitPoint,
                               glm::vec3 hitNormal, bool feelsModel, std::optional<Holding> holding);

/// The hand hovering before a villager or an animal: on the line of sight at the thing's distance from the camera less
/// its radius, moved on along the line by how far that point is above or below the thing times the felt direction's
/// rise
[[nodiscard]] glm::vec3 RestAtPosition(glm::vec3 camera, glm::vec3 cursorDirection, glm::vec3 feltDirection, glm::vec3 position,
                                       float radius);

/// Whether the hand turns to the land's slope where it is: when it rests on no face, or when over a thing it is lower
/// above the land than half its own height
[[nodiscard]] bool TurnsToLand(bool restsOnFace, bool overThing, float aboveLand, float handHeight);

} // namespace openblack::hand_feel
