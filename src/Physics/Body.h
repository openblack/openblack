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

#include <array>
#include <numbers>
#include <optional>
#include <span>
#include <vector>

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Materials.h"

namespace openblack::physics
{
class Ground;

/// One step of the physics; twenty of them make a game turn of 0.1 s
inline constexpr float k_StepSeconds = 0.005f;
inline constexpr int k_StepsPerTurn = 20;
inline constexpr float k_Gravity = 9.81f;
/// How far ahead along its motion a body tests its contacts, so fast bodies don't pass through thin things
inline constexpr float k_LookAheadSeconds = 0.06f;
/// No body moves faster than this, nor does the hand throw faster
inline constexpr float k_MaxSpeed = 124.0f;
/// No body spins faster than one and a half turns a second
inline constexpr float k_MaxSpin = 3.0f * std::numbers::pi_v<float>;

/// What a step did to a body
enum class StepResult : uint8_t
{
	/// It didn't move: it's static, or resting and untouched, or resting and still holding
	None,
	/// It moved
	Moved,
	/// It came to rest
	Stopped,
	/// A resting body was knocked harder than it could hold, and moves
	Knocked,
	/// It fell more than four of its radii under the sea's level
	Delete,
};

/// One part of a skeleton the physics tests points against as an ellipsoid: the box of the part in its own space and
/// where that space stands in the world
struct Ellipsoid
{
	glm::vec3 boxMin {0.0f};
	glm::vec3 boxMax {0.0f};
	glm::mat4 localToWorld {1.0f};
	glm::mat4 worldToLocal {1.0f};
};

/// Where a line through a point first meets something
struct RayHit
{
	glm::vec3 point;
	glm::vec3 normal;
};

/// The shape of a body before it is placed in the world
struct Shape
{
	/// The points, about the centre of mass, already scaled
	std::vector<glm::vec3> points;
	/// Triangles of points, the faces other bodies' points hit
	std::vector<std::array<uint32_t, 3>> faces;
	/// The centre of mass in the model's own units, unscaled
	glm::vec3 centreOfMass {0.0f};
	/// The farthest point from the centre of mass, scaled
	float radius {0.0f};
	/// Multiplies the air drag once the body is set up: twice for villagers, animals and building pieces, 0.3 for trees
	float dragFactor {1.0f};
};

/// What a body is made of, beside its shape
struct BodySetup
{
	/// The object's scale
	float scale {1.0f};
	/// Half the height of the object's model, unscaled: the body can't come to rest before it has been in the physics
	/// that many seconds times the scale
	float halfHeight {0.0f};
	float mass {1.0f};
	Material material {};
	/// Static bodies (buildings, the creature, shields) never move; only others push off them
	bool dynamic {true};
};

/// A body's matrix: its axes in the world, scaled or not, and its origin
struct Pose
{
	glm::mat3 axes {1.0f};
	glm::vec3 origin {0.0f};
};

/// A rigid body as the game simulates it: a cloud of points, each a stiff spring with friction where it touches the land,
/// the sea or another body's faces, probed a little ahead of the motion and integrated in small fixed steps. There is
/// no bounce factor: how a body bounces, slides, rolls and settles comes from its material's stiffness, damping and
/// friction.
class Body
{
public:
	struct Point
	{
		/// Position about the centre of mass, scaled, in the body's axes
		glm::vec3 local {0.0f};
		float length {0.0f};
		/// Its distance from the centre at the look-ahead position (at least 0.001)
		float aheadLength {0.0f};
		/// Where the point will be a look-ahead from now
		glm::vec3 world {0.0f};
		/// Where it touches what it hits
		glm::vec3 contact {0.0f};
		/// The surface normal of what it hits, then the force of the contact
		glm::vec3 normal {0.0f};
		/// How deep it is in what it hits this step; positive inside
		float depth {0.0f};
		/// The depth at the first step of the current touch, 0 when not touching
		float firstDepth {0.0f};
		/// Where its friction holds it
		glm::vec3 anchor {0.0f};
		/// The body whose face it hits, none for the land
		Body* other {nullptr};
	};

	struct Face
	{
		std::array<uint32_t, 3> indices {};
		glm::vec3 localNormal {0.0f};
		glm::vec3 worldNormal {0.0f};
	};

	Body(const BodySetup& setup, const Shape& shape);

	/// Places the body from the object's matrix (its axes carry the object's scale): the body's centre is the object's
	/// origin moved to the centre of mass, its axes are made unit, its points and faces follow, its touches are forgotten
	/// and the start of the turn is this pose.
	void SetUpPose(const Pose& objectMatrix);
	/// Places the body with its axes and centre as they are (a building piece's matrix, the creature's sphere)
	void SetPoseDirect(const glm::mat3& axes, glm::vec3 centre);
	/// The creature's body: its centre at its bounding sphere's, its points standing at its parts' origins in the world
	/// though they sit on the centre in its own frame. The axes are left as they are.
	void PlacePoints(glm::vec3 centre, std::span<const glm::vec3> worldPoints);
	/// The creature's posed skeleton, whose parts are what other bodies hit
	void SetSkeleton(std::vector<Ellipsoid> bones);
	[[nodiscard]] bool HasSkeleton() const { return !_bones.empty(); }

	/// Optionally lays the body along the slope under its centre, then moves it up or down until its lowest point is on
	/// the land; with noPullDown it is only ever raised. The points aren't moved with it until the next step.
	void SettleOnLand(const Ground& ground, bool noPullDown, bool alignToSlope);

	// One step, in this order for every body: ClearForces and TouchGround for all, then TouchFaces for every pair, then
	// ApplyContacts for all, then Integrate for all.

	/// Forces start again from the steady push on the body; the points move to where they will be a look-ahead from now
	void ClearForces();
	/// Air drag and gravity, then the sea or the land under each point
	void TouchGround(const Ground& ground);
	/// This body's points against another body's faces (or its skeleton)
	void TouchFaces(Body& other);
	/// Each touching point pushes as a spring and holds by friction, on this body and back on what it touches
	void ApplyContacts();
	[[nodiscard]] StepResult Integrate();

	/// How far this body's points are inside another's faces along a direction, the deepest of them (0 when none is)
	[[nodiscard]] float OverlapDepth(const Body& other, glm::vec3 direction) const;
	/// Moves the body up and places it again there
	void Raise(float height);

	/// The start of a game turn: the pose the body is drawn from
	void BeginTurn();
	/// A resting body follows its object: its centre moves, its points stay where they were placed
	void MoveCentre(glm::vec3 centre) { _centre = centre; }

	/// The object's matrix from the body: its axes scaled by the object's scale, its origin back from the centre of mass
	[[nodiscard]] Pose ObjectPose() const;
	/// The pose a moving body is drawn at, a share of the way from the turn's start to now; animated models are turned a
	/// quarter about their up axis
	[[nodiscard]] Pose DrawPose(float turnFraction, bool animated) const;

	/// Sets the spin from an angular velocity in the world
	void SetAngularVelocity(glm::vec3 omega);
	/// Sets the spin from an angular velocity about the body's own axes
	void SetAngularVelocityInBodyAxes(glm::vec3 omega);
	/// The angular velocity in the world
	[[nodiscard]] glm::vec3 AngularVelocity() const;

	[[nodiscard]] const std::vector<Point>& Points() const { return _points; }
	[[nodiscard]] const std::vector<Face>& Faces() const { return _faces; }
	[[nodiscard]] const std::vector<Ellipsoid>& Skeleton() const { return _bones; }
	/// Unit axes of the body in the world
	[[nodiscard]] const glm::mat3& Axes() const { return _axes; }
	/// The centre of mass in the world
	[[nodiscard]] glm::vec3 Centre() const { return _centre; }
	[[nodiscard]] const glm::mat3& TurnStartAxes() const { return _turnStartAxes; }
	[[nodiscard]] glm::vec3 TurnStartCentre() const { return _turnStartCentre; }
	[[nodiscard]] glm::vec3 CentreOfMass() const { return _centreOfMass; }
	[[nodiscard]] float Scale() const { return _scale; }
	[[nodiscard]] float Radius() const { return _radius; }
	[[nodiscard]] float Mass() const { return _mass; }
	[[nodiscard]] float Speed() const { return _speed; }
	[[nodiscard]] float Drag() const { return _drag; }
	void SetDrag(float drag) { _drag = drag; }
	[[nodiscard]] bool IsDynamic() const { return _dynamic; }
	[[nodiscard]] int Contacts() const { return _contacts; }
	[[nodiscard]] int RestCounter() const { return _restCounter; }
	/// The force and turning force summed this step
	[[nodiscard]] glm::vec3 Force() const { return _force; }
	[[nodiscard]] glm::vec3 Torque() const { return _torque; }
	/// Inertia about the body's own axes, as rows
	[[nodiscard]] const std::array<std::array<float, 3>, 3>& Inertia() const { return _inertia; }

	glm::vec3 velocity {0.0f};
	glm::vec3 angularMomentum {0.0f};
	/// Pushes that last the game turn: a villager pushing it, the hand's twist after a throw. Torques, spins and angular
	/// momenta here follow the right-hand rule (torque = r x F), the opposite sign to how the game writes them, so any
	/// spin or twist worked out the game's way must be negated before it is handed in
	glm::vec3 externalForce {0.0f};
	glm::vec3 externalTorque {0.0f};
	/// Rises as the body soaks up water
	float density {0.0f};
	/// The last body this one touched or was touched by
	Body* lastHit {nullptr};
	bool resting {false};
	/// Whether anything touched it this step
	bool touched {false};
	bool inWater {false};
	/// Set up since its last step: a resting body tests its points against other resting bodies once
	bool justSetUp {false};

private:
	void SetUpInertia();
	[[nodiscard]] glm::vec3 BodyOmega() const;
	/// The nearest face entry behind a point along a direction, at most one direction length back
	[[nodiscard]] std::optional<RayHit> FaceBehind(glm::vec3 point, glm::vec3 direction) const;
	/// The face nearest behind a point along a direction, however far
	[[nodiscard]] std::optional<glm::vec3> AnyFaceBehind(glm::vec3 point, glm::vec3 direction) const;
	/// The skeleton part a line meets first
	[[nodiscard]] std::optional<RayHit> SkeletonHit(glm::vec3 point, glm::vec3 direction) const;

	float _scale {1.0f};
	glm::mat3 _axes {1.0f};
	glm::vec3 _centre {0.0f};
	glm::mat3 _turnStartAxes {1.0f};
	glm::vec3 _turnStartCentre {0.0f};
	glm::vec3 _aheadCentre {0.0f};
	glm::vec3 _centreOfMass {0.0f};
	float _speed {0.0f};
	float _mass {1.0f};
	float _stiffness {0.0f};
	float _damping {0.0f};
	float _friction {0.0f};
	float _spinKeptPerStep {1.0f};
	float _drag {0.0f};
	float _radius {0.0f};
	bool _dynamic {true};
	int _contacts {0};
	int _restCounter {0};
	glm::vec3 _force {0.0f};
	glm::vec3 _torque {0.0f};
	std::array<std::array<float, 3>, 3> _inertia {};
	std::array<std::array<float, 3>, 3> _inverseInertia {};
	std::vector<Point> _points;
	std::vector<Face> _faces;
	std::vector<Ellipsoid> _bones;
};

/// The cosine and sine of a quarter turn as the game rounds them to floats: the cosine is not quite zero
inline constexpr float k_QuarterTurnCos = -4.371139e-8f;
inline constexpr float k_QuarterTurnSin = 1.0f;

/// Axes turned a quarter about their up axis, as the models moved by bones are drawn from their bodies' axes
[[nodiscard]] glm::mat3 QuarterTurned(const glm::mat3& axes);
/// The body's axes of a model moved by bones, from the axes it is drawn with
[[nodiscard]] glm::mat3 QuarterTurnedBack(const glm::mat3& axes);

/// The skeleton part a line through a point meets first, by how far along the line (negative is behind the point)
[[nodiscard]] std::optional<std::pair<RayHit, float>> EllipsoidHit(std::span<const Ellipsoid> bones, glm::vec3 point,
                                                                   glm::vec3 direction);

} // namespace openblack::physics
