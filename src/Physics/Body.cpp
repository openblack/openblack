/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Body.h"

#include <cmath>

#include <algorithm>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>

#include "Ground.h"

using namespace openblack::physics;

namespace
{
/// Points this close to the sea's level or the land's are in it
constexpr float k_SeaLevelTolerance = 0.0001f;
/// A floating body soaks up this much density each step it is in the sea
constexpr float k_SoakPerStep = 6.66667e-5f;
/// The sea slows a body this many times more than the air
constexpr float k_WaterDragFactor = 100.0f;
/// The share of the sea's push each point under the surface turns the body by
constexpr float k_WaterTurnShare = 0.02f;
/// How much the damping of a deepening contact counts
constexpr float k_DampingFactor = 200.0f;
/// Two bodies slide off each other this much more easily than off the land
constexpr float k_BodyFrictionFactor = 0.3f;
constexpr float k_DragRadiusFactor = 0.3f;
/// A body falling this many radii under the sea's level is gone
constexpr float k_DeleteDepthRadii = -4.0f;
/// The rest test: the speed limit at first, once resting, and how it loosens after a long time in the physics
constexpr float k_RestThreshold = 1.0f;
constexpr float k_RestingThreshold = 4.0f;
constexpr int k_RestCounterLoosens = 15000;
constexpr float k_RestLoosening = 6.66667e-5f;
constexpr int k_RestCounterPerStep = 5;
constexpr float k_MinRotationStep = 1e-5f;
constexpr float k_FacingThreshold = -0.0001f;
constexpr float k_AnyFaceStart = -10000.0f;
constexpr float k_MinAheadLength = 0.001f;
/// The spin kept each step is the share kept each second raised to the step's length
constexpr double k_StepExponent = 0.005000000074505806;
/// The cosine of a quarter turn as the game stores it, which is not quite zero
constexpr float k_QuarterTurnCos = -4.371139e-8f;
constexpr float k_QuarterTurnSin = 1.0f;

glm::vec3 NormaliseOrKeep(glm::vec3 v)
{
	if (v.x == 0.0f && v.y == 0.0f && v.z == 0.0f)
	{
		return v;
	}
	return v / std::sqrt(glm::dot(v, v));
}

glm::mat3 NormaliseAxes(glm::mat3 axes)
{
	for (int i = 0; i < 3; ++i)
	{
		axes[i] = NormaliseOrKeep(axes[i]);
	}
	return axes;
}

glm::vec2 Xz(glm::vec3 v)
{
	return {v.x, v.z};
}

bool InsideTriangle(glm::vec3 p, glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 normal)
{
	return glm::dot(glm::cross(v1 - v0, p - v0), normal) > 0.0f && glm::dot(glm::cross(v2 - v1, p - v1), normal) > 0.0f &&
	       glm::dot(glm::cross(v0 - v2, p - v2), normal) > 0.0f;
}
} // namespace

Body::Body(const BodySetup& setup, const Shape& shape)
    : density(setup.material.density)
    , _scale(setup.scale)
    , _centreOfMass(shape.centreOfMass)
    , _mass(setup.mass)
    , _stiffness(setup.mass * setup.material.springK)
    , _damping(setup.mass * setup.material.dampK)
    , _friction(setup.material.friction)
    , _spinKeptPerStep(static_cast<float>(std::pow(static_cast<double>(setup.material.spinKeptPerSecond), k_StepExponent)))
    , _drag(setup.material.drag)
    , _radius(shape.radius)
    , _dynamic(setup.dynamic)
    , _restCounter(-static_cast<int>(std::lrint(setup.scale * setup.halfHeight * 1000.0f)))
{
	_points.reserve(shape.points.size());
	for (const auto& local : shape.points)
	{
		_points.push_back(Point {.local = local});
	}
	_faces.reserve(shape.faces.size());
	for (const auto& indices : shape.faces)
	{
		_faces.push_back(Face {.indices = indices});
	}
	SetUpInertia();
	_drag *= shape.dragFactor;
}

void Body::SetUpInertia()
{
	externalForce = glm::vec3(0.0f);
	externalTorque = glm::vec3(0.0f);
	for (auto& point : _points)
	{
		point.length = glm::length(point.local);
		point.aheadLength = point.length;
	}
	if (_dynamic && !_points.empty())
	{
		auto& inertia = _inertia;
		inertia = {};
		const float pointMass = _mass / static_cast<float>(_points.size());
		for (const auto& point : _points)
		{
			const auto [x, y, z] = std::array {point.local.x, point.local.y, point.local.z};
			inertia[0][0] += (y * y + z * z) * pointMass;
			inertia[1][1] += (x * x + z * z) * pointMass;
			inertia[2][2] += (x * x + y * y) * pointMass;
			inertia[0][1] -= x * y * pointMass;
			inertia[1][0] -= x * y * pointMass;
			inertia[0][2] -= x * z * pointMass;
			inertia[2][0] -= x * z * pointMass;
			inertia[2][1] -= y * z * pointMass;
			// The game sums x z here where y z belongs, which makes lopsided bodies tumble its own way
			inertia[1][2] -= x * z * pointMass;
		}
		glm::mat3 matrix;
		for (int row = 0; row < 3; ++row)
		{
			for (int column = 0; column < 3; ++column)
			{
				matrix[column][row] = inertia[row][column];
			}
		}
		// A body whose points are all in a line can't be turned; it keeps the identity rather than dividing by nothing
		const auto inverse = glm::determinant(matrix) != 0.0f ? glm::inverse(matrix) : glm::mat3(1.0f);
		for (int row = 0; row < 3; ++row)
		{
			for (int column = 0; column < 3; ++column)
			{
				_inverseInertia[row][column] = inverse[column][row];
			}
		}
		_drag *= _radius * _radius * k_DragRadiusFactor;
	}
	else
	{
		_inertia = {{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
		_inverseInertia = _inertia;
	}
	inWater = false;
	touched = false;
	justSetUp = true;
}

void Body::SetUpPose(const Pose& objectMatrix)
{
	_centre = objectMatrix.origin + objectMatrix.axes * _centreOfMass;
	_axes = NormaliseAxes(objectMatrix.axes);
	_turnStartAxes = _axes;
	_turnStartCentre = _centre;
	for (auto& point : _points)
	{
		point.world = _axes * point.local + _centre;
		point.contact = point.world;
		point.firstDepth = 0.0f;
		point.depth = 0.0f;
	}
	_bones.clear();
	_contacts = 0;
	for (auto& face : _faces)
	{
		const auto& a = _points[face.indices[0]].local;
		const auto& b = _points[face.indices[1]].local;
		const auto& c = _points[face.indices[2]].local;
		face.localNormal = NormaliseOrKeep(glm::cross(b - a, c - a));
		face.worldNormal = _axes * face.localNormal;
	}
}

void Body::SetPoseDirect(const glm::mat3& axes, glm::vec3 centre)
{
	_axes = axes;
	_centre = centre;
	_turnStartAxes = _axes;
	_turnStartCentre = _centre;
	for (auto& point : _points)
	{
		point.world = _axes * point.local + _centre;
		point.contact = point.world;
		point.firstDepth = 0.0f;
		point.depth = 0.0f;
	}
	for (auto& face : _faces)
	{
		const auto& a = _points[face.indices[0]].local;
		const auto& b = _points[face.indices[1]].local;
		const auto& c = _points[face.indices[2]].local;
		face.localNormal = NormaliseOrKeep(glm::cross(b - a, c - a));
		face.worldNormal = _axes * face.localNormal;
	}
}

void Body::SetSkeleton(std::vector<Ellipsoid> bones)
{
	_bones = std::move(bones);
}

void Body::AdjustToGroundLevel(const Ground& ground, bool noPullDown, bool alignToSlope)
{
	if (alignToSlope)
	{
		const auto normal = ground.NormalAt(Xz(_centre));
		const auto forward = NormaliseOrKeep(glm::cross(_axes[0], normal));
		_axes = glm::mat3(glm::cross(normal, forward), normal, forward);
	}
	float lowest = 1000.0f;
	for (const auto& point : _points)
	{
		const auto world = _axes * point.local + _centre;
		lowest = std::min(lowest, world.y - ground.HeightAt(Xz(world)));
	}
	if (noPullDown && lowest > 0.0f)
	{
		lowest = 0.0f;
	}
	_centre.y -= lowest;
	_turnStartAxes = _axes;
	_turnStartCentre = _centre;
}

void Body::ClearForces()
{
	touched = false;
	if (resting)
	{
		_aheadCentre = _centre;
		_force = glm::vec3(0.0f);
		_torque = glm::vec3(0.0f);
		return;
	}
	const auto ahead = velocity * k_LookAheadSeconds;
	for (auto& point : _points)
	{
		point.world = _axes * point.local + _centre + ahead;
		point.aheadLength = std::max(glm::length(point.world - _centre), k_MinAheadLength);
	}
	_aheadCentre = _centre + ahead;
	for (auto& face : _faces)
	{
		face.worldNormal = _axes * face.localNormal;
	}
	_force = externalForce;
	_torque = externalTorque;
}

void Body::TouchGround(const Ground& ground)
{
	if (resting)
	{
		if (_bones.empty() || !_faces.empty())
		{
			for (auto& point : _points)
			{
				point.contact = point.world;
				point.depth = 0.0f;
			}
		}
		else
		{
			// A resting skeleton's points all stand at its first part's origin
			const auto origin = glm::vec3(_bones.front().localToWorld[3]);
			for (auto& point : _points)
			{
				point.world = origin;
				point.contact = origin;
				point.length = 1.0f;
				point.aheadLength = 1.0f;
				point.depth = 0.0f;
			}
		}
		return;
	}

	_force -= velocity * (_speed * _drag);
	_contacts = 0;
	_force.y -= _mass * k_Gravity;

	const bool overSea = ground.HeightAt(Xz(_centre)) < k_SeaLevelTolerance && _centre.y < _radius && !_points.empty() &&
	                     ground.IsSeaCell(Xz(_points.front().world));
	if (overSea)
	{
		const auto submerged = std::ranges::count_if(_points, [](const Point& point) { return point.world.y < 0.0f; });
		if (submerged > 0)
		{
			inWater = true;
			touched = true;
			density += k_SoakPerStep;
			const float share = std::min((_radius - _centre.y) / (_radius + _radius), 1.0f);
			const float drag = share * _speed * _drag * k_WaterDragFactor;
			const glm::vec3 water(-drag * velocity.x, share * _mass * k_Gravity / density - drag * velocity.y,
			                      -drag * velocity.z);
			_force += water;
			const auto turn = water * (k_WaterTurnShare / static_cast<float>(submerged));
			for (auto& point : _points)
			{
				point.contact = point.world;
				point.depth = 0.0f;
				if (point.world.y < 0.0f)
				{
					_torque += glm::cross(point.world - _aheadCentre, turn);
				}
			}
			// While any point is in the sea the body never touches the sea floor
			return;
		}
		inWater = false;
	}

	for (auto& point : _points)
	{
		const float height = ground.HeightAt(Xz(point.world));
		point.depth = height - point.world.y;
		point.other = nullptr;
		point.contact = point.world;
		if (point.depth >= 0.0f)
		{
			++_contacts;
			point.normal = ground.NormalAt(Xz(point.world));
			point.contact.y = height;
			touched = true;
		}
	}
}

std::optional<RayHit> Body::FaceBehind(glm::vec3 point, glm::vec3 direction) const
{
	std::optional<RayHit> found;
	float best = -1.0f;
	for (const auto& face : _faces)
	{
		const auto& normal = face.worldNormal;
		const float facing = glm::dot(normal, direction);
		if (!(facing < k_FacingThreshold))
		{
			continue;
		}
		const auto& v0 = _points[face.indices[0]].world;
		const float t = -glm::dot(point - v0, normal) / facing;
		if (!(t < 0.0f) || !(best < t))
		{
			continue;
		}
		const auto hit = point + t * direction;
		if (!InsideTriangle(hit, v0, _points[face.indices[1]].world, _points[face.indices[2]].world, normal))
		{
			continue;
		}
		best = t;
		found = RayHit {.point = hit, .normal = normal};
	}
	return found;
}

std::optional<glm::vec3> Body::AnyFaceBehind(glm::vec3 point, glm::vec3 direction) const
{
	std::optional<glm::vec3> found;
	float best = k_AnyFaceStart;
	for (const auto& face : _faces)
	{
		const auto& v0 = _points[face.indices[0]].world;
		const auto& v1 = _points[face.indices[1]].world;
		const auto& v2 = _points[face.indices[2]].world;
		const auto normal = glm::cross(v1 - v0, v2 - v0);
		const float facing = glm::dot(normal, direction);
		if (!(facing < k_FacingThreshold))
		{
			continue;
		}
		const float t = -glm::dot(point - v0, normal) / facing;
		if (!(t < 0.0f) || !(t > best))
		{
			continue;
		}
		const auto hit = point + t * direction;
		if (!InsideTriangle(hit, v0, v1, v2, normal))
		{
			continue;
		}
		best = t;
		found = hit;
	}
	return found;
}

std::optional<std::pair<RayHit, float>> openblack::physics::EllipsoidHit(std::span<const Ellipsoid> bones, glm::vec3 point,
                                                                         glm::vec3 direction)
{
	constexpr float k_Tiny = 0.0001f;
	std::optional<std::pair<RayHit, float>> best;
	for (const auto& bone : bones)
	{
		const auto half = (bone.boxMax - bone.boxMin) * 0.5f;
		if (!(half.x > k_Tiny && half.y > k_Tiny && half.z > k_Tiny))
		{
			continue;
		}
		// Into the part's own space, where its box is the unit sphere about the box's centre
		const auto localPoint = glm::vec3(bone.worldToLocal * glm::vec4(point, 1.0f));
		const auto start = (localPoint - bone.boxMin) / half - 1.0f;
		const auto along = glm::vec3(bone.worldToLocal * glm::vec4(direction, 0.0f)) / half;
		const float a = glm::dot(along, along);
		if (!(a > k_Tiny))
		{
			continue;
		}
		const float b = 2.0f * glm::dot(along, start);
		const float discriminant = b * b - (glm::dot(start, start) - 1.0f) * a * 4.0f;
		if (!(discriminant >= 0.0f))
		{
			continue;
		}
		const float t = (-b - std::sqrt(discriminant)) / (a + a);
		if (best.has_value() && !(t < best->second))
		{
			continue;
		}
		const auto onSphere = start + along * t;
		const auto hitLocal = (onSphere + 1.0f) * half + bone.boxMin;
		const auto normalLocal = onSphere * half;
		const auto hitWorld = glm::vec3(bone.localToWorld * glm::vec4(hitLocal, 1.0f));
		const auto normalWorld = NormaliseOrKeep(glm::vec3(bone.localToWorld * glm::vec4(normalLocal, 0.0f)));
		best = std::pair {RayHit {.point = hitWorld, .normal = normalWorld}, t};
	}
	return best;
}

std::optional<RayHit> Body::SkeletonHit(glm::vec3 point, glm::vec3 direction) const
{
	if (auto hit = EllipsoidHit(_bones, point, direction))
	{
		return hit->first;
	}
	return std::nullopt;
}

void Body::TouchFaces(Body& other)
{
	const float reach2 = other._radius * other._radius;
	for (auto& point : _points)
	{
		const auto offset = point.world - other._centre;
		if (!(glm::dot(offset, offset) < reach2))
		{
			continue;
		}
		const auto direction = _bones.empty() ? point.world - _centre : NormaliseOrKeep(other._centre - point.world);
		std::optional<RayHit> hit;
		if (other._bones.empty())
		{
			hit = other.FaceBehind(point.world, direction);
		}
		else
		{
			hit = other.SkeletonHit(point.world, direction);
			if (hit.has_value())
			{
				// A skeleton is hit only between the body's centre and where the point will be
				const float along = glm::dot(hit->point - _centre, direction) / point.aheadLength;
				if (along < 0.0f || point.aheadLength < along)
				{
					continue;
				}
			}
		}
		if (!hit.has_value())
		{
			continue;
		}
		++_contacts;
		point.contact = hit->point;
		point.other = &other;
		point.normal = hit->normal;
		const float depth = point.aheadLength - glm::dot(hit->point - _centre, direction) / point.aheadLength;
		if (point.depth < depth)
		{
			point.depth = depth;
			if (!resting || !other.resting)
			{
				lastHit = &other;
				touched = true;
				other.lastHit = this;
				other.touched = true;
			}
		}
	}
}

void Body::ApplyContacts()
{
	justSetUp = false;
	_restCounter += k_RestCounterPerStep;
	if (_contacts == 0)
	{
		return;
	}
	for (auto& point : _points)
	{
		if (!(0.0f < point.depth))
		{
			point.anchor = point.contact;
			point.firstDepth = 0.0f;
			continue;
		}
		float stiffness = _stiffness;
		float damping = _damping;
		float friction = _friction;
		if (point.other != nullptr)
		{
			stiffness = std::min(_stiffness, point.other->_stiffness);
			damping = std::min(_damping, point.other->_damping);
			friction = std::min(_friction, point.other->_friction) * k_BodyFrictionFactor;
		}
		float push = 0.0f;
		if (point.firstDepth == 0.0f)
		{
			// The first step of a touch pushes by its depth alone
			point.firstDepth = point.depth;
			push = point.depth * stiffness;
		}
		else
		{
			push = stiffness * point.firstDepth +
			       (point.depth - point.firstDepth) * k_DampingFactor * (point.aheadLength / point.length) * damping;
		}
		push = std::max(push, 0.0f);
		const float maxFriction = push * friction;
		point.normal *= push;
		auto hold = (point.anchor - point.contact) * stiffness;
		if (maxFriction * maxFriction < glm::dot(hold, hold))
		{
			// It slides: friction holds no harder than its limit, and the anchor follows the point
			hold *= maxFriction / std::sqrt(glm::dot(hold, hold));
			point.anchor = point.contact + hold / stiffness;
		}
		point.normal += hold;
		const auto total = point.normal;
		_force += total;
		_torque += glm::cross(point.world - _aheadCentre, total);
		if (point.other != nullptr)
		{
			point.other->_force -= total;
			point.other->_torque += glm::cross(point.world - point.other->_centre, -total);
		}
	}
}

glm::vec3 Body::BodyOmega() const
{
	const auto local = glm::transpose(_axes) * angularMomentum;
	glm::vec3 omega(0.0f);
	for (int column = 0; column < 3; ++column)
	{
		for (int row = 0; row < 3; ++row)
		{
			omega[column] += local[row] * _inverseInertia[row][column];
		}
	}
	return omega;
}

glm::vec3 Body::AngularVelocity() const
{
	return _axes * BodyOmega();
}

void Body::SetAngularVelocityInBodyAxes(glm::vec3 omega)
{
	glm::vec3 local(0.0f);
	for (int column = 0; column < 3; ++column)
	{
		for (int row = 0; row < 3; ++row)
		{
			local[column] += omega[row] * _inertia[row][column];
		}
	}
	angularMomentum = _axes * local;
}

void Body::SetAngularVelocity(glm::vec3 omega)
{
	SetAngularVelocityInBodyAxes(glm::transpose(_axes) * omega);
}

StepResult Body::Integrate()
{
	if (!_dynamic || (resting && !touched))
	{
		return StepResult::None;
	}
	if (_centre.y < _radius * k_DeleteDepthRadii)
	{
		return StepResult::Delete;
	}

	angularMomentum += _torque * k_StepSeconds;
	angularMomentum *= _spinKeptPerStep;
	auto omega = BodyOmega();
	const float spin2 = glm::dot(omega, omega);
	if (k_MaxSpin * k_MaxSpin < spin2 && (omega.x != 0.0f || omega.y != 0.0f || omega.z != 0.0f))
	{
		omega *= k_MaxSpin / std::sqrt(spin2);
	}
	const auto worldOmega = _axes * omega;

	velocity += _force * (k_StepSeconds / _mass);
	_speed = glm::length(velocity);
	if (k_MaxSpeed < _speed)
	{
		velocity *= k_MaxSpeed / _speed;
		_speed = k_MaxSpeed;
	}

	float threshold = k_RestThreshold;
	if (resting)
	{
		threshold = k_RestingThreshold;
	}
	else if (k_RestCounterLoosens < _restCounter)
	{
		threshold = static_cast<float>(_restCounter) * k_RestLoosening;
	}
	if (_contacts != 0 && _speed < threshold && spin2 < threshold * threshold * 0.25f)
	{
		const auto turning = _torque / (_radius * _radius * _mass);
		if (glm::dot(turning, turning) < threshold * threshold)
		{
			if (resting)
			{
				velocity = glm::vec3(0.0f);
				_speed = 0.0f;
				angularMomentum = glm::vec3(0.0f);
				return StepResult::None;
			}
			if (0 < _restCounter)
			{
				velocity = glm::vec3(0.0f);
				_speed = 0.0f;
				angularMomentum = glm::vec3(0.0f);
				return StepResult::Stopped;
			}
		}
	}

	const auto step = worldOmega * k_StepSeconds;
	const float angle = glm::length(step);
	if (k_MinRotationStep < angle)
	{
		_axes = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, step / angle)) * _axes;
	}
	_centre += velocity * k_StepSeconds;
	return resting ? StepResult::Knocked : StepResult::Moved;
}

float Body::OverlapDepth(const Body& other, glm::vec3 direction) const
{
	float deepest = 0.0f;
	for (const auto& point : _points)
	{
		std::optional<glm::vec3> hit;
		if (other._bones.empty())
		{
			hit = other.AnyFaceBehind(point.world, direction);
		}
		else if (auto skeletonHit = other.SkeletonHit(point.world, direction))
		{
			hit = skeletonHit->point;
		}
		if (hit.has_value())
		{
			deepest = std::max(deepest, glm::dot(point.world - *hit, direction));
		}
	}
	return deepest;
}

void Body::Raise(float height)
{
	_centre.y += height;
	SetUpPose(ObjectPose());
}

void Body::BeginTurn()
{
	_turnStartAxes = _axes;
	_turnStartCentre = _centre;
}

Pose Body::ObjectPose() const
{
	const auto scaled = _axes * _scale;
	return {.axes = scaled, .origin = _centre - scaled * _centreOfMass};
}

Pose Body::DrawPose(float turnFraction, bool animated) const
{
	glm::mat3 axes;
	for (int i = 0; i < 3; ++i)
	{
		axes[i] = NormaliseOrKeep(_turnStartAxes[i] + (_axes[i] - _turnStartAxes[i]) * turnFraction);
	}
	const auto centre = _turnStartCentre + (_centre - _turnStartCentre) * turnFraction;
	if (animated)
	{
		axes = QuarterTurned(axes);
	}
	const auto scaled = axes * _scale;
	return {.axes = scaled, .origin = centre - scaled * _centreOfMass};
}

glm::mat3 openblack::physics::QuarterTurned(const glm::mat3& axes)
{
	auto turned = axes;
	turned[0] = k_QuarterTurnCos * axes[0] + k_QuarterTurnSin * axes[2];
	turned[2] = k_QuarterTurnCos * axes[2] - k_QuarterTurnSin * axes[0];
	return turned;
}

glm::mat3 openblack::physics::QuarterTurnedBack(const glm::mat3& axes)
{
	auto turned = axes;
	turned[0] = k_QuarterTurnCos * axes[0] - k_QuarterTurnSin * axes[2];
	turned[2] = k_QuarterTurnCos * axes[2] + k_QuarterTurnSin * axes[0];
	return turned;
}
