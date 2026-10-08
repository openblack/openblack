/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// A creature's mind casting miracles: whether it may try one, how it goes near, gets away from and turns to face what it
// casts at, and its try at the miracle. A try short of all but one of the sightings it needs fizzles; it counts as one
// more sighting, and the creature shows its embarrassment once it chooses what to do next, and may be sad about it too.
// A miracle it can't cast, too tired or at something it can't be cast at, leaves it frustrated: hungry above all else.

#define LOCATOR_IMPLEMENTATIONS

#include <cmath>

#include <algorithm>
#include <numbers>

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/mat2x2.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "Creature/CreatureCastMoves.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureMindTables.h"
#include "Creature/CreatureSpellCasting.h"
#include "Creature/CreatureSpellMind.h"
#include "Creature/CreatureWatching.h"
#include "CreatureMindSystem.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureCasting.h"
#include "ECS/Components/CreatureFight.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "ObjectMeasures.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
namespace cast_moves = openblack::creature_cast_moves;
using creature_desires::Desire;
using Kind = creature_mind::Movement::Kind;

constexpr float k_TurnsPerSecond = 10.0f;
/// Going near something, it looks every this many turns whether it has got anywhere, and gives up when it has moved
/// less than this since
constexpr uint32_t k_StuckTurns = 50;
constexpr float k_StuckDistance = 0.01f;
/// Getting stuck holds back the desire it went for this many seconds
constexpr float k_StuckSuppressSeconds = 10.0f;
/// It sets off again only for somewhere further than this from where it set off for
constexpr float k_SameDestination = 0.1f;
/// It heads for what it goes near within this share of a whole turn, measured in the game's 2048 to a turn
constexpr int32_t k_AngleUnits = 2048;
constexpr int32_t k_HeadingTolerance = 6;
/// A fizzled try: embarrassed, then one time in two sad too, its sadness growing by up to this much
constexpr size_t k_Embarrassed = 73;
constexpr float k_SadnessFromFizzle = 0.3f;
/// A creature goes round a thing at least this share of its height
constexpr float k_LowObjectShare = 0.1f;
/// The miracles a creature casts are numbered from the first to the last magic type
constexpr uint32_t k_LastCreatureMagic = 41;
/// A creature tries a power-up miracle only once grown past this stage
constexpr uint32_t k_PowerUpPhase = 7;

ecs::Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

glm::vec2 Flat(const glm::vec3& point)
{
	return {point.x, point.z};
}

/// The game's heading of a creature, from the way it faces: 0 facing -z, a quarter turn facing +x
float GameHeading(const Transform& transform)
{
	const auto ahead = -(transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	return std::atan2(ahead.x, -ahead.z);
}

/// Whether a creature heads for a point near enough, or stands in the same cell of the land as it
bool HeadingFor(const Transform& transform, glm::vec2 point)
{
	const auto at = Flat(transform.position);
	if (std::floor(at.x / cast_moves::k_CellSize) == std::floor(point.x / cast_moves::k_CellSize) &&
	    std::floor(at.y / cast_moves::k_CellSize) == std::floor(point.y / cast_moves::k_CellSize))
	{
		return true;
	}
	constexpr float k_UnitsPerRadian = static_cast<float>(k_AngleUnits) / (2.0f * std::numbers::pi_v<float>);
	const auto heading = static_cast<int32_t>((GameHeading(transform) - std::numbers::pi_v<float> * 0.5f) * k_UnitsPerRadian) &
	                     (k_AngleUnits - 1);
	const auto d = point - at;
	const auto towards =
	    (k_AngleUnits / 4 + static_cast<int32_t>(std::atan2(-d.x, d.y) * k_UnitsPerRadian)) & (k_AngleUnits - 1);
	// The game takes the difference without going round
	return std::abs(heading - towards) < k_AngleUnits / k_HeadingTolerance;
}

/// The circles a fixed thing keeps creatures off, from its model's box and its place
std::vector<cast_moves::CollideCircle> ModelCircles(const ecs::Registry& registry, entt::entity entity)
{
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	const auto* transform = registry.TryGet<const Transform>(entity);
	if (mesh == nullptr || transform == nullptr || !Locator::resources::has_value() ||
	    !Locator::resources::value().GetMeshes().Contains(mesh->id))
	{
		return {};
	}
	const auto& box = Locator::resources::value().GetMeshes().Handle(mesh->id)->GetBoundingBox();
	const auto half = (box.maxima - box.minima) * 0.5f * transform->scale.x;
	const auto centre = transform->position + transform->rotation * (box.Center() * transform->scale);
	const glm::mat2 turn(transform->rotation[0].x, transform->rotation[0].z, transform->rotation[2].x,
	                     transform->rotation[2].z);
	return cast_moves::CollideCircles({half.x, half.z}, Flat(centre), turn);
}

/// Whether a thing standing in a cell of the land keeps it from being clear for a creature to get away to: the trees and
/// fields always by the circles they keep clear, anything else fixed when the creature must go round it (a building,
/// feature or static thing when as tall as a tenth of the creature or burning, a dead tree only burning), and nothing
/// that moves
bool CellBlocked(const ecs::Registry& registry, entt::entity creature, int32_t x, int32_t z)
{
	if (!Locator::entitiesMap::has_value() || x < 0 || z < 0)
	{
		return false;
	}
	const auto& map = Locator::entitiesMap::value();
	const ecs::MapInterface::CellId cell {static_cast<uint16_t>(x), static_cast<uint16_t>(z)};
	const float creatureHeight = object_measures::Height(registry, creature);
	const auto burning = [](entt::entity entity) {
		return Locator::fireSystem::has_value() && Locator::fireSystem::value().IsOnFire(entity);
	};
	const auto blocks = [&](entt::entity entity) {
		if (!registry.Valid(entity) || registry.AllOf<Forest>(entity))
		{
			return false;
		}
		std::vector<cast_moves::CollideCircle> circles;
		if (registry.AllOf<Tree>(entity) && !registry.AllOf<DeadTree>(entity))
		{
			if (const auto* at = registry.TryGet<const Transform>(entity))
			{
				circles.push_back({.centre = Flat(at->position), .radius = cast_moves::k_TreeCollideRadius});
			}
		}
		else
		{
			const bool tallEnough = object_measures::Height(registry, entity) >= k_LowObjectShare * creatureHeight;
			bool test = false;
			if (registry.AnyOf<Field, AnimatedStatic>(entity))
			{
				test = true;
			}
			else if (registry.AllOf<DeadTree>(entity))
			{
				test = burning(entity);
			}
			else if (registry.AnyOf<Abode, Feature, MobileStatic>(entity))
			{
				test = tallEnough || burning(entity);
			}
			if (!test)
			{
				return false;
			}
			circles = ModelCircles(registry, entity);
		}
		return std::ranges::any_of(circles, [x, z](const auto& circle) { return cast_moves::BlocksCell(circle, x, z); });
	};
	return std::ranges::any_of(map.GetFixedInGridCell(cell), blocks) ||
	       std::ranges::any_of(map.GetMobileInGridCell(cell), blocks);
}

/// How many times a creature must see a miracle to learn it, and how often it has
struct Sightings
{
	float seen;
	float needed;
	bool known;
};
std::optional<Sightings> SightingsOf(const CreatureMindState& mind, const creature_mind_tables::Tables& tables,
                                     CreatureType species, uint32_t magicType)
{
	if (!mind.learnt.has_value() || magicType >= tables.miracles.size())
	{
		return std::nullopt;
	}
	const auto& knowledge = mind.learnt->knowledge;
	if (magicType >= knowledge.miraclesSeen.size() || magicType >= knowledge.miraclesKnown.size())
	{
		return std::nullopt;
	}
	return Sightings {.seen = static_cast<float>(knowledge.miraclesSeen[magicType].count),
	                  .needed =
	                      creature_watching::TimesNeeded(tables.miracles[magicType].timesToSee,
	                                                     creature_mind_tables::MiracleMultiplier(creature::InfoRow(species))),
	                  .known = knowledge.miraclesKnown[magicType]};
}

/// Whether the creature's body can pay for making a miracle
bool CanPayFor(entt::entity creature, MagicType type)
{
	const auto& registry = Entities();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* needs = registry.TryGet<const CreatureNeeds>(creature);
	const auto& info = Locator::infoConstants::value();
	if (body == nullptr || needs == nullptr || creature::InfoRow(body->species) >= info.creature.size())
	{
		return false;
	}
	const auto& species = info.creature.at(creature::InfoRow(body->species));
	return creature_spell_casting::CanCast(
	    {.size = body->size, .strength = body->strength, .energy = needs->needs.energy, .exhaustion = needs->needs.exhaustion},
	    {.chantsPerEnergy = species.chantsPerEnergy,
	     .energyFloor = species.spellEnergyFloor,
	     .sizeFactor = species.spellSizeFactor},
	    magic::GetMagicEffectInfo(info, type).costToCreate);
}
} // namespace

std::optional<uint32_t> CreatureMindSystem::CastMagicOf(uint32_t action)
{
	const auto& info = Locator::infoConstants::value();
	if (action >= info.creatureAction.size())
	{
		return std::nullopt;
	}
	const auto magic = info.creatureAction.at(action).magicType;
	if (magic == 0 || magic > k_LastCreatureMagic)
	{
		return std::nullopt;
	}
	return magic;
}

bool CreatureMindSystem::MayCast(entt::entity creature, const CreatureMindState& mind, uint32_t action, bool powerUp)
{
	const auto* tables = GetTables();
	const auto magic = CastMagicOf(action);
	const auto* body = Entities().TryGet<const Creature>(creature);
	if (tables == nullptr || !magic.has_value() || body == nullptr || (powerUp && mind.developmentPhase <= k_PowerUpPhase))
	{
		return false;
	}
	const auto sightings = SightingsOf(mind, *tables, body->species, *magic);
	// The game goes by the sightings alone
	if (!sightings.has_value() || !creature_spell_casting::MayTry(sightings->seen, sightings->needed))
	{
		return false;
	}
	return CanPayFor(creature, static_cast<MagicType>(*magic));
}

std::optional<creature_plan_actions::CastInfo> CreatureMindSystem::CastInfoFor(entt::entity creature, uint32_t action)
{
	const auto magic = CastMagicOf(action);
	const auto* body = Entities().TryGet<const Creature>(creature);
	if (!magic.has_value() || body == nullptr)
	{
		return std::nullopt;
	}
	const auto& info = Locator::infoConstants::value();
	const auto type = static_cast<MagicType>(*magic);
	// The miracle's own gesture, power-ups too: none in the game's tables
	const auto gesture = magic::GetMagicInfo(info, type).gestureType;
	return creature_plan_actions::CastInfo {
	    .magicType = *magic, .gesture = static_cast<uint32_t>(gesture), .height = cast_moves::k_HeightOfSizeOne * body->size};
}

bool CreatureMindSystem::TryMiracle(entt::entity creature, MagicType type, entt::entity target)
{
	auto& registry = Entities();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* tables = GetTables();
	if (mind == nullptr || body == nullptr || tables == nullptr || !Locator::magicSystem::has_value())
	{
		return false;
	}
	auto& magic = Locator::magicSystem::value();
	magic.ReleaseCreatureCast(creature);
	const auto& info = Locator::infoConstants::value();
	// In a fight every miracle costs stamina, and it can't cast one it hasn't the stamina for
	if (auto* fighting = registry.TryGet<CreatureFighting>(creature))
	{
		const float cost = creature_spell_casting::StaminaCost(magic::GetMagicEffectInfo(info, type).costToCreate);
		auto& stamina = fighting->fighter.stamina;
		if (stamina < cost)
		{
			return false;
		}
		stamina = std::clamp(stamina - cost, 0.0f, 1.0f);
	}
	// A try short of all but one of the sightings it needs fizzles, and counts as one more
	const auto magicType = static_cast<uint32_t>(type);
	if (const auto seen = SightingsOf(*mind, *tables, body->species, magicType))
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Creature {} tries miracle {}: seen {} of {}{}", entt::to_integral(creature),
		                    magicType, seen->seen, seen->needed, seen->known ? ", known" : "");
	}
	if (const auto sightings = SightingsOf(*mind, *tables, body->species, magicType);
	    sightings.has_value() && !creature_spell_casting::TrySucceeds(sightings->seen, sightings->needed))
	{
		auto* casting = registry.TryGet<CreatureCasting>(creature);
		(casting != nullptr ? *casting : registry.Assign<CreatureCasting>(creature)).fizzle =
		    CreatureCasting::Fizzle {.target = target, .magicType = type};
		++mind->learnt->knowledge.miraclesSeen[magicType].count;
		return false;
	}
	// It is frustrated only when it hadn't the energy for the miracle or the miracle couldn't be cast at the target
	if (magic.CastByCreature(creature, type, target))
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Creature {} cast miracle {}", entt::to_integral(creature), magicType);
		return true;
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Creature {} couldn't cast miracle {}", entt::to_integral(creature), magicType);
	// Frustrated, it wants food above all
	if (mind->desires.has_value() && creature::InfoRow(body->species) < info.creature.size())
	{
		creature_spell_mind::MakeFullyDominantOverOthers(*mind->desires, Desire::Hunger,
		                                                 info.creature.at(creature::InfoRow(body->species)).desireFloor);
	}
	return false;
}

void CreatureMindSystem::ShowFizzle(entt::entity creature, CreatureMindState& mind)
{
	auto* casting = Entities().TryGet<CreatureCasting>(creature);
	if (casting == nullptr || !casting->fizzle.has_value())
	{
		return;
	}
	const auto fizzle = *casting->fizzle;
	casting->fizzle.reset();
	// Embarrassed, then one time in two sad as well, and sadder for it
	std::vector<creature_mind::Step> agenda {{.kind = creature_mind::Step::Kind::Action, .animation = k_Embarrassed}};
	if (Random(2) == 0)
	{
		agenda.push_back({.kind = creature_mind::Step::Kind::Action, .animation = creature_layers::animations::k_Sad});
		if (mind.desires.has_value())
		{
			creature_desires::ChangeSource(*mind.desires, creature_desires::sources::k_Sadness, Chance() * k_SadnessFromFizzle);
		}
	}
	if (!Replan(creature, creature_mind::Activity::Planned, std::move(agenda)))
	{
		return;
	}
	// It is showing how it feels about what it tried
	if (const auto* tables = GetTables())
	{
		if (const auto action = creature_mind_tables::FindAction(*tables, "CommunicateState"))
		{
			mind.planner.current = creature_planner::Plan {
			    .desire = Desire::ManifestState,
			    .action = *action,
			    .object = Entities().Valid(fizzle.target) ? std::optional(entt::to_integral(fizzle.target)) : std::nullopt};
			mind.planActive = true;
			mind.planSerial = mind.idle.serial;
			mind.agendaSeen = mind.idle.serial;
		}
	}
}

void CreatureMindSystem::StartSubMove(entt::entity creature, const creature_mind::Movement& movement, float seconds)
{
	auto& registry = Entities();
	auto* existing = registry.TryGet<CreatureCasting>(creature);
	auto& casting = existing != nullptr ? *existing : registry.Assign<CreatureCasting>(creature);
	casting.target = movement.object.has_value() ? static_cast<entt::entity>(*movement.object) : entt::null;
	casting.keep = movement.maxDistance;
	casting.settleSeconds = seconds;
	casting.phase = 0;
	casting.turns = 0;
	casting.holdTurns = 0;
	casting.destination.reset();
	casting.outcome = CreatureCasting::Outcome::Running;
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Creature {}: {} {} keeping {:.1f}, its radius {:.2f}, the target's {:.2f}",
	                    entt::to_integral(creature), static_cast<int>(movement.kind), entt::to_integral(casting.target),
	                    casting.keep, object_measures::TwoDRadius(registry, creature),
	                    registry.Valid(casting.target) ? object_measures::RoutePlanRadius(registry, casting.target, creature)
	                                                   : 0.0f);
	switch (movement.kind)
	{
	case Kind::GoNearObject:
		casting.move = CreatureCasting::Move::GoNear;
		break;
	case Kind::GetAwayFromObject:
		casting.move = CreatureCasting::Move::GetAway;
		break;
	case Kind::TurnToFaceObject:
		casting.move = CreatureCasting::Move::TurnToFace;
		break;
	default:
		casting.move = CreatureCasting::Move::None;
		return;
	}
	const auto* animation = registry.TryGet<const CreatureAnimation>(creature);
	StepSubMove(creature, animation != nullptr && creature_layers::IsPlaying(animation->body));
}

bool CreatureMindSystem::GoNear(entt::entity creature, CreatureCasting& casting, bool reissue)
{
	auto& registry = Entities();
	const auto at = object_measures::PositionOf(registry, casting.target);
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (!at.has_value() || transform == nullptr || !Locator::creatureLocomotionSystem::has_value())
	{
		return false;
	}
	// It stops short of the thing by what it keeps clear of and its own radius, and no further than the distance more
	const float clear =
	    object_measures::RoutePlanRadius(registry, casting.target, creature) + object_measures::TwoDRadius(registry, creature);
	const auto target = Flat(*at);
	if (reissue && casting.destination.has_value() && glm::distance(*casting.destination, target) < k_SameDestination)
	{
		return true;
	}
	const float distance = glm::distance(Flat(transform->position), target);
	const float near = std::min(clear, distance - 0.05f);
	const float far = std::max(casting.keep + clear, near + 0.001f);
	// TODO(raffclar): the game walks it on at whatever speed it was last asked to go, which stopping what it does sets back
	// to walking for every creature but a computer player's; nothing here keeps that speed, so it walks
	const auto result = Locator::creatureLocomotionSystem::value().MoveTo(
	    creature, target, CreatureLocomotionSystemInterface::Pace::Walk, near, far);
	if (result != CreatureLocomotionSystemInterface::MoveResult::Started)
	{
		return false;
	}
	casting.destination = target;
	return true;
}

void CreatureMindSystem::StepSubMove(entt::entity creature, bool animating)
{
	auto& registry = Entities();
	auto* casting = registry.TryGet<CreatureCasting>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (casting == nullptr || casting->move == CreatureCasting::Move::None ||
	    casting->outcome != CreatureCasting::Outcome::Running || transform == nullptr ||
	    !Locator::creatureLocomotionSystem::has_value())
	{
		return;
	}
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	const auto at = object_measures::PositionOf(registry, casting->target);
	const auto here = Flat(transform->position);
	// Playing an action, walking or turning: the game counts all as its body doing something
	const bool bodyBusy = animating || locomotion.IsMoving(creature);
	const auto fail = [&] { casting->outcome = CreatureCasting::Outcome::Failed; };
	const auto done = [&] { casting->outcome = CreatureCasting::Outcome::Done; };
	const bool start = casting->phase == 0;
	switch (casting->move)
	{
	case CreatureCasting::Move::GoNear:
	{
		const bool itself = at.has_value() && object_measures::WalkedUpToItself(registry, casting->target);
		if (start)
		{
			// A building or a tree it walks up to by its walk alone
			if (itself && !GoNear(creature, *casting, false))
			{
				casting->destination.reset();
			}
			casting->stuckCheck = here;
			casting->phase = 1;
			casting->turns = 0;
			break;
		}
		++casting->turns;
		if (itself && !bodyBusy)
		{
			done();
			break;
		}
		const float routeRadius = at.has_value() ? object_measures::RoutePlanRadius(registry, casting->target, creature)
		                                         : object_measures::TwoDRadius(registry, creature);
		const auto target = at.has_value() ? Flat(*at) : here;
		const float selfRadius = object_measures::TwoDRadius(registry, creature);
		if (cast_moves::Arrived(glm::distance(here, target), selfRadius, routeRadius, casting->keep))
		{
			locomotion.Stop(creature);
			done();
			break;
		}
		if (!locomotion.IsMoving(creature) || !HeadingFor(*transform, target))
		{
			if (!GoNear(creature, *casting, locomotion.IsMoving(creature)))
			{
				fail();
				break;
			}
		}
		// It gives up when it has got nowhere for a while, and wants what it went for less for a while
		auto* mindState = registry.TryGet<CreatureMindState>(creature);
		const uint32_t count = mindState != nullptr ? mindState->stepTurns : casting->turns;
		if (count % k_StuckTurns == 0)
		{
			casting->stuckCheck = here;
		}
		else if (count % k_StuckTurns == k_StuckTurns - 1 && glm::distance(here, casting->stuckCheck) < k_StuckDistance)
		{
			if (mindState != nullptr && mindState->desires.has_value() && mindState->planActive &&
			    mindState->planner.current.has_value())
			{
				creature_desires::Suppress(*mindState->desires, mindState->planner.current->desire, k_StuckSuppressSeconds,
				                           k_TurnsPerSecond);
			}
			fail();
		}
		break;
	}
	case CreatureCasting::Move::GetAway:
	{
		if (casting->phase == 0)
		{
			// Not while it plays an action or turns on the spot, only standing or walking
			const auto* moves = registry.TryGet<const CreatureLocomotion>(creature);
			if (animating || (moves != nullptr && moves->motion == CreatureLocomotion::Motion::Turning))
			{
				break;
			}
			if (!at.has_value())
			{
				done();
				break;
			}
			const float far =
			    cast_moves::GetAwayDistance(casting->keep, object_measures::TwoDRadius(registry, casting->target));
			if (!(glm::distance(Flat(*at), here) < far))
			{
				done();
				break;
			}
			const auto away = cast_moves::GetAwayPoint(transform->position, *at, far);
			const auto clear =
			    cast_moves::FindClearArea(Flat(away), object_measures::Height(registry, creature), [&](int32_t x, int32_t z) {
				    return locomotion.GetWalkableLand().At(x, z) == creature_route::Ground::Open &&
				           !CellBlocked(registry, creature, x, z);
			    });
			if (!clear.has_value() || locomotion.MoveTo(creature, *clear, CreatureLocomotionSystemInterface::Pace::Walk, 0.0f,
			                                            1.0f) != CreatureLocomotionSystemInterface::MoveResult::Started)
			{
				fail();
				break;
			}
			casting->phase = 1;
			break;
		}
		if (!bodyBusy)
		{
			done();
		}
		break;
	}
	case CreatureCasting::Move::TurnToFace:
	{
		if (!at.has_value())
		{
			done();
			break;
		}
		const auto target = Flat(*at);
		const auto* moving = registry.TryGet<const CreatureLocomotion>(creature);
		const float heading = moving != nullptr ? moving->heading : 0.0f;
		const bool onTop = glm::distance(here, target) < cast_moves::k_OnTopDistance;
		if (casting->phase == 0)
		{
			// Right on top of it there is nothing to turn to, but it still settles as it would
			if (onTop)
			{
				casting->phase = 1;
				break;
			}
			locomotion.Stop(creature);
			if (locomotion.TurnToFace(creature, target))
			{
				casting->phase = 1;
			}
			break;
		}
		if (bodyBusy)
		{
			break;
		}
		if (onTop && casting->phase == 1)
		{
			casting->phase = 2;
			break;
		}
		if (onTop)
		{
			done();
			break;
		}
		const bool facing = cast_moves::Facing(here, heading, target);
		if (casting->phase == 1)
		{
			if (!facing)
			{
				locomotion.TurnToFace(creature, target);
				break;
			}
			casting->holdTurns = static_cast<uint32_t>(casting->settleSeconds * k_TurnsPerSecond);
			casting->phase = 2;
			break;
		}
		if (casting->holdTurns == 0)
		{
			done();
			break;
		}
		--casting->holdTurns;
		if (!facing)
		{
			locomotion.TurnToFace(creature, target);
		}
		break;
	}
	case CreatureCasting::Move::None:
		break;
	}
}

creature_mind::SubMove CreatureMindSystem::SubMoveOf(entt::entity creature)
{
	const auto* casting = Entities().TryGet<const CreatureCasting>(creature);
	if (casting == nullptr || casting->move == CreatureCasting::Move::None)
	{
		return creature_mind::SubMove::Running;
	}
	switch (casting->outcome)
	{
	case CreatureCasting::Outcome::Done:
		return creature_mind::SubMove::Done;
	case CreatureCasting::Outcome::Failed:
		return creature_mind::SubMove::Failed;
	case CreatureCasting::Outcome::Running:
		break;
	}
	return creature_mind::SubMove::Running;
}

void CreatureMindSystem::KnowMiracle(entt::entity creature, size_t miracle)
{
	auto* mind = Entities().TryGet<CreatureMindState>(creature);
	if (mind == nullptr || !mind->learnt.has_value() || miracle >= mind->learnt->knowledge.miraclesKnown.size())
	{
		return;
	}
	// As if it had seen it as often as it needs to learn it
	const auto* body = Entities().TryGet<const Creature>(creature);
	const auto* tables = GetTables();
	if (body != nullptr && tables != nullptr && miracle < tables->miracles.size())
	{
		const auto needed = creature_watching::TimesNeeded(
		    tables->miracles[miracle].timesToSee, creature_mind_tables::MiracleMultiplier(creature::InfoRow(body->species)));
		auto& seen = mind->learnt->knowledge.miraclesSeen[miracle].count;
		seen = std::max(seen, static_cast<uint32_t>(std::ceil(needed)));
	}
	mind->learnt->knowledge.miraclesKnown[miracle] = true;
}

bool CreatureMindSystem::TellCast(entt::entity creature, MagicType type, entt::entity target)
{
	auto& registry = Entities();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	const auto* tables = GetTables();
	if (mind == nullptr || tables == nullptr || !registry.Valid(target))
	{
		return false;
	}
	for (uint32_t action = 0; action < tables->actions.size(); ++action)
	{
		const auto* executor = creature_plan_actions::For(tables->actions[action].name);
		if (executor == nullptr || !creature_plan_actions::IsCast(*executor) ||
		    CastMagicOf(action) != static_cast<uint32_t>(type) ||
		    tables->actions[action].desire >= creature_desires::k_DesireCount)
		{
			continue;
		}
		// Told by the debug tools, it goes about the casting as it would by itself, without weighing other plans
		auto cast = CastInfoFor(creature, action);
		auto agenda = creature_plan_actions::Agenda(
		    *executor, entt::to_integral(target),
		    object_measures::PositionOf(registry, target).has_value() ? Flat(*object_measures::PositionOf(registry, target))
		                                                              : glm::vec2(0.0f),
		    {}, [this](uint32_t range) { return Random(range); }, cast);
		return agenda.has_value() && Replan(creature, creature_mind::Activity::Told, std::move(*agenda));
	}
	return false;
}
