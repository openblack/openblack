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

#include <glm/vec3.hpp>

#include "3D/AllMeshes.h"
#include "Enums.h"

/// The rules of the god hand taking hold of things, carrying them and letting them go: what it may pick up and how long
/// it waits to, how each kind of thing hangs in it, the spring that drags the hand along with what it holds, and whether
/// a release is a throw, a put-down or a pour. Pure functions, tested on their own; the hand's system applies them.
namespace openblack::hand_grab
{

/// A press shorter than this is a tap; untuggable things and things in flight are only taken once it is held this long
inline constexpr uint32_t k_GrabWaitMs = 225;
/// A game turn, in milliseconds, as the wait counts the turns gone by
inline constexpr uint32_t k_TurnMs = 100;
/// The hand's fade into its pulling pose; a pull only starts once it is over
inline constexpr float k_PullBlendSeconds = 0.13f;
/// Rocks wider than this across the ground can't be picked up
inline constexpr float k_RockMaxPickUpRadius = 3.6f;

/// The hand's spring, dragging the hand after where it should be while it is ready to throw
inline constexpr float k_SpringStiffness = 260.0f;
inline constexpr float k_SpringDamping = 40.0f;
/// The spring is stepped in steps of this many milliseconds of game time
inline constexpr uint32_t k_SpringStepMs = 10;
/// Nothing leaves the hand faster than this
inline constexpr float k_MaxThrowSpeed = 124.0f;

/// A release is a throw when its speed across the ground, squared, is more than this; slower it is a put-down
inline constexpr float k_HandThrowSpeedSquared = 4.0f;
/// The same for what a creature lets go of
inline constexpr float k_CreatureThrowSpeedSquared = 1.0f;
/// A pot let go no faster than this (its whole speed squared) is poured out where it is
inline constexpr float k_PourSpeedSquared = 5.0f;
/// A release faster than this (squared) is laid along the slope only when it is a tree
inline constexpr float k_FastReleaseSpeedSquared = 1.0f;

/// After a throw the hand's motion gives what it threw a twist: this long after letting go...
inline constexpr uint32_t k_ReleaseSpinDelayMs = 180;
/// ...by this much times its mass and speed, for one game turn
inline constexpr float k_ReleaseSpinFactor = 1.6f;

/// What the hand holds hangs this far below it on the palm
inline constexpr float k_AboveHang = 0.2f;
/// What it holds any other way hangs at least this far below it
inline constexpr float k_MinimumHang = 1.9f;
/// A rooted thing (a standing tree) hangs a further share of its height lower
inline constexpr float k_RootedHangShare = 0.1f;
/// The point the hand picks on the land under the cursor is raised by this share of the hang
inline constexpr float k_CursorRaiseShare = 0.6f;
/// What the hand takes hangs at least this share of the hand's own height below it
inline constexpr float k_LeastLoweringShare = 0.3f;
/// The hand's standard height
inline constexpr float k_StandardHandHeight = 3.2f;

/// Put down tilted more than this (radians, either way), a tree isn't planted again
inline constexpr float k_UprightTilt = 0.2f;
/// People, animals and fences put down on ground steeper than this (the up of the land's normal) slide off instead
inline constexpr float k_LeastLandingNormalY = 0.7f;
/// A put-down stands where it is over dry land, or where the nearest cell stands higher than this
inline constexpr uint8_t k_LowestLandingAltitude = 1;

/// The kinds of thing as the hand treats them
enum class GrabKind : uint8_t
{
	/// Nothing the hand may take
	None,
	Villager,
	Animal,
	/// A rock, a heavy static or a dead tree: refused only when a rock wider than the hand can lift
	Rock,
	/// Any other static: toys, fences, totems, stones
	MobileStatic,
	/// Loose things: pots, balls, mushrooms, logs
	MobileObject,
	/// The one loose thing held on the palm
	Poo,
	Tree,
	DeadTree,
};

/// What deciding whether the hand may take a thing needs to know of it
struct Holdable
{
	GrabKind kind {GrabKind::None};
	/// It is still in the world, not being taken away
	bool available {true};
	/// A villager inside its home
	bool atHome {false};
	/// Held by a hand already
	bool inHand {false};
	/// A villager on its way to hide in a building
	bool hiding {false};
	/// An animal whose kind the player may pick up
	bool speciesAllows {false};
	/// Its radius across the ground, at its scale
	float radius {0.0f};
	/// A rock (not another heavy static): its size limits it
	bool isRock {false};
};

/// Whether a thing of its kind says the hand may hold it
[[nodiscard]] bool ValidForPlaceInHand(const Holdable& thing);

/// The checks a press goes through, in the game's order, before the hand takes a thing
struct Gate
{
	bool spaceInHand {true};
	bool alreadyInHand {false};
	bool valid {false};
	/// A script said it can't be picked up
	bool cannotBePickedUp {false};
	/// Something else carries it (a tornado)
	bool carried {false};
	/// The hand is in its player's influence (or the thing doesn't need it to be)
	bool inInfluence {true};
};
/// Whether a press on a thing takes it; otherwise the press is a tap
[[nodiscard]] bool PassesGate(const Gate& gate);

/// How long a press has been held: by the clock, but no more than the game turns gone by allow
[[nodiscard]] uint32_t ElapsedMs(uint32_t nowMs, uint32_t pressMs, uint32_t turn, uint32_t pressTurn);

/// How a kind of thing hangs in the hand
struct HoldFacts
{
	HoldType type {HoldType::Above};
	/// Its height times this is how far its model hangs below the hand
	float loweringMultiplier {0.0f};
	/// How far it spreads out of the hand, which opens the hand's pose
	float holdRadius {0.0f};
};
/// How a thing hangs: from its kind, its static's row and model, and its size at its scale
[[nodiscard]] HoldFacts HoldOf(GrabKind kind, MobileStaticInfo staticType, MeshId mesh, float height, float radius);

/// How far a thing hangs below the hand as it is taken: its own hang, but no closer than a share of the hand's height
[[nodiscard]] float PickUpLowering(float loweringMultiplier, float height, float handSize);
/// How far the hand rises for what it holds: a little for the palm, else its hang at the least; a standing tree a share
/// of its height more
[[nodiscard]] float HandRise(HoldType hold, float pickUpLowering, float handSize, std::optional<float> rootedHeight);
/// How far the point the hand picks on the land is raised for what it holds
[[nodiscard]] float CursorRaise(float handRise);

/// The spring that drags the hand after where it should be while it is ready to throw. It is stepped in fixed steps of
/// game time, at least one each frame however short, and what it holds leaves the hand at its speed.
class HandSpring
{
public:
	/// The spring takes hold at the hand, at rest
	void Start(glm::vec3 position);
	/// A frame of some game milliseconds towards where the hand should be
	void Step(glm::vec3 target, uint32_t frameMs);
	[[nodiscard]] glm::vec3 Position() const { return _position; }
	[[nodiscard]] glm::vec3 Velocity() const { return _velocity; }

private:
	glm::vec3 _position {0.0f};
	glm::vec3 _velocity {0.0f};
	uint32_t _elapsedMs {0};
	uint32_t _steppedMs {0};
};

/// Whether a release at a velocity is a throw rather than a put-down (a creature's at a lower speed)
[[nodiscard]] bool IsThrow(glm::vec3 velocity, bool byCreature);
/// Whether a release is fast enough that only a tree is laid along the slope as it leaves the hand
[[nodiscard]] bool IsFastRelease(glm::vec3 velocity);
/// Whether a pot let go at a velocity is poured out where it is
[[nodiscard]] bool PotPours(glm::vec3 velocity);
/// The twist a throw gets from how far the hand moved since letting go, for a body of a mass at a speed: a turning force
/// about the level axis across the hand's motion
[[nodiscard]] glm::vec3 ReleaseSpinTorque(float mass, float speed, glm::vec3 handMoved);

/// What deciding whether a put-down stands where it is needs
struct Landing
{
	bool thrown {false};
	/// It had to be raised over something under it
	bool raised {false};
	/// A villager let go by a computer player stands even when raised
	bool computerVillager {false};
	bool dryLand {false};
	/// The altitude of the cell nearest it, none off the map
	std::optional<uint8_t> nearestAltitude;
	/// A person, an animal or a fence, which needs ground that isn't too steep
	bool needsGentleSlope {false};
	/// The up of the land's normal under it
	float normalY {1.0f};
};
/// Whether a release stands where it is put down rather than flying
[[nodiscard]] bool LandsOnRelease(const Landing& landing);

/// What happens to a put-down that stands
enum class LandedOutcome : uint8_t
{
	/// It leaves the physics at once, its kind's landing run (people, animals, fences, trees planted again)
	LeavesPhysics,
	/// It stays in the physics, settling where it is put
	Settles,
	/// A tree held tilted or not to be planted again: it stays in the physics and falls, not counted as put down
	Falls,
};
struct LandedThing
{
	bool living {false};
	bool fence {false};
	bool tree {false};
	bool burning {false};
	bool onLand {false};
	/// Its tilt about its two level axes as the hand held it
	float tiltX {0.0f};
	float tiltZ {0.0f};
	/// The hand was made to let go, or the thing mustn't be planted again
	bool dontReplant {false};
	/// A creature let it go: its tilt and planting aren't asked
	bool byCreature {false};
};
[[nodiscard]] LandedOutcome OutcomeOfLanding(const LandedThing& thing);

/// Where a line meets a triangle, as a distance along the line, none when it misses it
[[nodiscard]] std::optional<float> RayTriangle(glm::vec3 origin, glm::vec3 direction, glm::vec3 a, glm::vec3 b, glm::vec3 c);

} // namespace openblack::hand_grab
