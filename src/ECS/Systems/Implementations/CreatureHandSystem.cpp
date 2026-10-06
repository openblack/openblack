/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureHandSystem.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/CreatureBody.h"
#include "3D/LandIslandInterface.h"
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/HandOnCreature.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using openblack::creature::CreatureRig;
namespace feedback = openblack::creature_feedback;

namespace
{
/// A creature of size 1 is about this many units tall
constexpr float k_HeightAtSizeOne = 15.0f;
/// The body is taken as capsules round its bones this thick, as shares of its height
constexpr float k_BodyRadiusShare = 0.12f;
/// The hand's speed is eased over about this many seconds, so one jerky frame doesn't slap
constexpr float k_SpeedEaseSeconds = 0.05f;
/// The hand shows its slap this long
constexpr float k_SlapShowMs = 300.0f;
/// The furthest a line of sight is followed to a creature
constexpr float k_RayLength = 1e5f;

entt::entity PlayerHand()
{
	return Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
}

const CreatureRig::ActionPoints* PointsOf(const Creature& creature)
{
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto id = creature::GetRigId(creature.species);
	if (!rigs.Contains(id))
	{
		return nullptr;
	}
	const auto& rig = *rigs.Handle(id);
	return rig.actionPoints.has_value() ? &*rig.actionPoints : nullptr;
}

/// The creature's body as the hand can touch it this frame: capsules round its bones, in the world
std::vector<feedback::Capsule> BodyOf(const Creature& creature, const CreatureAnimation& animation, const Transform& transform)
{
	std::vector<feedback::Capsule> capsules;
	const auto placement = creature::PlacementMatrix(transform.position, transform.rotation, transform.scale);
	const auto radius = k_BodyRadiusShare * k_HeightAtSizeOne * creature.size;
	const auto& parents = animation.skeleton.parents;
	for (uint32_t bone = 0; bone < parents.size() && bone < animation.boneMatrices.size(); ++bone)
	{
		const auto parent = parents[bone];
		const auto to = glm::vec3(creature::PosedBone(bone, animation.boneMatrices, placement)[3]);
		const auto from = parent < animation.boneMatrices.size()
		                      ? glm::vec3(creature::PosedBone(parent, animation.boneMatrices, placement)[3])
		                      : to;
		capsules.push_back({.from = from, .to = to, .radius = radius});
	}
	return capsules;
}

/// Where each part of the body a stroke can land on is this frame, in the world
std::optional<std::array<glm::vec3, feedback::k_BodyPartCount>>
PartsOf(const Creature& creature, const CreatureAnimation& animation, const Transform& transform)
{
	const auto* points = PointsOf(creature);
	if (points == nullptr)
	{
		return std::nullopt;
	}
	const auto placement = creature::PlacementMatrix(transform.position, transform.rotation, transform.scale);
	const auto at = [&](uint32_t bone) { return glm::vec3(creature::PosedBone(bone, animation.boneMatrices, placement)[3]); };
	const auto mirror = [&](uint32_t bone) { return bone < animation.mirror.size() ? animation.mirror[bone] : bone; };
	return std::array<glm::vec3, feedback::k_BodyPartCount> {
	    at(points->head),
	    at(points->rightArmpit),
	    at(mirror(points->rightArmpit)),
	    at(points->belly),
	    at(points->groin),
	    at(points->rightFoot),
	    at(mirror(points->rightFoot)),
	    at(points->rightHand),
	    at(mirror(points->rightHand)),
	};
}

/// Where a line of sight meets the upright plane through the creature that faces back along it
std::optional<glm::vec3> OnPlaneThrough(const glm::vec3& centre, const glm::vec3& origin, const glm::vec3& direction)
{
	auto normal = glm::vec3(direction.x, 0.0f, direction.z);
	if (glm::length(normal) < 1e-4f)
	{
		normal = glm::vec3(0.0f, 0.0f, 1.0f);
	}
	normal = glm::normalize(normal);
	const auto facing = glm::dot(direction, normal);
	if (std::abs(facing) < 1e-6f)
	{
		return std::nullopt;
	}
	const auto along = glm::dot(centre - origin, normal) / facing;
	return along > 0.0f ? std::optional(origin + (direction * along)) : std::nullopt;
}
} // namespace

bool CreatureHandSystem::Grab(const glm::vec3& rayOrigin, const glm::vec3& rayDirection)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!Locator::handSystem::has_value())
	{
		return false;
	}
	std::optional<entt::entity> nearest;
	float best = k_RayLength;
	registry.Each<const Creature, const CreatureAnimation, const Transform>(
	    [&](entt::entity entity, const Creature& creature, const CreatureAnimation& animation, const Transform& transform) {
		    const auto body = BodyOf(creature, animation, transform);
		    if (const auto hit = feedback::RayHit(rayOrigin, rayDirection, body); hit.has_value() && *hit < best)
		    {
			    best = *hit;
			    nearest = entity;
		    }
	    });
	if (!nearest.has_value())
	{
		return false;
	}
	registry.AssignOrReplace<HandOnCreature>(PlayerHand(), HandOnCreature {.creature = *nearest});
	return true;
}

std::optional<CreatureHandSystem::HandPose>
CreatureHandSystem::Update(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, glm::vec2 cursor, float seconds)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* contact = Locator::handSystem::has_value() ? registry.TryGet<HandOnCreature>(PlayerHand()) : nullptr;
	if (contact == nullptr)
	{
		return std::nullopt;
	}
	const auto creatureEntity = contact->creature;
	if (!registry.Valid(creatureEntity) || !registry.AllOf<Creature, CreatureAnimation, Transform>(creatureEntity))
	{
		registry.Remove<HandOnCreature>(PlayerHand());
		return std::nullopt;
	}
	const auto& creature = registry.Get<const Creature>(creatureEntity);
	const auto& animation = registry.Get<const CreatureAnimation>(creatureEntity);
	const auto& transform = registry.Get<const Transform>(creatureEntity);
	const auto ms = seconds * 1000.0f;
	const auto height = k_HeightAtSizeOne * creature.size;

	// Where the hand is: on the body under the cursor, or beside it on the plane through the creature
	const auto centre = transform.position + glm::vec3(0.0f, height * 0.5f, 0.0f);
	const auto onPlane = OnPlaneThrough(centre, rayOrigin, rayDirection);
	const auto body = BodyOf(creature, animation, transform);
	const auto hit = feedback::RayHit(rayOrigin, rayDirection, body);
	const auto touch = hit.has_value() ? std::optional(rayOrigin + (rayDirection * *hit)) : std::nullopt;
	const auto point = onPlane.value_or(touch.value_or(centre));

	contact->sinceStrokeMs += ms;
	contact->sinceSlapMs += ms;
	contact->slapShowMs = std::max(contact->slapShowMs - ms, 0.0f);
	contact->onBodyMs = touch.has_value() ? contact->onBodyMs + ms : 0.0f;
	if (contact->lastPoint.has_value() && seconds > 0.0f)
	{
		const auto raw = glm::distance(point, *contact->lastPoint) / seconds;
		const auto ease = std::clamp(seconds / k_SpeedEaseSeconds, 0.0f, 1.0f);
		contact->speed += (raw - contact->speed) * ease;
	}
	const bool sweepsRight = cursor.x > contact->lastCursor.x;
	contact->lastPoint = point;
	contact->lastCursor = cursor;

	auto& minds = Locator::creatureMindSystem::value();
	const auto ground = Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(glm::xz(point))
	                                                        : transform.position.y;
	if (touch.has_value() && contact->sinceSlapMs >= feedback::k_SlapIntervalMs)
	{
		if (const auto slap = feedback::ClassifySlap(point.y - ground, contact->speed, height, sweepsRight))
		{
			// The hand slaps either way, but it only counts when the creature can reel from it
			contact->sinceSlapMs = 0.0f;
			contact->slapShowMs = k_SlapShowMs;
			if (minds.ForceAction(creatureEntity, slap->animation, slap->mirrored, std::nullopt, 0.0f,
			                      feedback::k_SlapInterruptsAfter))
			{
				contact->sum = feedback::AfterSlap(contact->sum, slap->gentle);
			}
		}
	}
	const bool slow = contact->speed <= feedback::k_SlapSpeed * height;
	if (touch.has_value() && slow && contact->onBodyMs > feedback::k_StrokeHoldMs)
	{
		if (const auto parts = PartsOf(creature, animation, transform))
		{
			const auto part = feedback::NearestPart(*touch, *parts);
			const auto index = static_cast<size_t>(part);
			if (feedback::StrokeDue(contact->lastPart, part, contact->sinceStrokeMs) &&
			    minds.ForceAction(creatureEntity, feedback::k_RewardAnimations.at(index), feedback::k_RewardMirrored.at(index),
			                      feedback::k_RewardFaces.at(index), feedback::k_RewardFaceSeconds,
			                      feedback::k_StrokeInterruptsAfter))
			{
				contact->sum = feedback::AfterStroke(contact->sum);
				contact->lastPart = part;
				contact->sinceStrokeMs = 0.0f;
			}
		}
	}
	return HandPose {.position = touch.value_or(point), .onBody = touch.has_value(), .slapping = contact->slapShowMs > 0.0f};
}

void CreatureHandSystem::Release()
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!Locator::handSystem::has_value())
	{
		return;
	}
	const auto hand = PlayerHand();
	const auto* contact = registry.TryGet<const HandOnCreature>(hand);
	if (contact == nullptr)
	{
		return;
	}
	const auto creature = contact->creature;
	const auto delivered = feedback::Delivered(contact->sum);
	registry.Remove<HandOnCreature>(hand);
	if (registry.Valid(creature) && Locator::creatureMindSystem::has_value())
	{
		Locator::creatureMindSystem::value().ReceiveFeedback(creature, delivered);
	}
}

std::optional<entt::entity> CreatureHandSystem::GetCreature() const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* contact = Locator::handSystem::has_value() ? registry.TryGet<const HandOnCreature>(PlayerHand()) : nullptr;
	return contact != nullptr ? std::optional(contact->creature) : std::nullopt;
}

float CreatureHandSystem::GetFeedbackSum() const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* contact = Locator::handSystem::has_value() ? registry.TryGet<const HandOnCreature>(PlayerHand()) : nullptr;
	return contact != nullptr ? contact->sum : 0.0f;
}
