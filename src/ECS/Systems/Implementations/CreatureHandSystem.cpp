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
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/LandIslandInterface.h"
#include "Creature/CreatureFeedback.h"
#include "Creature/CreatureHandRules.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureSpells.h"
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
	return feedback::BodyCapsules(animation.skeleton.parents, animation.boneMatrices,
	                              creature::PlacementMatrix(transform.position, transform.rotation, transform.scale),
	                              feedback::k_BodyRadiusShare * feedback::k_HeightAtSizeOne * creature.size);
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
	const auto nearest = CreatureAlong(rayOrigin, rayDirection);
	if (!nearest.has_value() || !MayHold(*nearest))
	{
		return false;
	}
	auto& registry = Locator::entitiesRegistry::value();
	registry.AssignOrReplace<HandOnCreature>(PlayerHand(), HandOnCreature {.creature = *nearest});
	registry.Remove<HandLastFeedback>(PlayerHand());
	return true;
}

bool CreatureHandSystem::MayHold(entt::entity creature) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.Valid(creature) ? registry.TryGet<const Creature>(creature) : nullptr;
	if (body == nullptr)
	{
		return false;
	}
	const auto* mind = registry.TryGet<const CreatureMindState>(creature);
	const auto* spells = registry.TryGet<const CreatureSpells>(creature);
	const creature_hand::Holdable holdable {
	    .owner = body->owner,
	    .species = body->species,
	    .asleep = mind != nullptr && creature_mind::IsAsleep(mind->idle),
	    .frozen = spells != nullptr && spells->spells.IsActive(creature_spells::Spell::Freeze),
	};
	if (!creature_hand::MayHold(holdable))
	{
		SPDLOG_LOGGER_INFO(spdlog::get("game"),
		                   "The hand can't hold creature {}: it belongs to nobody, is an ogre, or is asleep or frozen",
		                   entt::to_integral(creature));
		return false;
	}
	return true;
}

bool CreatureHandSystem::IsClick() const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* contact = Locator::handSystem::has_value() ? registry.TryGet<const HandOnCreature>(PlayerHand()) : nullptr;
	return contact != nullptr && !contact->byCommand && creature_hand::IsClick(contact->heldMs, contact->strokedOrSlapped);
}

std::optional<entt::entity> CreatureHandSystem::CreatureAlong(const glm::vec3& rayOrigin, const glm::vec3& rayDirection) const
{
	if (!Locator::handSystem::has_value())
	{
		return std::nullopt;
	}
	std::optional<entt::entity> nearest;
	float best = k_RayLength;
	Locator::entitiesRegistry::value().Each<const Creature, const CreatureAnimation, const Transform>(
	    [&](entt::entity entity, const Creature& creature, const CreatureAnimation& animation, const Transform& transform) {
		    const auto body = BodyOf(creature, animation, transform);
		    if (const auto hit = feedback::RayHit(rayOrigin, rayDirection, body); hit.has_value() && *hit < best)
		    {
			    best = *hit;
			    nearest = entity;
		    }
	    });
	return nearest;
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
	const auto height = feedback::k_HeightAtSizeOne * creature.size;
	const auto centre = transform.position + glm::vec3(0.0f, height * 0.5f, 0.0f);

	// Held by a command, the hand stays on the part it last stroked, or where it last slapped, whatever the cursor does
	if (contact->byCommand)
	{
		contact->slapShowMs = std::max(contact->slapShowMs - ms, 0.0f);
		contact->sinceStrokeMs += ms;
		contact->sinceSlapMs += ms;
		if (contact->lastPart.has_value())
		{
			if (const auto parts = PartsOf(creature, animation, transform))
			{
				contact->lastPoint = parts->at(static_cast<size_t>(*contact->lastPart));
			}
		}
		return HandPose {
		    .position = contact->lastPoint.value_or(centre), .onBody = true, .slapping = contact->slapShowMs > 0.0f};
	}

	// Where the hand is: on the body under the cursor, or beside it on the plane through the creature
	const auto onPlane = OnPlaneThrough(centre, rayOrigin, rayDirection);
	const auto body = BodyOf(creature, animation, transform);
	const auto hit = feedback::RayHit(rayOrigin, rayDirection, body);
	const auto touch = hit.has_value() ? std::optional(rayOrigin + (rayDirection * *hit)) : std::nullopt;
	const auto point = onPlane.value_or(touch.value_or(centre));

	contact->heldMs += ms;
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
			contact->strokedOrSlapped = true;
			if (minds.ForceAction(creatureEntity, slap->animation, slap->mirrored, std::nullopt,
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
			                      feedback::RewardFace(part), feedback::k_StrokeInterruptsAfter))
			{
				contact->sum = feedback::AfterStroke(contact->sum);
				contact->lastPart = part;
				contact->sinceStrokeMs = 0.0f;
				contact->strokedOrSlapped = true;
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
	const auto sum = contact->sum;
	const auto delivered = feedback::Delivered(sum);
	registry.Remove<HandOnCreature>(hand);
	registry.AssignOrReplace<HandLastFeedback>(hand, HandLastFeedback {.sum = sum});
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

HandOnCreature* CreatureHandSystem::HoldByCommand(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!Locator::handSystem::has_value() || !registry.Valid(creature) ||
	    !registry.AllOf<Creature, CreatureAnimation, Transform>(creature) || !MayHold(creature))
	{
		return nullptr;
	}
	const auto hand = PlayerHand();
	if (auto* contact = registry.TryGet<HandOnCreature>(hand); contact != nullptr && contact->creature == creature)
	{
		contact->byCommand = true;
		return contact;
	}
	// Held to another creature, the hand lets go of it first, which tells it how it was treated
	Release();
	registry.Remove<HandLastFeedback>(hand);
	return &registry.AssignOrReplace<HandOnCreature>(hand, HandOnCreature {.creature = creature, .byCommand = true});
}

bool CreatureHandSystem::Stroke(entt::entity creature, feedback::BodyPart part)
{
	auto* contact = HoldByCommand(creature);
	if (contact == nullptr || !Locator::creatureMindSystem::has_value())
	{
		return false;
	}
	const auto index = static_cast<size_t>(part);
	contact->lastPart = part;
	if (!Locator::creatureMindSystem::value().ForceAction(creature, feedback::k_RewardAnimations.at(index),
	                                                      feedback::k_RewardMirrored.at(index), feedback::RewardFace(part),
	                                                      feedback::k_StrokeInterruptsAfter))
	{
		return false;
	}
	contact->sum = feedback::AfterStroke(contact->sum);
	contact->sinceStrokeMs = 0.0f;
	return true;
}

bool CreatureHandSystem::Slap(entt::entity creature, float heightShare, bool gentle, bool sweepsRight)
{
	auto* contact = HoldByCommand(creature);
	if (contact == nullptr || !Locator::creatureMindSystem::has_value())
	{
		return false;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const auto height = feedback::k_HeightAtSizeOne * registry.Get<const Creature>(creature).size;
	// As fast across the body as a gentle slap or a hard one is
	const auto speed =
	    (gentle ? (feedback::k_SlapSpeed + feedback::k_HardSlapSpeed) * 0.5f : feedback::k_HardSlapSpeed * 1.5f) * height;
	const auto slap = feedback::ClassifySlap(heightShare * height, speed, height, sweepsRight);
	if (!slap.has_value())
	{
		return false;
	}
	contact->lastPart.reset();
	contact->lastPoint = registry.Get<const Transform>(creature).position + glm::vec3(0.0f, heightShare * height, 0.0f);
	contact->sinceSlapMs = 0.0f;
	contact->slapShowMs = k_SlapShowMs;
	if (!Locator::creatureMindSystem::value().ForceAction(creature, slap->animation, slap->mirrored, std::nullopt,
	                                                      feedback::k_SlapInterruptsAfter))
	{
		return false;
	}
	contact->sum = feedback::AfterSlap(contact->sum, slap->gentle);
	return true;
}

bool CreatureHandSystem::IsHeldByCommand() const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* contact = Locator::handSystem::has_value() ? registry.TryGet<const HandOnCreature>(PlayerHand()) : nullptr;
	return contact != nullptr && contact->byCommand;
}

float CreatureHandSystem::GetLastFeedbackSum() const
{
	const auto& registry = Locator::entitiesRegistry::value();
	if (!Locator::handSystem::has_value())
	{
		return 0.0f;
	}
	if (const auto* contact = registry.TryGet<const HandOnCreature>(PlayerHand()))
	{
		return contact->sum;
	}
	const auto* last = registry.TryGet<const HandLastFeedback>(PlayerHand());
	return last != nullptr ? last->sum : 0.0f;
}
