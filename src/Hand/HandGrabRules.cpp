/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandGrabRules.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Physics/Body.h"

using namespace openblack;
using namespace openblack::hand_grab;

namespace
{
/// Held from the side, a gate totem and the weeping stones hang this share of their height below the hand
constexpr float k_HeavyStaticLowering = 0.7f;
/// ...and the singing stone this share
constexpr float k_SingingStoneLowering = 0.4f;
/// Loose things hang this share of their height below the hand, held from the side
constexpr float k_SideLowering = 0.7f;
/// The one loose thing held on the palm sits this share of its height above it
constexpr float k_PalmMobileLowering = -0.3f;
/// People and animals hang this share of their height below the hand
constexpr float k_LivingLowering = 0.65f;
/// Trees hang this share of their height below the hand
constexpr float k_TreeLowering = 0.1f;
/// A tree spreads out of the hand this share of its radius
constexpr float k_TreeHoldRadiusShare = 0.2f;
/// Held on the palm, a thing spreads out of the hand this share of its height
constexpr float k_PalmHoldRadiusShare = 0.75f;

bool GateTotem(MobileStaticInfo type)
{
	return type >= MobileStaticInfo::GateTotemApe && type <= MobileStaticInfo::GateTotemTiger;
}

/// Any thing's own reach out of the hand: its height on the palm, its radius otherwise
float ObjectHoldRadius(HoldType hold, float height, float radius)
{
	return hold == HoldType::Above ? height * k_PalmHoldRadiusShare : radius;
}
} // namespace

bool hand_grab::ValidForPlaceInHand(const Holdable& thing)
{
	switch (thing.kind)
	{
	case GrabKind::Villager:
		// Not one at home, in a hand already, or hiding in a building
		return thing.available && !thing.atHome && !thing.inHand && !thing.hiding;
	case GrabKind::Animal:
		return thing.speciesAllows;
	case GrabKind::Rock:
	case GrabKind::DeadTree:
		return !(thing.isRock && thing.radius > k_RockMaxPickUpRadius);
	case GrabKind::MobileStatic:
	case GrabKind::MobileObject:
	case GrabKind::Poo:
	case GrabKind::Tree:
		return true;
	case GrabKind::None:
		break;
	}
	return false;
}

bool hand_grab::PassesGate(const Gate& gate)
{
	return gate.spaceInHand && !gate.alreadyInHand && gate.valid && !gate.cannotBePickedUp && !gate.carried && gate.inInfluence;
}

uint32_t hand_grab::ElapsedMs(uint32_t nowMs, uint32_t pressMs, uint32_t turn, uint32_t pressTurn)
{
	const uint32_t byClock = nowMs - pressMs;
	const uint32_t byTurns = (turn + 1) * k_TurnMs - pressTurn * k_TurnMs;
	return std::min(byClock, byTurns);
}

HoldFacts hand_grab::HoldOf(GrabKind kind, MobileStaticInfo staticType, MeshId mesh, float height, float radius)
{
	HoldFacts facts;
	switch (kind)
	{
	case GrabKind::Villager:
	case GrabKind::Animal:
		facts.type = HoldType::Villager;
		facts.loweringMultiplier = k_LivingLowering;
		facts.holdRadius = ObjectHoldRadius(facts.type, height, radius);
		break;
	case GrabKind::Tree:
	case GrabKind::DeadTree:
		facts.type = HoldType::Tree;
		facts.loweringMultiplier = k_TreeLowering;
		facts.holdRadius = radius * k_TreeHoldRadiusShare;
		break;
	case GrabKind::MobileObject:
		facts.type = HoldType::Side;
		facts.loweringMultiplier = k_SideLowering;
		facts.holdRadius = ObjectHoldRadius(facts.type, height, radius);
		break;
	case GrabKind::Poo:
		facts.type = HoldType::Above;
		facts.loweringMultiplier = k_PalmMobileLowering;
		facts.holdRadius = ObjectHoldRadius(facts.type, height, radius);
		break;
	case GrabKind::Rock:
	case GrabKind::MobileStatic:
		if (GateTotem(staticType) || staticType == MobileStaticInfo::WeepingStone ||
		    staticType == MobileStaticInfo::WeepingStoneReward)
		{
			facts.type = HoldType::Side;
			facts.loweringMultiplier = k_HeavyStaticLowering;
		}
		else if (staticType == MobileStaticInfo::SingingStone_1)
		{
			facts.type = HoldType::Side;
			facts.loweringMultiplier = k_SingingStoneLowering;
		}
		else if (mesh == MeshId::ObjectToyCuddly || mesh == MeshId::ObjectToySkittle)
		{
			facts.type = HoldType::Side;
		}
		facts.holdRadius = ObjectHoldRadius(facts.type, height, radius);
		break;
	case GrabKind::None:
		facts.holdRadius = ObjectHoldRadius(facts.type, height, radius);
		break;
	}
	return facts;
}

float hand_grab::HoldDistance(float loweringMultiplier, float height, float handSize)
{
	return std::max(loweringMultiplier * height, k_LeastLoweringShare * k_StandardHandHeight * handSize);
}

float hand_grab::HandRise(HoldType hold, float lowering, float handSize, std::optional<float> rootedHeight)
{
	float rise = 0.0f;
	switch (hold)
	{
	case HoldType::Above:
		rise = k_AboveHang;
		break;
	case HoldType::Magic:
		rise = k_StandardHandHeight * handSize;
		break;
	case HoldType::Grain:
	case HoldType::Tree:
	case HoldType::Side:
	case HoldType::Villager:
		rise = std::max(lowering, k_MinimumHang);
		break;
	case HoldType::None:
	case HoldType::Fingers:
		break;
	}
	if (rootedHeight.has_value())
	{
		rise += k_RootedHangShare * *rootedHeight;
	}
	return rise;
}

float hand_grab::CursorRaise(float handRise)
{
	return k_CursorRaiseShare * handRise;
}

void HandSpring::Start(glm::vec3 position)
{
	_position = position;
	_velocity = glm::vec3(0.0f);
}

void HandSpring::Count(uint32_t frameMs)
{
	_elapsedMs += frameMs;
}

void HandSpring::Step(glm::vec3 target)
{
	constexpr float k_Step = static_cast<float>(k_SpringStepMs) * 0.001f;
	// At least one step a frame, then as many as the game's time has run
	_steps = 0;
	do
	{
		_steppedMs += k_SpringStepMs;
		++_steps;
		_velocity += (k_SpringStiffness * (target - _position) - k_SpringDamping * _velocity) * k_Step;
		if (const float speed = glm::length(_velocity); speed > k_MaxThrowSpeed)
		{
			_velocity *= k_MaxThrowSpeed / speed;
		}
		_position += _velocity * k_Step;
	} while (_steppedMs < _elapsedMs);
}

Countdown hand_grab::CountDown(int32_t& msLeft, uint32_t frameMs)
{
	if (msLeft <= 0)
	{
		return Countdown::Cancelled;
	}
	msLeft -= static_cast<int32_t>(frameMs);
	if (msLeft < 0)
	{
		return Countdown::RunOut;
	}
	return msLeft == 0 ? Countdown::Cancelled : Countdown::Running;
}

float hand_grab::MaxForce(float strength)
{
	return (strength + 1.0f) * k_MaxForcePerStrength;
}

glm::vec3 hand_grab::TugGrip(const Tug& tug, float holdDistance)
{
	const float length = glm::length(tug.axes[1]);
	const auto up = length > 0.0f ? tug.axes[1] / length : tug.axes[1];
	return tug.base + up * holdDistance;
}

TugResult hand_grab::PullAt(Tug& tug, const TugFrame& frame)
{
	TugResult result;
	const auto grip = TugGrip(tug, frame.holdDistance);
	// The pull is a spring from where the hand grips it to the hand, no stronger than the hand
	const auto stretch = frame.hand - grip;
	const float distance = glm::length(stretch);
	result.force = k_TugStiffness * distance >= frame.maxForce && distance > 0.0f ? stretch * (frame.maxForce / distance)
	                                                                              : stretch * k_TugStiffness;
	// It stretches towards the hand, by how far the hand is from its grip against how far up it is gripped
	if (frame.holdDistance > 0.0f)
	{
		result.stretchTarget = std::min((distance + frame.holdDistance) / frame.holdDistance, k_TugMaxStretch);
	}
	// A tree comes free once the hand is strong enough to lift it and pulls harder than it weighs; anything else at once
	result.comesFree =
	    !frame.tree || (frame.weight * physics::k_Gravity <= frame.maxForce && frame.weight < glm::length(result.force));

	// The pull turns it about its base, against its heaviness and a damping that grows with its spin
	const glm::vec3 torque = frame.leans ? glm::cross(grip - tug.base, result.force) / k_TugTurnInertia : glm::vec3(0.0f);
	const float damping = glm::length(tug.spin) * k_TugTurnDamping * frame.seconds / k_TugTurnInertia;
	tug.spin += torque * frame.seconds - tug.spin * damping;
	const auto halfBefore = tug.base + tug.axes[1] * (0.5f * frame.height);
	const auto turn = tug.spin * frame.seconds;
	if (const float angle = glm::length(turn); angle > 0.0f)
	{
		tug.axes = glm::mat3(glm::rotate(glm::mat4(1.0f), angle, turn / angle)) * tug.axes;
	}
	// How fast its middle moves as it leans, the speed it has when it comes free
	const auto halfAfter = tug.base + tug.axes[1] * (0.5f * frame.height);
	tug.pullVelocity = frame.leans && frame.seconds > 0.0f ? (halfAfter - halfBefore) / frame.seconds : glm::vec3(0.0f);
	return result;
}

glm::vec3 hand_grab::TugHandPoint(const Tug& tug, float holdDistance, float stretch)
{
	const float length = glm::length(tug.axes[1]);
	const auto up = length > 0.0f ? tug.axes[1] / length : tug.axes[1];
	return tug.base + up * (holdDistance * stretch);
}

bool hand_grab::IsThrow(glm::vec3 velocity, bool byCreature)
{
	const float across = velocity.x * velocity.x + velocity.z * velocity.z;
	return across > (byCreature ? k_CreatureThrowSpeedSquared : k_HandThrowSpeedSquared);
}

bool hand_grab::IsFastRelease(glm::vec3 velocity)
{
	return glm::dot(velocity, velocity) > k_FastReleaseSpeedSquared;
}

bool hand_grab::PotPours(glm::vec3 velocity)
{
	return glm::dot(velocity, velocity) <= k_PourSpeedSquared;
}

glm::vec3 hand_grab::ReleaseSpinTorque(float mass, float speed, glm::vec3 handMoved)
{
	// A twist about the level axis across the hand's motion since it let go, its turning sense as the bodies count it
	const float strength = k_ReleaseSpinFactor * mass * speed;
	return {strength * handMoved.z, 0.0f, -strength * handMoved.x};
}

bool hand_grab::LandsOnRelease(const Landing& landing)
{
	if (landing.thrown || (landing.raised && !landing.computerVillager))
	{
		return false;
	}
	const bool ground =
	    landing.dryLand || (landing.nearestAltitude.has_value() && *landing.nearestAltitude > k_LowestLandingAltitude);
	if (!ground)
	{
		return false;
	}
	return !landing.needsGentleSlope || landing.normalY >= k_LeastLandingNormalY;
}

LandedOutcome hand_grab::OutcomeOfLanding(const LandedThing& thing)
{
	if (!thing.living && !thing.fence && !(thing.tree && !thing.burning && thing.onLand))
	{
		return LandedOutcome::Settles;
	}
	// A tree a god's hand held tilted, or mustn't plant again, falls rather than being planted
	if (thing.tree && !thing.byCreature &&
	    (thing.dontReplant || std::abs(thing.tiltX) > k_UprightTilt || std::abs(thing.tiltZ) > k_UprightTilt))
	{
		return LandedOutcome::Falls;
	}
	return LandedOutcome::LeavesPhysics;
}

namespace
{
/// The game turns a scoop takes to ramp up: its seconds at ten turns a second, whole
int32_t ScoopRampTurns(const ScoopFacts& facts)
{
	constexpr float k_TurnsPerSecond = 1000.0f / static_cast<float>(k_TurnMs);
	return static_cast<int32_t>(k_TurnsPerSecond * facts.rampSeconds);
}
} // namespace

float hand_grab::ScoopRamp(uint32_t turns, const ScoopFacts& facts)
{
	const auto rampTurns = ScoopRampTurns(facts);
	// A ramp of no turns is over at once
	const float share =
	    rampTurns > 0 ? std::clamp(static_cast<float>(turns) / static_cast<float>(rampTurns), 0.0f, 1.0f) : 1.0f;
	return share * share;
}

uint32_t hand_grab::ScoopAmount(uint32_t turns, const ScoopFacts& facts)
{
	const float ramp = ScoopRamp(turns, facts);
	// The difference is taken as the game takes it, unsigned
	const auto rise = static_cast<double>(static_cast<uint32_t>(facts.perTurnEnd - facts.perTurn));
	return static_cast<uint32_t>(static_cast<double>(facts.perTurn) + rise * static_cast<double>(ramp));
}

uint32_t hand_grab::ScoopTaken(uint32_t wanted, uint32_t sourceHas, uint32_t held, const ScoopFacts& facts)
{
	uint32_t taken = std::min(wanted, sourceHas);
	if (facts.maxPickedUp != 0)
	{
		const uint32_t room = held < facts.maxPickedUp ? facts.maxPickedUp - held : 0;
		taken = std::min(taken, room);
	}
	return taken;
}

std::optional<float> hand_grab::RayTriangle(glm::vec3 origin, glm::vec3 direction, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	constexpr float k_Parallel = 1e-8f;
	const auto edge1 = b - a;
	const auto edge2 = c - a;
	const auto p = glm::cross(direction, edge2);
	const float det = glm::dot(edge1, p);
	if (std::abs(det) < k_Parallel)
	{
		return std::nullopt;
	}
	const float inverse = 1.0f / det;
	const auto s = origin - a;
	const float u = glm::dot(s, p) * inverse;
	if (u < 0.0f || u > 1.0f)
	{
		return std::nullopt;
	}
	const auto q = glm::cross(s, edge1);
	const float v = glm::dot(direction, q) * inverse;
	if (v < 0.0f || u + v > 1.0f)
	{
		return std::nullopt;
	}
	const float t = glm::dot(edge2, q) * inverse;
	if (t < 0.0f)
	{
		return std::nullopt;
	}
	return t;
}
