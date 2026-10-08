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

#include <glm/mat3x3.hpp>

#include "3D/AllMeshes.h"
#include "Enums.h"

/// The rules for people, animals and creatures as the physics throws, knocks and lands them: how they lie when they come
/// down, the clips they play, how hard a knock hurts them, how long they struggle in the sea, and how they react to
/// things flying near them.
namespace openblack::physics::living
{

/// How a villager or animal lies when it comes down, by how its body leans sideways at the end of its flight
enum class LandingPose : uint8_t
{
	/// On its feet
	Feet,
	/// A villager on its front, an animal on its right side
	Front,
	/// A villager on its back, an animal on its left side
	Back,
	/// Put down with no body: no pose of its own
	None,
};

/// How far the body's side axis must point up or down for it to have landed on its back or front
inline constexpr float k_LyingLean = 0.5f;

/// A villager's landing pose from the up component of its body's side axis at the start of the turn it came to rest
[[nodiscard]] LandingPose VillagerLandingPose(float sideUp);
/// An animal's: the villager's mapping turned the other way round
[[nodiscard]] LandingPose AnimalLandingPose(float sideUp);

/// The heading, in radians, a villager stands up with: along its body's up axis when it lies on its front, opposite
/// it when on its back, and opposite its forward axis when on its feet. Axes are the body's turn-start columns: side,
/// up, forward.
[[nodiscard]] float VillagerLandingHeading(LandingPose pose, const glm::mat3& axes);
/// An animal stands up facing opposite its body's current forward axis
[[nodiscard]] float AnimalLandingHeading(const glm::mat3& axes);
/// The heading of a direction across the land, as the game measures it from an axis
[[nodiscard]] float HeadingOf(glm::vec3 axis);
/// An angle brought into the game's range of headings
[[nodiscard]] float WrapHeading(float radians);

/// The clip a villager plays while it flies: dead, carried by a vortex, or thrown
[[nodiscard]] AnimId VillagerThrownClip(bool alive, bool inVortex);
/// The clip a villager plays as it lands, by its pose, and whether it carries something on its feet
[[nodiscard]] AnimId VillagerLandedClip(LandingPose pose, bool carrying);

/// An animal kind's clips while thrown, held, and landed in each pose; none keeps its current clip
struct AnimalClips
{
	std::optional<AnimId> thrown;
	std::optional<AnimId> inHand;
	std::optional<AnimId> landedFeet;
	std::optional<AnimId> landedRight;
	std::optional<AnimId> landedLeft;
};
[[nodiscard]] AnimalClips ClipsOf(AnimalInfo kind);
/// The landing clip of an animal kind in a pose
[[nodiscard]] std::optional<AnimId> AnimalLandedClip(AnimalInfo kind, LandingPose pose);

/// A knock harder than this many times a living thing's own weight hurts it
inline constexpr float k_HurtingKnock = 2.0f;
/// The share of the game's crush effect each multiple of its weight past that does
inline constexpr float k_CrushPerKnock = 0.03f;
/// How much of the crush effect a knock of so many times its weight does to a villager or animal, none for a softer one
[[nodiscard]] std::optional<float> LivingCrush(float knock);

/// A creature's mass from its size and how fat and how strong its body is drawn (each -1 to 1)
[[nodiscard]] float CreatureMass(float scale, float thinFat, float weakStrong);
/// The most a creature counts a knock as, in multiples of its weight
inline constexpr float k_CreatureKnockCap = 100.0f;
/// The share of the crush effect each multiple does to a creature, and the least that counts
inline constexpr float k_CreatureCrushPerKnock = 0.01f;
inline constexpr float k_CreatureLeastCrush = 0.005f;
/// How much of the crush effect a turn's mean force does to a creature of a mass, none for less than counts
[[nodiscard]] std::optional<float> CreatureCrush(float impact, float creatureMass);

/// A drowning villager's countdown, a turn on: what is left, and whether it dies this turn. An indestructible one is
/// held at 10 so it never gets there.
struct DrowningStep
{
	uint16_t left {0};
	bool dies {false};
};
inline constexpr uint16_t k_IndestructibleDrowning = 10;
[[nodiscard]] DrowningStep StepDrowning(uint16_t left, bool indestructible);

/// What a villager does about a thing flying at it
enum class FlyingObjectResponse : uint8_t
{
	/// It runs: the thing is nearer than it flies in two seconds
	Run,
	/// It points at it
	Point,
};
[[nodiscard]] FlyingObjectResponse RespondToFlyingObject(float distance, float speed);
/// Whether an animal flees a flying thing: the same nearness; otherwise it takes no notice
[[nodiscard]] bool AnimalFleesFlyingObject(float distance, float speed);
/// The clip a villager points with: a woman or child is scared stiff one time in three (first roll of three is 0),
/// otherwise the second roll of three picks looking, standing or pointing and talking
[[nodiscard]] AnimId PointingClip(bool womanOrChild, uint32_t firstRoll, uint32_t secondRoll);

/// The rows of the deeds creatures copy that the physics reports
inline constexpr uint32_t k_DeedDamageByThrowing = 15;
inline constexpr uint32_t k_DeedDamageByThrowingAt = 16;
inline constexpr uint32_t k_DeedThrowInTheSea = 21;

} // namespace openblack::physics::living
