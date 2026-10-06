/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <numbers>
#include <optional>

#include <glm/vec2.hpp>

/// How a creature walks and runs, once a game turn. Its speed follows a requested fraction of its top speed, speeding up
/// and slowing down at a steady rate, slower uphill. The distance it covers drives its walk and run animations, blended
/// by how fast it goes, so its feet keep to the ground. Before setting off, and at corners too sharp to walk round, it
/// stops and either steps off towards its way, walks straight on when nearly facing it, or turns on the spot.
///
/// Headings are angles about the vertical axis: a creature with heading h faces (-sin h, -cos h) on the x, z plane, as
/// a body rotated by h about y does, its meshes looking along their -z axis.
namespace openblack::creature_locomotion
{
/// The places of the moving animations in the creature spec file
namespace animations
{
constexpr size_t k_Stand = 0;
constexpr size_t k_Walk = 1;
constexpr size_t k_Run = 2;
/// Turning on the spot to the right by about 0, 90 and 180 degrees, then the same to the left
constexpr size_t k_RightSpin = 3;
constexpr size_t k_LeftSpin = 6;
/// Stepping off to the right by 0, 90 and 180 degrees, then the same to the left
constexpr size_t k_RightStep = 10;
constexpr size_t k_LeftStep = 13;
} // namespace animations

/// Speeding up and slowing down, in units a second each second
constexpr float k_Acceleration = 12.0f;
/// The slowest a moving creature is allowed to go
constexpr float k_MinSpeed = 0.1f;
/// Running flat out, a creature goes this much faster than its running speed
constexpr float k_TopSpeedMargin = 1.1f;
/// A route turning this far from the way the creature faces makes it stop and start again
constexpr float k_CornerAngle = std::numbers::pi_v<float> / 6.0f;
/// Starting off, a creature this nearly facing its way walks straight on
constexpr float k_WalkStraightAngle = std::numbers::pi_v<float> / 12.0f;
/// The slope ahead slows the creature down by this much per unit of rise over its radius, within these limits
constexpr float k_SlopeSlowing = 0.6f;
constexpr float k_MinSlopeFactor = 0.3f;
constexpr float k_MaxSlopeFactor = 1.1f;
/// A creature this exhausted only goes slowly
constexpr float k_ExhaustedAt = 0.8f;
/// The smallest and largest sizes speeds are worked out for
constexpr float k_MinSize = 0.05f;
constexpr float k_MaxSize = 4.0f;

/// How fast a creature of a size walks and runs, in units a second
struct Speeds
{
	float walk;
	float run;
};
/// Walking is 8 and running 20 units a second for each unit of 0.875 size + 0.25
[[nodiscard]] Speeds SpeedsFor(float size);

/// The speed a creature heads for when asked for a fraction of its top speed
[[nodiscard]] float TargetSpeed(float fraction, float runSpeed);
/// The fraction of its top speed it goes at: what is wanted, or only its slow fraction once exhausted
[[nodiscard]] float RequiredFraction(float wanted, float slowFraction, float exhaustion);
/// Led on the leash, the creature goes from walking towards twice its running fraction the harder it is pulled
[[nodiscard]] float LeashFraction(float walkFraction, float runFraction, float pull);

/// The fastest it may go so as to stop in the distance left
[[nodiscard]] float StopCap(float remaining);
/// The fastest it may go so as to reach a corner at the speed allowed there, given as a squared speed in units of the
/// deceleration
[[nodiscard]] float CornerCap(float remaining, float cornerAllowance);
/// How the slope ahead changes the speed: slower uphill, a little faster downhill
[[nodiscard]] float SlopeFactor(float altitudeAhead, float altitude, float radius);
/// The speed after some seconds of speeding up towards the target. It never slows down towards a lower target: only
/// the stop and corner caps slow it.
[[nodiscard]] float Accelerate(float speed, float target, float seconds);

/// An angle brought within -pi to pi
[[nodiscard]] float WrapAngle(float angle);
/// The heading a direction on the x, z plane faces
[[nodiscard]] float HeadingOf(glm::vec2 direction);
/// The direction on the x, z plane a heading faces
[[nodiscard]] glm::vec2 DirectionOf(float heading);
/// Turning without animations: the heading eases round to the target by how far through the turn it is
[[nodiscard]] float LerpHeading(float heading, float target, float timeMs, float durationMs);

/// An animation cycle's length and the distance its root moves over it, in mesh units
struct Cycle
{
	float durationMs;
	float stride;
};

/// An animation played in the blend, its time and its weight
struct Slot
{
	size_t animation;
	float timeMs;
	float weight;
};

/// The walk and run blend for one turn
struct Gait
{
	/// Below walking speed the stand and the walk, above it the run and the walk
	std::array<Slot, 2> slots;
	/// The walk's time after this turn, and how far it moved on in milliseconds
	float walkTimeMs;
	float walkAdvanceMs;
};

/// Blends standing into walking below walking speed, and walking into running above it, by how fast the creature goes.
/// The walk moves on by the distance covered over the blend's stride, so the feet keep to the ground; the run keeps in
/// step with the walk. Weights stay within 0 and 1 even when the creature goes faster than its running speed.
[[nodiscard]] Gait BlendGait(float speed, const Speeds& speeds, float distance, float scale, const Cycle& stand,
                             const Cycle& walk, const Cycle& run, float walkTimeMs, float breathPhase);

enum class Side : uint8_t
{
	Right,
	Left,
};

/// Which side a target is on: an angle turning towards the creature's right is positive
[[nodiscard]] Side SideOf(float angle);

/// Two of the three 0, 90 and 180 degree animations of a side, and how far to blend from the first to the second
struct Pair
{
	size_t from;
	size_t to;
	float weight;
};
/// The pair showing an angle, from the side's first animation: up to 90 degrees between 0 and 90, beyond it between 90
/// and 180
[[nodiscard]] Pair PairFor(size_t first, float angle);

/// How a creature sets off towards a point, or turns to face one
struct Start
{
	enum class Kind : uint8_t
	{
		/// Stepping off sideways or backwards into a walk
		Step,
		/// Walking straight on
		Walk,
		/// Turning on the spot first, by the spin animations when there are any
		Turn,
	};
	Kind kind;
	Side side;
	std::optional<Pair> animations;
};

/// What the creature has to start with
struct StartOptions
{
	/// The strides of the side's 0, 90 and 180 degree steps in mesh units, when the species has all three
	std::optional<std::array<float, 3>> stepStrides;
	/// Whether the species has the side's spins
	bool hasSpins {false};
	/// Whether it may step or walk off at all, rather than only turn to face the point
	bool moving {true};
};

/// Steps when it has the steps and the point is further than every one of them takes it; else walks straight on when
/// nearly facing the point; else turns on the spot
[[nodiscard]] Start ChooseStart(float angle, float distanceInMesh, const StartOptions& options);

/// The distances around a destination that count as arriving: anywhere from min to max
struct Ring
{
	float min;
	float max;
};
/// The arrival ring for a walk of a distance: min never beyond the destination, max at least a hair beyond min
[[nodiscard]] Ring MoveRing(float distance, float minDistance, float maxDistance);
/// The arrival ring for walking up to an object: its radius and the creature's, and some way further
[[nodiscard]] Ring ObjectRing(float creatureRadius, float objectRadius, float extra);
/// Whether a creature this far from its destination has arrived
[[nodiscard]] bool Arrived(float distance, const Ring& ring);
/// How long a walk of a distance is given before it is given up on, in milliseconds
[[nodiscard]] float TimeLimitMs(float distance);
/// Running away from a threat: a distance further along the way from the threat to the creature
[[nodiscard]] glm::vec2 RunAwayPoint(glm::vec2 creature, glm::vec2 threat, float distance);
} // namespace openblack::creature_locomotion
