/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureLocomotionSystem.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <chrono>
#include <limits>

#include <LNDFile.h>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/mat3x3.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "Creature/CreatureAnimation.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
using openblack::creature::CreatureRig;
namespace locomotion = openblack::creature_locomotion;
namespace route = openblack::creature_route;
using Motion = CreatureLocomotion::Motion;

namespace
{
constexpr float k_TurnMs = std::chrono::duration<float, std::milli>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnSeconds = k_TurnMs / 1000.0f;
/// Lattice points searched for a route each turn
constexpr size_t k_PlanBudget = 4000;
/// A new destination this close to the current one carries on the same walk
constexpr float k_SameDestination = 0.1f;
/// Fidgets while waiting for a route: the first comes after this long, a look about is followed by a longer wait, and
/// a puzzled face is held this long
constexpr float k_FidgetMs = 3000.0f;
constexpr float k_LookAboutFidgetMs = 6000.0f;
constexpr float k_PuzzledMs = 2000.0f;
constexpr size_t k_PuzzledFace = creature_layers::animations::k_FirstFace + 6;
/// Following something, the walk is pointed anew once it has moved this far from where the walk heads
constexpr float k_FollowRetarget = 1.0f;
/// Running away, a creature goes up to this much further than its species' distance, and looks this far round the
/// point for somewhere to stand
constexpr uint32_t k_RunAwayExtra = 40;
constexpr float k_RunAwaySearch = 30.0f;
/// Arriving where it ran away to, anywhere within this
constexpr float k_RunAwayArrival = 5.0f;
/// Looking for somewhere to stand after straying off where it can
constexpr float k_StraySearch = 1000.0f;
/// Routes' corners are rounded at the creature's radius, and no tighter than this
constexpr float k_MinCornerRadius = 6.0f;
/// What is known of a species without the game's creature tables: the ape's speeds
constexpr float k_DefaultSlowFraction = 0.5f;
constexpr float k_DefaultWalkFraction = 0.7f;
constexpr float k_DefaultRunFraction = 0.9f;
constexpr float k_DefaultRunAwayDistance = 50.0f;
/// Turning or stepping, the steps never move it less than this a turn
constexpr float k_Tiny = 1e-4f;

struct Fractions
{
	float slow;
	float walk;
	float run;
	float runAway;
};

Fractions FractionsOf(CreatureType species)
{
	if (Locator::infoConstants::has_value())
	{
		const auto& creatures = Locator::infoConstants::value().creature;
		const auto row = creature::InfoRow(species);
		if (row < creatures.size())
		{
			const auto& info = creatures.at(row);
			return {.slow = info.slowSpeed, .walk = info.walkSpeed, .run = info.runSpeed, .runAway = info.runAwayDistance};
		}
	}
	return {.slow = k_DefaultSlowFraction,
	        .walk = k_DefaultWalkFraction,
	        .run = k_DefaultRunFraction,
	        .runAway = k_DefaultRunAwayDistance};
}

/// The species' animations, as its base mesh has them: the blends follow the base's lengths and moves
struct Moves
{
	const CreatureRig* rig {nullptr};

	[[nodiscard]] const skeletal_animation::Animation* Get(size_t index) const
	{
		if (rig == nullptr)
		{
			return nullptr;
		}
		const auto* animation = rig->GetAnimation(CreatureRig::Mesh::Base, index);
		return animation != nullptr && !animation->frames.empty() ? animation : nullptr;
	}
	[[nodiscard]] bool Has(size_t index) const { return Get(index) != nullptr; }
	[[nodiscard]] float Duration(size_t index) const
	{
		const auto* animation = Get(index);
		return animation != nullptr ? static_cast<float>(std::max(animation->duration, 1u)) : 1.0f;
	}
	[[nodiscard]] glm::vec2 Displacement(size_t index) const
	{
		const auto* animation = Get(index);
		return animation != nullptr ? glm::vec2(animation->displacement.x, animation->displacement.z) : glm::vec2(0.0f);
	}
	[[nodiscard]] locomotion::Cycle Cycle(size_t index) const
	{
		return {.durationMs = Duration(index), .stride = glm::length(Displacement(index))};
	}
	[[nodiscard]] bool CanWalk() const { return Has(locomotion::animations::k_Walk) && Has(locomotion::animations::k_Run); }
	[[nodiscard]] bool HasAll(size_t first) const { return Has(first) && Has(first + 1) && Has(first + 2); }
	[[nodiscard]] float PairDuration(const locomotion::Pair& pair) const
	{
		return Duration(pair.from) + ((Duration(pair.to) - Duration(pair.from)) * pair.weight);
	}
	[[nodiscard]] glm::vec2 PairDisplacement(const locomotion::Pair& pair) const
	{
		return Displacement(pair.from) + ((Displacement(pair.to) - Displacement(pair.from)) * pair.weight);
	}
};

Moves MovesOf(CreatureType species)
{
	auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto id = creature::GetRigId(species);
	return {.rig = rigs.Contains(id) ? &*rigs.Handle(id) : nullptr};
}

float HeightAt(glm::vec2 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
}

float HeadingOfRotation(const glm::mat3& rotation)
{
	// The body is turned about y; its mesh looks back along +z
	return std::atan2(rotation[2].x, rotation[2].z);
}

/// The creature's size and speeds this turn
void Measure(CreatureLocomotion& self, const Creature& creature, const Transform& transform)
{
	self.speeds = locomotion::SpeedsFor(creature.size);
	self.scale = std::abs(transform.scale.x);
	auto& meshes = Locator::resources::value().GetMeshes();
	const auto meshId = creature::GetIdFromType(creature.species, creature::CreatureBody::Appearance::Base);
	if (meshes.Contains(meshId))
	{
		// Its radius is the mean of its mesh's half width and half length
		const auto size = meshes.Handle(meshId)->GetBoundingBox().Size();
		self.radius = self.scale * (size.x + size.z) * 0.25f;
	}
}

void SetIdle(CreatureLocomotion& self)
{
	self.motion = Motion::Standing;
	self.speed = 0.0f;
	self.move.reset();
	self.tracks.clear();
}

void ClearWalk(CreatureLocomotion& self)
{
	SetIdle(self);
	self.planner.reset();
	self.route = {};
	self.routeReady = false;
	self.destination.reset();
	self.facingOnly = false;
}

glm::vec2 PositionOf(const CreatureLocomotion& self)
{
	return glm::xz(self.toPosition);
}

void PlaceAt(CreatureLocomotion& self, glm::vec2 point)
{
	self.toPosition = glm::vec3(point.x, HeightAt(point), point.y);
}

/// The two animations of a turn or step, through the same fraction of each
void TrackPair(CreatureLocomotion& self, const Moves& moves, float fromMs, float advanceMs)
{
	if (!self.move.has_value() || !self.move->animations.has_value())
	{
		return;
	}
	const auto& pair = *self.move->animations;
	const auto duration = self.move->durationMs;
	for (const auto& [animation, weight] : {std::pair {pair.from, 1.0f - pair.weight}, std::pair {pair.to, pair.weight}})
	{
		const auto own = moves.Duration(animation);
		const auto ratio = duration > 0.0f ? own / duration : 0.0f;
		self.tracks.push_back({.animation = animation,
		                       .fromMs = fromMs * ratio,
		                       .advanceMs = advanceMs * ratio,
		                       .durationMs = own,
		                       .weight = weight,
		                       .looping = false,
		                       .breathing = false});
	}
}

/// Turns on the spot or steps off towards the end of the route's segment, or walks straight on, by how far round it is
void BeginLeg(CreatureLocomotion& self, const Moves& moves)
{
	const auto position = PositionOf(self);
	const auto target = self.route.Finished() ? self.route.points.back() : self.route.points[self.route.segment + 1];
	const auto offset = target - position;
	if (glm::length(offset) <= k_Tiny)
	{
		self.motion = Motion::Walking;
		return;
	}
	self.targetHeading = locomotion::HeadingOf(offset);
	const auto angle = locomotion::WrapAngle(self.targetHeading - self.heading);
	const auto side = locomotion::SideOf(angle);
	const auto firstStep =
	    side == locomotion::Side::Right ? locomotion::animations::k_RightStep : locomotion::animations::k_LeftStep;
	const auto firstSpin =
	    side == locomotion::Side::Right ? locomotion::animations::k_RightSpin : locomotion::animations::k_LeftSpin;
	locomotion::StartOptions options {.stepStrides = std::nullopt, .hasSpins = moves.HasAll(firstSpin), .moving = true};
	if (moves.HasAll(firstStep))
	{
		options.stepStrides =
		    std::array {glm::length(moves.Displacement(firstStep)), glm::length(moves.Displacement(firstStep + 1)),
		                glm::length(moves.Displacement(firstStep + 2))};
	}
	const auto distanceInMesh = self.scale > 0.0f ? glm::length(offset) / self.scale : 0.0f;
	const auto start = locomotion::ChooseStart(angle, distanceInMesh, options);
	self.speed = 0.0f;
	switch (start.kind)
	{
	case locomotion::Start::Kind::Walk:
		self.motion = Motion::Walking;
		self.move.reset();
		break;
	case locomotion::Start::Kind::Step:
		self.motion = Motion::Stepping;
		self.move = CreatureLocomotion::Move {.animations = start.animations,
		                                      .timeMs = 0.0f,
		                                      .durationMs = moves.PairDuration(*start.animations),
		                                      .displacement = moves.PairDisplacement(*start.animations),
		                                      .startHeading = self.heading,
		                                      .startRouteHeading = self.route.Heading()};
		break;
	case locomotion::Start::Kind::Turn:
		self.motion = Motion::Turning;
		self.move = CreatureLocomotion::Move {.animations = start.animations,
		                                      .timeMs = 0.0f,
		                                      .durationMs = start.animations.has_value()
		                                                        ? moves.PairDuration(*start.animations)
		                                                        : moves.Duration(locomotion::animations::k_Stand),
		                                      .displacement = glm::vec2(0.0f),
		                                      .startHeading = self.heading,
		                                      .startRouteHeading = self.route.Heading()};
		break;
	}
}

void Arrive(CreatureLocomotion& self)
{
	ClearWalk(self);
	self.failed = false;
}

void TurnStep(CreatureLocomotion& self, const Moves& moves, float playbackRate)
{
	auto& move = *self.move;
	const auto advance = k_TurnMs * playbackRate;
	const auto from = move.timeMs;
	move.timeMs = std::min(move.timeMs + advance, move.durationMs);
	if (move.animations.has_value())
	{
		// The spin's own root turns the body; its heading changes once the turn is over
		TrackPair(self, moves, from, move.timeMs - from);
	}
	else
	{
		self.heading = locomotion::LerpHeading(move.startHeading, self.targetHeading, move.timeMs, move.durationMs);
	}
	if (move.timeMs < move.durationMs)
	{
		return;
	}
	self.heading = self.targetHeading;
	self.move.reset();
	self.tracks.clear();
	if (self.facingOnly || self.route.points.empty())
	{
		ClearWalk(self);
		return;
	}
	BeginLeg(self, moves);
}

void StepOff(CreatureLocomotion& self, const Moves& moves)
{
	auto& move = *self.move;
	const auto from = move.timeMs;
	move.timeMs = std::min(move.timeMs + k_TurnMs, move.durationMs);
	const auto fraction = move.durationMs > 0.0f ? (move.timeMs - from) / move.durationMs : 1.0f;
	// The step's move carries it along the route
	const auto distance = glm::length(move.displacement) * self.scale * fraction;
	self.speed = distance / k_TurnSeconds;
	self.distance = distance;
	const auto advanced = route::Advance(self.route, distance);
	PlaceAt(self, advanced.position);
	if (advanced.finished)
	{
		Arrive(self);
		return;
	}
	// The step's own root turns the body round towards the route as it steps; its heading follows once the step ends
	TrackPair(self, moves, from, move.timeMs - from);

	if (std::abs(locomotion::WrapAngle(advanced.heading - move.startRouteHeading)) > locomotion::k_CornerAngle)
	{
		self.tracks.clear();
		self.move.reset();
		BeginLeg(self, moves);
		return;
	}
	if (move.timeMs >= move.durationMs)
	{
		// It leaves the step already walking, along the route
		self.heading = advanced.heading;
		self.move.reset();
		self.motion = Motion::Walking;
		self.speed = self.speeds.walk;
	}
}

void Walk(CreatureLocomotion& self, const Moves& moves, float targetSpeed, float breathPhase)
{
	if (self.route.Finished())
	{
		Arrive(self);
		return;
	}
	auto cap = locomotion::StopCap(self.route.RemainingTotal());
	if (std::abs(self.route.TurnAtEnd()) >= locomotion::k_CornerAngle)
	{
		// Too sharp a corner to walk round: it slows to stop there
		cap = std::min(cap, locomotion::CornerCap(self.route.Remaining(), 0.0f));
	}
	const auto position = PositionOf(self);
	const auto ahead = position + (locomotion::DirectionOf(self.heading) * self.radius);
	const auto slope = locomotion::SlopeFactor(HeightAt(ahead), HeightAt(position), self.radius);
	self.speed = std::min(locomotion::Accelerate(self.speed, slope * targetSpeed, k_TurnSeconds), cap);

	const auto distance = self.speed * k_TurnSeconds;
	self.distance = distance;
	const auto advanced = route::Advance(self.route, distance);
	PlaceAt(self, advanced.position);
	if (advanced.finished)
	{
		Arrive(self);
		return;
	}
	if (std::abs(locomotion::WrapAngle(advanced.heading - self.heading)) >= locomotion::k_CornerAngle)
	{
		self.speed = 0.0f;
		BeginLeg(self, moves);
		return;
	}
	self.heading = advanced.heading;

	const auto walkFrom = self.walkTimeMs;
	const auto stand = moves.Cycle(locomotion::animations::k_Stand);
	const auto walk = moves.Cycle(locomotion::animations::k_Walk);
	const auto run = moves.Cycle(locomotion::animations::k_Run);
	const auto gait =
	    locomotion::BlendGait(self.speed, self.speeds, distance, self.scale, stand, walk, run, self.walkTimeMs, breathPhase);
	self.walkTimeMs = gait.walkTimeMs;
	for (const auto& slot : gait.slots)
	{
		CreatureLocomotion::Track track {.animation = slot.animation,
		                                 .fromMs = walkFrom,
		                                 .advanceMs = gait.walkAdvanceMs,
		                                 .durationMs = walk.durationMs,
		                                 .weight = slot.weight,
		                                 .looping = true,
		                                 .breathing = false};
		if (slot.animation == locomotion::animations::k_Stand)
		{
			track.durationMs = stand.durationMs;
			track.breathing = true;
		}
		else if (slot.animation == locomotion::animations::k_Run)
		{
			// The run keeps in step with the walk
			const auto ratio = walk.durationMs > 0.0f ? run.durationMs / walk.durationMs : 0.0f;
			track.fromMs = walkFrom * ratio;
			track.advanceMs = gait.walkAdvanceMs * ratio;
			track.durationMs = run.durationMs;
		}
		self.tracks.push_back(track);
	}
}

/// What is in the way of a walk from one point to another, as circles grown by the creature's radius
std::vector<route::Circle> GatherObstacles(ecs::Registry& registry, entt::entity self, glm::vec2 from, glm::vec2 to,
                                           float creatureHeight, float radius)
{
	std::vector<route::Circle> circles;
	// Only what lies near the way there is worth walking round
	const auto reach = std::max(100.0f, 0.5f * glm::distance(from, to));
	const auto near = [&](glm::vec2 point, float size) { return route::DistanceToSegment(point, from, to) <= reach + size; };
	auto& meshes = Locator::resources::value().GetMeshes();
	registry.Each<const Fixed, const Transform, const Mesh>([&](entt::entity entity, const Fixed& fixed,
	                                                            const Transform& transform, const Mesh& mesh) {
		if (!near(fixed.boundingCenter, fixed.boundingRadius))
		{
			return;
		}
		const auto height =
		    meshes.Contains(mesh.id) ? std::abs(transform.scale.y) * meshes.Handle(mesh.id)->GetBoundingBox().Size().y : 0.0f;
		auto kind = route::Obstacle::Object;
		if (registry.AnyOf<Tree>(entity))
		{
			kind = route::Obstacle::Tree;
		}
		else if (registry.AnyOf<Abode>(entity))
		{
			kind = route::Obstacle::Building;
		}
		if (route::MustAvoid(kind, height, creatureHeight, false))
		{
			circles.push_back({.centre = fixed.boundingCenter, .radius = fixed.boundingRadius + radius});
		}
	});
	registry.Each<const CreatureLocomotion, const Transform>(
	    [&](entt::entity entity, const CreatureLocomotion& other, const Transform& transform) {
		    const auto centre = glm::xz(transform.position);
		    const auto kind =
		        other.motion == Motion::Standing ? route::Obstacle::StandingCreature : route::Obstacle::MovingCreature;
		    if (entity != self && near(centre, other.radius) && route::MustAvoid(kind, 0.0f, creatureHeight, false))
		    {
			    circles.push_back({.centre = centre, .radius = other.radius + radius});
		    }
	    });
	return circles;
}

/// How far round something reaches, for walking up to it
float RadiusOf(ecs::Registry& registry, entt::entity entity)
{
	if (const auto* other = registry.TryGet<const CreatureLocomotion>(entity))
	{
		return other->radius;
	}
	if (const auto* fixed = registry.TryGet<const Fixed>(entity))
	{
		return fixed->boundingRadius;
	}
	return 1.0f;
}

bool IsMovingMotion(Motion motion)
{
	return motion != Motion::Standing;
}
} // namespace

const route::WalkableLand& CreatureLocomotionSystem::GetWalkableLand()
{
	if (!_land.has_value())
	{
		if (!Locator::terrainSystem::has_value())
		{
			_land.emplace();
			return *_land;
		}
		const auto& land = Locator::terrainSystem::value();
		_land = route::WalkableLand::Build(
		    [&land](int32_t x, int32_t z) {
			    return land.GetHeightAt(glm::vec2(static_cast<float>(x), static_cast<float>(z)) * route::k_CellSize);
		    },
		    [&land](int32_t x, int32_t z) -> std::optional<bool> {
			    const auto* cell = land.FindCell(glm::u16vec2(static_cast<uint16_t>(x), static_cast<uint16_t>(z)));
			    if (cell == nullptr)
			    {
				    return std::nullopt;
			    }
			    return cell->properties.hasWater != 0;
		    });
	}
	return *_land;
}

CreatureLocomotionSystem::MoveResult CreatureLocomotionSystem::StartMove(entt::entity creature, CreatureLocomotion& self,
                                                                         glm::vec2 point, float fraction, float minDistance,
                                                                         float maxDistance)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	if (body == nullptr || !MovesOf(body->species).CanWalk())
	{
		return MoveResult::Busy;
	}
	const auto& land = GetWalkableLand();
	if (!land.IsValid(point, route::k_DestinationClearance))
	{
		return MoveResult::InvalidDestination;
	}
	self.fraction = fraction;
	self.failed = false;
	self.facingOnly = false;
	if (IsMovingMotion(self.motion) && self.destination.has_value() &&
	    glm::distance(*self.destination, point) <= k_SameDestination)
	{
		return MoveResult::Started;
	}

	// Straying somewhere it can't stand, it is put back on the nearest place it can first
	auto position = PositionOf(self);
	if (!land.IsValid(position, route::k_Clearance))
	{
		if (const auto valid = land.NearestValid(position, route::k_Clearance, k_StraySearch))
		{
			position = *valid;
			PlaceAt(self, position);
			self.fromPosition = self.toPosition;
		}
	}
	const auto distance = glm::distance(position, point);
	self.ring = locomotion::MoveRing(distance, minDistance, maxDistance);
	self.destination = point;
	self.planningMs = locomotion::TimeLimitMs(distance);
	self.fidgetMs = k_FidgetMs;
	self.fidgeted = false;

	const auto creatureHeight = creature_morph::k_HeightAtSizeOne * body->size;
	self.planner.emplace(route::Request {
	    .start = position,
	    .destination = point,
	    .minDistance = self.ring.min,
	    .maxDistance = self.ring.max,
	    .obstacles = GatherObstacles(registry, creature, position, point, creatureHeight, self.radius),
	    .cornerRadius = std::max(self.radius, k_MinCornerRadius),
	});
	self.routeReady = false;
	// Planned at once, a creature already walking carries on along its new route
	if (self.planner->Step(land, k_PlanBudget) == route::Planner::Status::Found && self.motion == Motion::Walking)
	{
		self.route = {.points = self.planner->GetRoute(), .segment = 0, .travelled = 0.0f};
		self.planner.reset();
		return MoveResult::Started;
	}
	self.route = {};
	SetIdle(self);
	self.motion = Motion::Planning;
	return MoveResult::Started;
}

CreatureLocomotionSystem::MoveResult CreatureLocomotionSystem::MoveTo(entt::entity creature, glm::vec2 point, Pace pace,
                                                                      float minDistance, float maxDistance)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* self = registry.TryGet<CreatureLocomotion>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	if (self == nullptr || body == nullptr || !self->started)
	{
		return MoveResult::Busy;
	}
	self->following.reset();
	const auto fractions = FractionsOf(body->species);
	return StartMove(creature, *self, point, pace == Pace::Run ? fractions.run : fractions.walk, minDistance, maxDistance);
}

CreatureLocomotionSystem::MoveResult CreatureLocomotionSystem::MoveToObject(entt::entity creature, entt::entity target,
                                                                            Pace pace, float extra)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* self = registry.TryGet<CreatureLocomotion>(creature);
	const auto* at = registry.TryGet<const Transform>(target);
	if (self == nullptr || at == nullptr)
	{
		return MoveResult::Busy;
	}
	const auto ring = locomotion::ObjectRing(self->radius, RadiusOf(registry, target), extra);
	return MoveTo(creature, glm::xz(at->position), pace, ring.min, ring.max);
}

CreatureLocomotionSystem::MoveResult CreatureLocomotionSystem::Follow(entt::entity creature, entt::entity target,
                                                                      float distance, Pace pace)
{
	const auto result = MoveToObject(creature, target, pace, distance);
	if (auto* self = Locator::entitiesRegistry::value().TryGet<CreatureLocomotion>(creature);
	    self != nullptr && result != MoveResult::Busy)
	{
		self->following = target;
		self->followDistance = distance;
	}
	return result;
}

CreatureLocomotionSystem::MoveResult CreatureLocomotionSystem::FleeFrom(entt::entity creature, glm::vec2 threat)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* self = registry.TryGet<CreatureLocomotion>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	if (self == nullptr || body == nullptr || !self->started)
	{
		return MoveResult::Busy;
	}
	const auto fractions = FractionsOf(body->species);
	const auto distance =
	    static_cast<float>(std::uniform_int_distribution<uint32_t>(0, k_RunAwayExtra - 1)(_random)) + fractions.runAway;
	auto point = locomotion::RunAwayPoint(PositionOf(*self), threat, distance);
	const auto& land = GetWalkableLand();
	if (!land.IsValid(point, route::k_DestinationClearance))
	{
		const auto valid = land.NearestValid(point, route::k_DestinationClearance, k_RunAwaySearch);
		if (!valid.has_value())
		{
			return MoveResult::InvalidDestination;
		}
		point = *valid;
	}
	self->following.reset();
	return StartMove(creature, *self, point, fractions.run, 0.0f, k_RunAwayArrival);
}

bool CreatureLocomotionSystem::TurnToFace(entt::entity creature, glm::vec2 point)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* self = registry.TryGet<CreatureLocomotion>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	if (self == nullptr || body == nullptr || !self->started)
	{
		return false;
	}
	const auto offset = point - PositionOf(*self);
	if (glm::length(offset) <= k_Tiny)
	{
		return false;
	}
	ClearWalk(*self);
	self->following.reset();
	self->facingOnly = true;
	self->targetHeading = locomotion::HeadingOf(offset);
	const auto angle = locomotion::WrapAngle(self->targetHeading - self->heading);
	const auto moves = MovesOf(body->species);
	const auto firstSpin = locomotion::SideOf(angle) == locomotion::Side::Right ? locomotion::animations::k_RightSpin
	                                                                            : locomotion::animations::k_LeftSpin;
	const auto start = locomotion::ChooseStart(
	    angle, 0.0f, {.stepStrides = std::nullopt, .hasSpins = moves.HasAll(firstSpin), .moving = false});
	if (start.kind != locomotion::Start::Kind::Turn)
	{
		// Nearly facing it already
		self->heading = self->targetHeading;
		self->facingOnly = false;
		return true;
	}
	self->motion = Motion::Turning;
	self->move =
	    CreatureLocomotion::Move {.animations = start.animations,
	                              .timeMs = 0.0f,
	                              .durationMs = start.animations.has_value() ? moves.PairDuration(*start.animations)
	                                                                         : moves.Duration(locomotion::animations::k_Stand),
	                              .displacement = glm::vec2(0.0f),
	                              .startHeading = self->heading,
	                              .startRouteHeading = self->heading};
	return true;
}

void CreatureLocomotionSystem::Stop(entt::entity creature)
{
	if (auto* self = Locator::entitiesRegistry::value().TryGet<CreatureLocomotion>(creature))
	{
		ClearWalk(*self);
		self->following.reset();
	}
}

bool CreatureLocomotionSystem::IsMoving(entt::entity creature) const
{
	const auto* self = Locator::entitiesRegistry::value().TryGet<const CreatureLocomotion>(creature);
	return self != nullptr && IsMovingMotion(self->motion);
}

void CreatureLocomotionSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& land = GetWalkableLand();

	// Following something, the walk is pointed anew as it moves away
	std::vector<std::pair<entt::entity, entt::entity>> retarget;
	registry.Each<CreatureLocomotion>([&](entt::entity entity, CreatureLocomotion& self) {
		if (!self.following.has_value() || !self.started)
		{
			return;
		}
		const auto* at = registry.Valid(*self.following) ? registry.TryGet<const Transform>(*self.following) : nullptr;
		if (at == nullptr)
		{
			self.following.reset();
			return;
		}
		const auto target = glm::xz(at->position);
		const bool moved = IsMovingMotion(self.motion) && self.destination.has_value() &&
		                   glm::distance(target, *self.destination) > k_FollowRetarget;
		const bool left =
		    !IsMovingMotion(self.motion) && glm::distance(target, PositionOf(self)) > self.ring.max + k_FollowRetarget;
		if (moved || left)
		{
			retarget.emplace_back(entity, *self.following);
		}
	});
	for (const auto& [entity, target] : retarget)
	{
		const auto& self = registry.Get<CreatureLocomotion>(entity);
		const auto& body = registry.Get<Creature>(entity);
		const auto distance = self.followDistance;
		const auto fractions = FractionsOf(body.species);
		Follow(entity, target, distance, self.fraction >= fractions.run ? Pace::Run : Pace::Walk);
	}

	registry.Each<const Creature, CreatureLocomotion, const Transform, CreatureAnimation>(
	    [&](entt::entity entity, const Creature& creature, CreatureLocomotion& self, const Transform& transform,
	        CreatureAnimation& animation) {
		    if (!self.started)
		    {
			    self.started = true;
			    self.heading = HeadingOfRotation(transform.rotation);
			    self.toPosition = transform.position;
			    self.toHeading = self.heading;
		    }
		    self.fromPosition = self.toPosition;
		    self.fromHeading = self.toHeading;
		    self.tracks.clear();
		    self.distance = 0.0f;
		    Measure(self, creature, transform);
		    const auto moves = MovesOf(creature.species);

		    if (self.puzzledMs > 0.0f)
		    {
			    self.puzzledMs -= k_TurnMs;
			    if (self.puzzledMs <= 0.0f && animation.face.wanted == k_PuzzledFace)
			    {
				    animation.face.wanted.reset();
			    }
		    }

		    // The route is planned a little each turn
		    if (self.planner.has_value())
		    {
			    const auto status = self.planner->Step(land, k_PlanBudget);
			    self.planningMs -= k_TurnMs;
			    if (status == route::Planner::Status::Found)
			    {
				    self.route = {.points = self.planner->GetRoute(), .segment = 0, .travelled = 0.0f};
				    self.routeReady = true;
				    self.planner.reset();
			    }
			    else if (status == route::Planner::Status::Failed || self.planningMs <= 0.0f)
			    {
				    // No way there: it stands where it is
				    ClearWalk(self);
				    self.failed = true;
			    }
		    }

		    const auto* mind = registry.TryGet<const CreatureMindState>(entity);
		    const auto fractions = FractionsOf(creature.species);
		    const auto fraction =
		        locomotion::RequiredFraction(self.fraction, fractions.slow, mind != nullptr ? mind->exhaustion : 0.0f);
		    const auto targetSpeed = locomotion::TargetSpeed(fraction, self.speeds.run);

		    switch (self.motion)
		    {
		    case Motion::Standing:
			    self.speed = 0.0f;
			    break;
		    case Motion::Planning:
			    if (self.routeReady)
			    {
				    // Sitting or playing an action, it gets up and finishes first
				    if (creature_layers::IsLooping(animation.body))
				    {
					    animation.body = creature_layers::EndLoop(animation.body);
				    }
				    if (!creature_layers::IsPlaying(animation.body))
				    {
					    self.routeReady = false;
					    BeginLeg(self, moves);
				    }
				    break;
			    }
			    self.fidgetMs -= k_TurnMs;
			    if (self.fidgetMs <= 0.0f)
			    {
				    // The first fidget is always a look about
				    const auto choice = self.fidgeted ? std::uniform_int_distribution<uint32_t>(0, 2)(_random) : 0u;
				    self.fidgeted = true;
				    self.fidgetMs = k_FidgetMs;
				    if (choice == 0)
				    {
					    self.fidgetMs = k_LookAboutFidgetMs;
				    }
				    else if (choice == 1)
				    {
					    animation.face.wanted = k_PuzzledFace;
					    self.puzzledMs = k_PuzzledMs;
				    }
				    else if (auto confused = creature_layers::PlayOnce(animation.body, creature_layers::animations::k_Confused,
				                                                       std::bernoulli_distribution(0.5)(_random)))
				    {
					    animation.body = *confused;
					    self.motion = Motion::Confused;
				    }
			    }
			    break;
		    case Motion::Confused:
			    if (!creature_layers::IsPlaying(animation.body))
			    {
				    self.motion = Motion::Planning;
			    }
			    break;
		    case Motion::Turning:
			    TurnStep(self, moves, creature_layers::PlaybackRate(creature.size));
			    break;
		    case Motion::Stepping:
			    StepOff(self, moves);
			    break;
		    case Motion::Walking:
			    Walk(self, moves, targetSpeed, animation.breathPhase);
			    break;
		    }

		    // Strayed somewhere it can't stand, it is put back where it can and sets off again
		    if ((self.motion == Motion::Walking || self.motion == Motion::Stepping) && self.destination.has_value() &&
		        !land.IsValid(PositionOf(self), route::k_Clearance))
		    {
			    if (const auto valid = land.NearestValid(PositionOf(self), route::k_Clearance, k_StraySearch))
			    {
				    PlaceAt(self, *valid);
				    self.fromPosition = self.toPosition;
				    const auto destination = *self.destination;
				    const auto ring = self.ring;
				    SetIdle(self);
				    StartMove(entity, self, destination, self.fraction, ring.min, ring.max);
			    }
		    }
		    self.toHeading = self.heading;
	    });
}

void CreatureLocomotionSystem::Update(float turnFraction)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto f = std::clamp(turnFraction, 0.0f, 1.0f);
	bool moved = false;
	registry.Each<CreatureLocomotion, Transform, CreatureAnimation>(
	    [&](CreatureLocomotion& self, Transform& transform, CreatureAnimation& animation) {
		    if (!self.started)
		    {
			    return;
		    }
		    const auto position = self.fromPosition + ((self.toPosition - self.fromPosition) * f);
		    const auto heading = self.fromHeading + (locomotion::WrapAngle(self.toHeading - self.fromHeading) * f);
		    const auto rotation = glm::mat3(glm::eulerAngleY(heading));
		    if (position != transform.position || rotation != transform.rotation)
		    {
			    transform.position = position;
			    transform.rotation = rotation;
			    moved = true;
		    }
		    animation.slots.clear();
		    for (const auto& track : self.tracks)
		    {
			    auto time = track.fromMs + (track.advanceMs * f);
			    if (track.breathing)
			    {
				    time = static_cast<float>(creature_animation::BreathTime(
				        animation.breathPhase, static_cast<uint32_t>(std::max(track.durationMs, 1.0f))));
			    }
			    else if (track.looping)
			    {
				    time = track.durationMs > 0.0f ? std::fmod(time, track.durationMs) : 0.0f;
			    }
			    else
			    {
				    time = std::clamp(time, 0.0f, std::max(track.durationMs - 1.0f, 0.0f));
			    }
			    animation.slots.push_back(
			        {.animation = track.animation, .timeMs = time, .weight = track.weight, .mirrored = false});
		    }
	    });
	if (moved)
	{
		registry.SetDirty();
	}
}
