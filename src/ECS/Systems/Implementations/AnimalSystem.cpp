/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "AnimalSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <limits>
#include <map>
#include <numbers>

#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Animals/AnimalAnimation.h"
#include "Animals/AnimalRules.h"
#include "Common/GUtilsAngle.h"
#include "Common/GameRandom.h"
#include "ECS/Archetypes/AnimalArchetype.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/PhysicsEntry.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/ReactionRules.h"
#include "Physics/LivingRules.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerFire.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace living = openblack::physics::living;
// Not "flock": on Linux that is also a function from <sys/file.h>
namespace flock_rules = openblack::magic::flock;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr auto k_TurnMilliseconds =
    static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(TimeSystemInterface::k_TurnDuration).count());
/// Game turns a second
constexpr float k_TurnsPerSecond = 1000.0f / static_cast<float>(k_TurnMilliseconds);
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The miracles' doves and bats tilt half a radian into a turn, easing into it over half a second
constexpr float k_SpellBirdBankAngle = 0.5f;
constexpr float k_SpellBirdTimeToBank = 0.5f;
/// A spell wolf eases into its tilt over a second, tilting by its speed times its turn, to at most half a radian
constexpr float k_WolfTimeToBank = 1.0f;
constexpr float k_WolfBankPerSpeedTurn = 0.5f / 20000.0f;
constexpr float k_WolfMostBank = 0.5f;
/// A wolf's run takes the fifth of its kind's speeds
constexpr size_t k_WolfRunSpeed = 4;
/// A bird on its special move within this many metres of its goal takes its usual speed
constexpr float k_SpecialMoveSlowing = 6.0f;
/// A wolf leaps at its prey from half its leap's stride (scaled) within its attack distance, else from this close
constexpr float k_PounceFromFar = 0.5f;
/// It leaps only at prey within this many game angles of the way it faces
constexpr uint32_t k_PounceAngle = 0x80;
/// A speed state times this is metres a second
constexpr float k_SpeedStateToMetresPerSecond = 100.0f / 65536.0f;
/// A wolf eats this many mouthfuls, and up to this many more
constexpr uint32_t k_MouthfulsMin = 15;
constexpr uint32_t k_MouthfulsRange = 10;
/// A random point is tried this many times, each with so many cells spiralling out from it, before falling back
constexpr int k_RandomPosTries = 2;
constexpr int k_RandomPosCells = 25;
/// An animal a miracle makes is born up to this many years past this age
constexpr uint32_t k_BirthAgeRange = 20;
constexpr uint32_t k_BirthAgeMin = 5;
/// How a flock's followers follow: in formation
constexpr int k_FollowInFormation = 3;
/// The prey brought down plays its fall the turn after, and waits for it from the turn after that
constexpr int k_FallStartsAfter = 2;
/// The clip a villager brought down plays before it is eaten
constexpr auto k_VillagerAttacked = static_cast<AnimId>(206);

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

const GAnimalInfo& InfoOf(AnimalInfo type)
{
	return Locator::infoConstants::value().animal.at(static_cast<size_t>(type));
}

float Ground(glm::vec2 xz)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

float Random(float max)
{
	return Locator::gameRandom::has_value() ? Locator::gameRandom::value().GameFloatRand(max) : 0.0f;
}

uint32_t RandomWhole(uint32_t n)
{
	return Locator::gameRandom::has_value() ? Locator::gameRandom::value().GameRand(n) : 0;
}

bool IsBird(AnimalInfo type)
{
	switch (type)
	{
	case AnimalInfo::Crow:
	case AnimalInfo::Dove:
	case AnimalInfo::Swallow:
	case AnimalInfo::Pigeon:
	case AnimalInfo::Seagull:
	case AnimalInfo::Bat:
	case AnimalInfo::Vulture:
	case AnimalInfo::CitadelDove:
	case AnimalInfo::CitadelBat:
	case AnimalInfo::SpellDove:
	case AnimalInfo::SpellBat:
		return true;
	default:
		return false;
	}
}

uint16_t SpeedStateOf(const GAnimalInfo& info, size_t index)
{
	const auto& group = info.speedGroup;
	const std::array speeds {group.speedDefault, group.speedFleeing, group.speed2, group.speed3, group.speed4, group.speed5};
	return static_cast<uint16_t>(speeds.at(index));
}

uint16_t TurnAngleOf(const Animal& animal)
{
	return static_cast<uint16_t>(InfoOf(animal.type).turnAngle);
}

glm::vec2 Metres(glm::ivec2 fixed)
{
	return {map_coords::ToMetres(fixed.x), map_coords::ToMetres(fixed.y)};
}

glm::ivec2 Fixed(glm::vec2 metres)
{
	return {map_coords::ToFixed(metres.x), map_coords::ToFixed(metres.y)};
}

glm::vec2 Xz(const glm::vec3& point)
{
	return {point.x, point.z};
}

bool OnMap(glm::vec2 point)
{
	return map_coords::InBounds(map_coords::FromMetres(point));
}

/// The clip of a state: the miracle's doves always flap their clip and its bats theirs; its wolves stand to decide,
/// leap, settle down to eat and eat, and run otherwise
AnimId ClipFor(AnimalInfo type, AnimalState state)
{
	switch (type)
	{
	case AnimalInfo::SpellDove:
		return AnimId::SpellDoveFlap;
	case AnimalInfo::SpellBat:
		return AnimId::BatFlap;
	case AnimalInfo::SpellWolf:
		switch (state)
		{
		case AnimalState::DecideWhatToDo:
			return AnimId::AWolfStand;
		case AnimalState::Pounce:
			return AnimId::AWolfPounce;
		case AnimalState::StartToEat:
			return AnimId::AWolfGotoEat;
		case AnimalState::Eat:
			return AnimId::AWolfEat;
		default:
			return AnimId::AWolfRun;
		}
	default:
		return InfoOf(type).defaultAnim;
	}
}

/// A state the animal takes, with the clip of that state: a new clip starts from its beginning
void SetTopState(Animal& animal, AnimalState state)
{
	animal.state = state;
	animal.turnsInState = 0;
	const auto clip = ClipFor(animal.type, state);
	if (animal.animation != clip)
	{
		animal.animation = clip;
		animal.clipPlace = 0;
	}
}

/// A state the animal takes keeping its clip
void SetState(Animal& animal, AnimalState state)
{
	animal.state = state;
	animal.turnsInState = 0;
}

const L3DAnim* ClipOf(AnimId clip)
{
	const auto& animations = Locator::resources::value().GetAnimations();
	const auto id = resources::HashIdentifier(static_cast<uint32_t>(clip));
	return animations.Contains(id) ? &*animations.Handle(id) : nullptr;
}

/// A clip's play time in milliseconds, 0 for none
uint32_t PlayTimeOf(AnimId clip)
{
	const auto* anim = ClipOf(clip);
	return anim != nullptr ? anim->GetPlayTime() : 0;
}

/// How far a clip carries its animal each play, in the model's units
float StrideOf(AnimId clip)
{
	const auto* anim = ClipOf(clip);
	return anim != nullptr ? anim->GetStride() : 0.0f;
}

/// An object's radius across the land: its model's widest half, across or along, times its scale
float RadiusOf(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	const auto* transform = registry.TryGet<const Transform>(entity);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (mesh == nullptr || transform == nullptr || !meshes.Contains(mesh->id))
	{
		return 0.0f;
	}
	const auto half = meshes.Handle(mesh->id)->GetBoundingBox().Size() * 0.5f;
	return std::max(half.x, half.z) * transform->scale.x;
}

/// Where the animal is in the world and the way it faces, from its move and height
void SyncWorld(Animal& animal)
{
	const auto xz = Metres(animal.move.position);
	animal.position = {xz.x, Ground(xz) + animal.height, xz.y};
	animal.heading = gutils::ConvertGameAngleTo3D(animal.move.angle);
}
} // namespace

entt::entity AnimalSystem::CreateFlock(glm::vec2 centre, float domainRadius, float flockDistance)
{
	auto& registry = EntityRegistry();
	const auto entity = registry.Create();
	registry.Assign<Flock>(entity, Flock {.centre = centre, .domainRadius = domainRadius, .flockDistance = flockDistance});
	return entity;
}

entt::entity AnimalSystem::CreateSpellAnimal(AnimalInfo type, glm::vec2 position, float heightAboveLand, uint16_t angle,
                                             PlayerNames owner, entt::entity flockEntity, entt::entity spell)
{
	auto& registry = EntityRegistry();
	const glm::vec3 point(position.x, Ground(position) + heightAboveLand, position.y);
	const auto& info = InfoOf(type);
	// Born at a random age (a spell wolf always grown), the day it was born a random part of its starting age back, at
	// its size for its age
	const auto age = RandomWhole(k_BirthAgeRange) + k_BirthAgeMin;
	(void)RandomWhole(info.startAge / 2);
	const auto bornAt = type == AnimalInfo::SpellWolf ? info.grownUpAge + 1 : age;
	const float scale = animals::BirthScale(bornAt, info.grownUpAge, info.ageToScale.values, Random);
	const auto entity =
	    ecs::archetypes::AnimalArchetype::Create(type, point, gutils::ConvertGameAngleTo3D(angle), scale, owner);
	auto& animal = registry.Get<Animal>(entity);
	animal.flock = flockEntity;
	animal.height = heightAboveLand;
	animal.goalHeight = heightAboveLand;
	animal.move.position = Fixed(position);
	animal.move.goal = animal.move.position;
	animal.move.angle = angle;
	animal.move.speed = SpeedStateOf(info, 0);
	// A spell wolf is hungry from the start
	animal.hunger = static_cast<int32_t>(info.hunger);
	SyncWorld(animal);
	animal.previousPosition = animal.position;
	animal.previousHeading = animal.heading;
	registry.Assign<SpellAnimal>(entity, SpellAnimal {.spell = spell, .previous = point});
	// The miracle's doves and bats take the brightest of the land's light wherever they are, its wolves are white
	registry.Assign<AnimalPose>(
	    entity, AnimalPose {.light = type == AnimalInfo::SpellWolf ? AnimalLight::White : AnimalLight::BrightestLand});
	if (type == AnimalInfo::SpellWolf)
	{
		registry.Assign<SpellWolf>(entity);
	}
	if (auto* flockData = registry.TryGet<Flock>(flockEntity))
	{
		flockData->members.push_back(entity);
	}
	return entity;
}

entt::entity AnimalSystem::LeaderOf(entt::entity flockEntity) const
{
	const auto& registry = EntityRegistry();
	const auto* flockData = registry.Valid(flockEntity) ? registry.TryGet<const Flock>(flockEntity) : nullptr;
	return flockData == nullptr || flockData->members.empty() ? entt::null : flockData->members.front();
}

std::vector<entt::entity> AnimalSystem::MembersOf(entt::entity flockEntity) const
{
	const auto& registry = EntityRegistry();
	const auto* flockData = registry.Valid(flockEntity) ? registry.TryGet<const Flock>(flockEntity) : nullptr;
	return flockData != nullptr ? flockData->members : std::vector<entt::entity> {};
}

glm::vec2 AnimalSystem::GoalOf(entt::entity animal) const
{
	// Where it is heading now: a wolf hunting heads for its prey
	const auto* data = EntityRegistry().TryGet<const Animal>(animal);
	return data != nullptr ? Metres(data->move.goal) : glm::vec2(0.0f);
}

float AnimalSystem::GoalHeightOf(entt::entity animal) const
{
	const auto* data = EntityRegistry().TryGet<const Animal>(animal);
	return data != nullptr ? data->goalHeight : 0.0f;
}

void AnimalSystem::SetupMoveTo(Animal& animal, glm::vec2 goal, float goalHeight, AnimalState finalState)
{
	animal.goalHeight = goalHeight;
	Banked(animal, animals::SetUpMove(animal.move, Fixed(goal), TurnAngleOf(animal)));
	animal.finalState = finalState;
	// It takes its kind's move state, and that state's clip
	SetTopState(animal, AnimalState::MoveToPos);
	SyncWorld(animal);
}

void AnimalSystem::Banked(Animal& animal, const animals::Turn& turn)
{
	const bool wolf = animal.type == AnimalInfo::SpellWolf;
	const bool spellBird = animal.type == AnimalInfo::SpellDove || animal.type == AnimalInfo::SpellBat;
	if (!wolf && !spellBird)
	{
		return;
	}
	const float timeToBank = wolf ? k_WolfTimeToBank : k_SpellBirdTimeToBank;
	if (turn.direction == 0)
	{
		animal.bank.SetTarget(0.0f, 0.0f, timeToBank);
		return;
	}
	float tilt = k_SpellBirdBankAngle;
	if (wolf)
	{
		tilt =
		    std::min(std::abs(static_cast<float>(animal.move.speed) * static_cast<float>(turn.step)) * k_WolfBankPerSpeedTurn,
		             k_WolfMostBank);
	}
	animal.bank.SetTarget(turn.direction < 0 ? -tilt : tilt, 0.0f, timeToBank);
}

bool AnimalSystem::MoveTo3D(Animal& animal)
{
	const auto before = Metres(animal.move.position);
	const float groundBefore = Ground(before);
	const float groundAtGoal = Ground(Metres(animal.move.goal));
	const auto result = animals::StepMove(animal.move, TurnAngleOf(animal));
	if (result.turned)
	{
		Banked(animal, result.turn);
	}
	animal.height = animals::FlyingHeight(groundBefore, animal.height, Ground(Metres(animal.move.position)), groundAtGoal,
	                                      animal.goalHeight, InfoOf(animal.type).altitudeMovementChange);
	SyncWorld(animal);
	return result.arrived;
}

glm::vec2 AnimalSystem::RandomPos(const Animal& animal, glm::vec2 centre, float inner, float outer) const
{
	const auto here = Metres(animal.move.position);
	const bool overLandOnly = animal.type == AnimalInfo::SpellDove || animal.type == AnimalInfo::SpellBat;
	const auto hasLand = [](glm::vec2 point) {
		return Locator::terrainSystem::has_value() &&
		       Locator::terrainSystem::value().FindCell(glm::u16vec2(map_coords::CellOf(point))) != nullptr;
	};
	// A point it may go to: on the map, one it can reach without circling, and for the doves and bats, over the land
	// all the way. (The game also turns down a point the map's walls block; openblack's map has none of them.)
	const auto valid = [&](glm::vec2 point) {
		return OnMap(point) &&
		       animals::OutsideTurningCircles(here, animal.move.angle, animal.move.speed, TurnAngleOf(animal), point) &&
		       (!overLandOnly || animals::OverLandAllTheWay(here, point, [&hasLand](glm::ivec2 cell) {
			       return hasLand(glm::vec2(cell) * 10.0f + 5.0f);
		       }));
	};
	for (int attempt = 0; attempt < k_RandomPosTries; ++attempt)
	{
		// The angle then the distance, each a random number of the game's
		const float angle = Random(k_TwoPi);
		const float distance = inner + Random(outer - inner);
		auto point = Metres(Fixed(centre + glm::vec2(std::cos(angle), std::sin(angle)) * distance));
		// Not there, then cells spiralling out from it
		map_coords::Spiral spiral;
		for (int left = k_RandomPosCells; left > 0; --left)
		{
			if (valid(point))
			{
				return point;
			}
			const auto& step = spiral.Next();
			point += glm::vec2(static_cast<float>(step.x), static_cast<float>(step.z)) * 10.0f;
		}
	}
	// None: the middle if it can reach it, else where it is
	return animals::OutsideTurningCircles(here, animal.move.angle, animal.move.speed, TurnAngleOf(animal), centre) ? centre
	                                                                                                               : here;
}

void AnimalSystem::ProcessTurn()
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	++_turn;
	auto& registry = EntityRegistry();
	std::vector<entt::entity> animals;
	registry.Each<const Animal>([&animals](entt::entity entity, const Animal&) { animals.push_back(entity); });
	for (const auto entity : animals)
	{
		if (!registry.Valid(entity))
		{
			continue;
		}
		// Carried off by a tornado, it is the tornado's until it lets go; held in a hand or flying, the hand's or the
		// physics'
		if (registry.AnyOf<CarriedByTornado, InHand, InPhysics>(entity))
		{
			continue;
		}
		auto& animal = registry.Get<Animal>(entity);
		animal.previousPosition = animal.position;
		animal.previousHeading = animal.heading;
		++animal.turnsInState;
		if (animal.state == AnimalState::Downed)
		{
			continue;
		}
		if (animal.state == AnimalState::Dying || animal.state == AnimalState::Dead)
		{
			ProcessDeath(entity, animal);
			continue;
		}
		// Hungrier each turn, up to its kind's hunger
		const auto& info = InfoOf(animal.type);
		if (info.hunger != 0 && animal.hunger < static_cast<int32_t>(info.hunger))
		{
			++animal.hunger;
		}
		// Fleeing what it reacts to comes before its own ways
		if (Flee(entity, animal))
		{
			continue;
		}
		if (IsBird(animal.type))
		{
			Bird(entity, animal);
		}
		else if (registry.AllOf<SpellWolf>(entity))
		{
			Wolf(entity, animal);
		}
	}
	ProcessEaten();
	ProcessFading();
	registry.SetDirty();
}

// The birds

void AnimalSystem::SendBird(entt::entity bird, glm::vec2 goal, float height, bool leader)
{
	auto& registry = EntityRegistry();
	auto* animal = registry.TryGet<Animal>(bird);
	if (animal == nullptr)
	{
		return;
	}
	// Off to its goal by its special move, then to choose its next leg
	SetupMoveTo(*animal, goal, height, AnimalState::StartWander);
	SetState(*animal, AnimalState::SpecialMoveToPos);
	// Once the leader is off, the others keep a formation behind it, and each special move ends in deciding again
	if (auto* flockData = leader ? registry.TryGet<Flock>(animal->flock) : nullptr)
	{
		flockData->followState = AnimalState::FollowFlock;
		flockData->followMode = k_FollowInFormation;
		flockData->afterMove = AnimalState::DecideWhatToDo;
	}
}

void AnimalSystem::Bird(entt::entity entity, Animal& animal)
{
	switch (animal.state)
	{
	case AnimalState::SpecialMoveToPos:
		SpecialMoveToPos(entity, animal);
		break;
	case AnimalState::DecideWhatToDo:
		DecideWhatToDo(entity, animal);
		break;
	case AnimalState::StartWander:
		StartWander(entity, animal);
		break;
	case AnimalState::FollowFlock:
		FollowFlock(entity, animal);
		break;
	case AnimalState::MoveToPos:
		// Its kind's move: there, it takes its final state
		if (MoveTo3D(animal))
		{
			SetTopState(animal, animal.finalState);
		}
		break;
	default:
		break;
	}
}

void AnimalSystem::SpecialMoveToPos(entt::entity /*entity*/, Animal& animal)
{
	const auto* flockData = EntityRegistry().TryGet<const Flock>(animal.flock);
	if (MoveTo3D(animal))
	{
		// There, the flock's state after a special move takes over its final state
		SetTopState(animal, animal.finalState);
		if (flockData != nullptr)
		{
			SetState(animal, flockData->afterMove);
		}
		return;
	}
	// Close to its goal it takes its usual speed
	if (animal.move.stage == animals::MoveStage::StepThrough &&
	    glm::distance(Metres(animal.move.position), Metres(animal.move.goal)) <= k_SpecialMoveSlowing)
	{
		animal.move.speed = SpeedStateOf(InfoOf(animal.type), 0);
	}
}

void AnimalSystem::DecideWhatToDo(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto* flockData = registry.TryGet<Flock>(animal.flock);
	if (flockData == nullptr)
	{
		return;
	}
	const auto leaderEntity = LeaderOf(animal.flock);
	if (leaderEntity == entity)
	{
		// The leader's next leg, at once
		SetTopState(animal, AnimalState::StartWander);
		StartWander(entity, animal);
		return;
	}
	if (leaderEntity == entt::null)
	{
		return;
	}
	// A follower picks a point near the leader, at its height, and then follows the flock
	const auto& leader = registry.Get<const Animal>(leaderEntity);
	const auto point = RandomPos(animal, Metres(leader.move.position), 0.0f, flockData->flockDistance);
	SetupMoveTo(animal, point, leader.height, AnimalState::DecideWhatToDo);
	animal.move.speed = SpeedStateOf(InfoOf(animal.type), 0);
	SetState(animal, flockData->followState);
}

void AnimalSystem::StartWander(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto* flockData = registry.TryGet<Flock>(animal.flock);
	if (flockData == nullptr)
	{
		return;
	}
	const auto& info = InfoOf(animal.type);
	animal.move.speed = SpeedStateOf(info, 0);
	// A point about where the flock was made, between its kind's inner radius and the flock's reach
	const auto point =
	    RandomPos(animal, flockData->centre, static_cast<float>(info.domainInnerRadius), flockData->domainRadius);
	// Higher or lower than its last goal by up to the variance; outside its kind's band about its normal height, back to
	// the normal height
	const float height = (animal.goalHeight + info.altitudeVariance) - Random(info.altitudeVariance + info.altitudeVariance);
	const float base = info.altitudeNormal;
	const float goalHeight = base + info.altitudeMin < height && height < base + info.altitudeMax ? height : base;
	SetupMoveTo(animal, point, goalHeight, AnimalState::DecideWhatToDo);
	SetState(animal, AnimalState::SpecialMoveToPos);
	flockData->followState = AnimalState::FollowFlock;
	flockData->followMode = k_FollowInFormation;
	flockData->afterMove = AnimalState::DecideWhatToDo;
	SpecialMoveToPos(entity, animal);
}

void AnimalSystem::FollowFlock(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto* flockData = registry.TryGet<Flock>(animal.flock);
	const auto leaderEntity = LeaderOf(animal.flock);
	if (flockData == nullptr || leaderEntity == entity || animal.state != flockData->followState)
	{
		DecideWhatToDo(entity, animal);
		return;
	}
	if (flockData->followMode != k_FollowInFormation || !MoveTo3D(animal))
	{
		return;
	}
	// There: on to its place in the formation, by its place in the flock, at the leader's height
	const auto found = std::ranges::find(flockData->members, entity);
	const int place = static_cast<int>(found - flockData->members.begin()) + 1;
	const auto& leader = registry.Get<const Animal>(leaderEntity);
	const auto goal =
	    animals::FormationGoal(Metres(leader.move.position), Metres(animal.move.position), animals::FormationSlotOf(place));
	SetupMoveTo(animal, goal, leader.height, AnimalState::DecideWhatToDo);
}

// The wolves

void AnimalSystem::SendWolf(entt::entity wolf, glm::vec2 start, glm::vec2 destination, float halfWidth)
{
	auto& registry = EntityRegistry();
	auto* run = registry.TryGet<SpellWolf>(wolf);
	auto* animal = registry.TryGet<Animal>(wolf);
	if (run == nullptr || animal == nullptr)
	{
		return;
	}
	run->finalDestination = destination;
	run->corridor = flock_rules::MakeCorridor(start, destination, halfWidth);
	RunToFinalDestination(wolf, *animal);
}

void AnimalSystem::RunToFinalDestination(entt::entity wolf, Animal& animal)
{
	const auto& run = EntityRegistry().Get<const SpellWolf>(wolf);
	const float scale = EntityRegistry().Get<const Transform>(wolf).scale.x;
	// Its kind's run times its scale, a tenth faster, to a whole speed state: the only speed a spell wolf takes
	const auto state = static_cast<double>(scale) * static_cast<double>(SpeedStateOf(InfoOf(animal.type), k_WolfRunSpeed)) *
	                   static_cast<double>(flock_rules::k_WolfRunFactor);
	animal.move.speed = static_cast<uint16_t>(std::clamp(static_cast<int32_t>(state), 0, 0xFFFF));
	SetupMoveTo(animal, run.finalDestination, 0.0f, AnimalState::SetDying);
}

void AnimalSystem::Wolf(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(entity);
	// What it was eating is gone: it decides again
	if (run.food != entt::null && !registry.Valid(run.food))
	{
		run.food = entt::null;
		run.eatCount = 0;
		SetTopState(animal, AnimalState::DecideWhatToDo);
		return;
	}
	switch (animal.state)
	{
	case AnimalState::MoveToPos:
		WolfMoveToPos(entity, animal);
		break;
	case AnimalState::SetDying:
		StartFading(entity);
		break;
	case AnimalState::DecideWhatToDo:
	case AnimalState::Wander:
		RunToFinalDestination(entity, animal);
		break;
	case AnimalState::StartWander:
		WolfStartWander(entity, animal);
		break;
	case AnimalState::Chase:
		Chase(entity, animal);
		break;
	case AnimalState::Pounce:
		Pounce(entity, animal);
		break;
	case AnimalState::StartToEat:
		// So many mouthfuls, once its settling down has played
		run.eatCount = static_cast<int>(RandomWhole(k_MouthfulsRange) + k_MouthfulsMin);
		WaitForClip(animal, AnimalState::Eat);
		break;
	case AnimalState::WaitForClip:
		if (animal.turnsInState * k_TurnMilliseconds >= PlayTimeOf(animal.animation))
		{
			SetTopState(animal, animal.afterClip);
		}
		break;
	case AnimalState::Eat:
		Eat(entity, animal);
		break;
	default:
		break;
	}
}

void AnimalSystem::WaitForClip(Animal& animal, AnimalState next)
{
	animal.afterClip = next;
	SetState(animal, AnimalState::WaitForClip);
}

void AnimalSystem::WolfMoveToPos(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	const auto& run = registry.Get<const SpellWolf>(entity);
	if (MoveTo3D(animal))
	{
		SetTopState(animal, animal.finalState);
	}
	// Hungry, it hunts what crosses its strip of land
	if (animal.state == AnimalState::MoveToPos && animal.hunger >= static_cast<int32_t>(InfoOf(animal.type).hunger))
	{
		ReactToFoodNeeds(entity, animal);
	}
	// Near where it was sent, it fades away
	if (flock_rules::WolfArrived(Metres(animal.move.position), run.finalDestination))
	{
		StartFading(entity);
	}
}

void AnimalSystem::ReactToFoodNeeds(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(entity);
	const auto& info = InfoOf(animal.type);
	// The prey it found last turn, still there to hunt, is chased; one brought down since is eaten. (The game also
	// passes over prey it cannot reach, such as a villager indoors; openblack's villagers are never out of reach.)
	if (run.prey != entt::null && registry.Valid(run.prey))
	{
		const auto& transform = registry.Get<const Transform>(run.prey);
		const auto* action = registry.TryGet<const LivingAction>(run.prey);
		const bool dying =
		    registry.AllOf<Villager>(run.prey) && action != nullptr &&
		    action->states[static_cast<size_t>(LivingAction::Index::Top)] == static_cast<uint8_t>(VillagerStates::Dying);
		const bool valid = !dying && transform.position.y - Ground(Xz(transform.position)) <= flock_rules::k_PreyMaxHeight &&
		                   glm::distance(Metres(animal.move.position), Xz(transform.position)) < info.huntingDistance;
		if (valid)
		{
			run.huntStart = _turn;
			SetupMoveToTarget(entity, animal, run.prey);
			return;
		}
	}
	run.prey = FindPrey(entity, animal);
	if (run.prey == entt::null && run.remembered.has_value())
	{
		// None now: it goes where it last found some
		SetupMoveTo(animal, *run.remembered, 0.0f, AnimalState::StartWander);
		run.remembered.reset();
	}
}

void AnimalSystem::WolfStartWander(entt::entity /*entity*/, Animal& animal)
{
	// It wanders off about its flock's leader, which a spell wolf takes up as its run next turn
	auto& registry = EntityRegistry();
	const auto* flockData = registry.TryGet<const Flock>(animal.flock);
	const auto leaderEntity = LeaderOf(animal.flock);
	SetTopState(animal, AnimalState::Wander);
	if (flockData != nullptr && leaderEntity != entt::null)
	{
		const auto centre = Metres(registry.Get<const Animal>(leaderEntity).move.position);
		const auto point =
		    RandomPos(animal, centre, static_cast<float>(InfoOf(animal.type).domainInnerRadius), flockData->domainRadius);
		Banked(animal, animals::SetUpMove(animal.move, Fixed(point), TurnAngleOf(animal)));
	}
}

void AnimalSystem::SetupMoveToTarget(entt::entity wolf, Animal& animal, entt::entity prey)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(wolf);
	run.prey = prey;
	// It makes for where it can reach the prey
	const auto preyPoint = Xz(registry.Get<const Transform>(prey).position);
	const auto goal = animals::WorkingPosition(preyPoint, RadiusOf(prey), Metres(animal.move.position), RadiusOf(wolf));
	Banked(animal, animals::SetUpMove(animal.move, Fixed(goal), TurnAngleOf(animal)));
	animal.goalHeight = 0.0f;
	// Prey already down is eaten; any other is chased, in whatever clip it has
	if (registry.AllOf<BeingEaten>(prey))
	{
		FinishPouncing(wolf, animal, prey);
		return;
	}
	SetState(animal, AnimalState::Chase);
}

void AnimalSystem::Abandon(entt::entity wolf, Animal& animal)
{
	auto& run = EntityRegistry().Get<SpellWolf>(wolf);
	run.prey = entt::null;
	run.remembered.reset();
	RunToFinalDestination(wolf, animal);
}

bool AnimalSystem::IsHuntingTargetValid(entt::entity wolf, const Animal& animal, entt::entity prey) const
{
	const auto& registry = EntityRegistry();
	const auto& run = registry.Get<const SpellWolf>(wolf);
	const auto* spellAnimal = registry.TryGet<const SpellAnimal>(wolf);
	if ((spellAnimal != nullptr && spellAnimal->Fading()) || !registry.Valid(prey) || registry.AllOf<BeingEaten>(prey))
	{
		return false;
	}
	if (const auto* villager = registry.TryGet<const Villager>(prey); villager != nullptr && villager->health == 0)
	{
		return false;
	}
	if (const auto* other = registry.TryGet<const Animal>(prey); other != nullptr && other->state == AnimalState::Downed)
	{
		return false;
	}
	const auto point = Xz(registry.Get<const Transform>(prey).position);
	return flock_rules::IsOnCorridor(run.corridor, point, Metres(animal.move.position)) &&
	       animals::OutsideTurningCircles(Metres(animal.move.position), animal.move.angle, animal.move.speed,
	                                      TurnAngleOf(animal), point);
}

void AnimalSystem::Chase(entt::entity wolf, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(wolf);
	if (run.prey == entt::null || !registry.Valid(run.prey))
	{
		Abandon(wolf, animal);
		return;
	}
	const auto& info = InfoOf(animal.type);
	// It heads straight for the prey, unless within a step of it already
	const auto preyPoint = Xz(registry.Get<const Transform>(run.prey).position);
	if (!(glm::distance(Metres(animal.move.position), preyPoint) < map_coords::ToMetres(animal.move.speed)))
	{
		animal.move.goal = Fixed(preyPoint);
		MoveTo3D(animal);
	}
	if (_turn - run.huntStart >= info.chaseTime || !IsHuntingTargetValid(wolf, animal, run.prey))
	{
		Abandon(wolf, animal);
		return;
	}
	// Close enough and facing it, it leaps
	const float distance = glm::distance(Metres(animal.move.position), preyPoint);
	const float scale = registry.Get<const Transform>(wolf).scale.x;
	const float reach = distance < info.attackDistance ? scale * StrideOf(AnimId::AWolfPounce) * 0.5f : k_PounceFromFar;
	if (distance <= reach)
	{
		const auto toPrey = gutils::GetAngleFromXZ(Metres(animal.move.position), preyPoint);
		if (gutils::GetAngleDifference(animal.move.angle, toPrey) <= k_PounceAngle)
		{
			SetTopState(animal, AnimalState::Pounce);
		}
		else
		{
			Abandon(wolf, animal);
		}
		return;
	}
	if (!(distance < info.huntingDistance))
	{
		Abandon(wolf, animal);
	}
}

void AnimalSystem::Pounce(entt::entity wolf, Animal& animal)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(wolf);
	const float scale = registry.Get<const Transform>(wolf).scale.x;
	// The ground leapt so far, and the whole leap: its clip's stride, scaled
	const float leapt = static_cast<float>(animal.move.speed) * k_SpeedStateToMetresPerSecond * k_TurnSeconds *
	                    static_cast<float>(animal.turnsInState);
	const float leap = scale * StrideOf(animal.animation);
	if (run.prey != entt::null && !registry.Valid(run.prey))
	{
		run.prey = entt::null;
		run.remembered.reset();
	}
	// The leap carries on along its last step
	animal.move.position += animal.move.step;
	SyncWorld(animal);
	if (run.prey == entt::null)
	{
		WolfStartWander(wolf, animal);
		return;
	}
	if (glm::distance(Metres(animal.move.position), Xz(registry.Get<const Transform>(run.prey).position)) <=
	    flock_rules::k_PounceReach)
	{
		BringDown(wolf, run.prey);
	}
	if (leap <= leapt)
	{
		if (registry.AllOf<BeingEaten>(run.prey))
		{
			FinishPouncing(wolf, animal, run.prey);
		}
		else
		{
			SetupMoveToTarget(wolf, animal, run.prey);
		}
	}
}

void AnimalSystem::IntoHand(entt::entity animal)
{
	auto* data = EntityRegistry().TryGet<Animal>(animal);
	if (data == nullptr)
	{
		return;
	}
	if (const auto clip = physics::living::ClipsOf(data->type).inHand)
	{
		data->animation = *clip;
		data->clipPlace = 0;
	}
}

void AnimalSystem::BringDown(entt::entity wolf, entt::entity prey)
{
	auto& registry = EntityRegistry();
	// Each turn the wolf is on it, it falls again for as long as its fall plays out, then is eaten over the turns
	int fallTurns = 0;
	if (auto* villager = registry.TryGet<Villager>(prey))
	{
		const float before = ecs::world_objects::LifeOf(prey);
		villager->health = static_cast<uint32_t>(std::lround(flock_rules::k_DownedLife * 100.0f));
		ecs::world_objects::CountInjury(prey, before, flock_rules::k_DownedLife);
		if (auto* wallHug = registry.TryGet<WallHug>(prey))
		{
			wallHug->goal = Xz(registry.Get<const Transform>(prey).position);
		}
		if (auto* action = registry.TryGet<LivingAction>(prey); action != nullptr && Locator::livingActionSystem::has_value())
		{
			Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top, VillagerStates::Downed,
			                                                      true);
		}
		fallTurns = flock_rules::TurnsToPlay(PlayTimeOf(k_VillagerAttacked), k_TurnMilliseconds);
	}
	else if (auto* other = registry.TryGet<Animal>(prey))
	{
		SetTopState(*other, AnimalState::Downed);
		fallTurns = flock_rules::TurnsToPlay(PlayTimeOf(other->animation), k_TurnMilliseconds);
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Animals: #{} brought down by wolf #{}", static_cast<uint32_t>(prey),
	                    static_cast<uint32_t>(wolf));
	registry.AssignOrReplace<BeingEaten>(prey, BeingEaten {.hunter = wolf,
	                                                       .turns = 0,
	                                                       .eatenFrom = k_FallStartsAfter + fallTurns,
	                                                       .left = flock_rules::k_BeingEatenTurns});
}

void AnimalSystem::FinishPouncing(entt::entity wolf, Animal& animal, entt::entity prey)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(wolf);
	animal.hunger = 0;
	run.prey = entt::null;
	run.remembered.reset();
	// It goes its scale short of the prey to eat it
	const auto& preyTransform = registry.Get<const Transform>(prey);
	const auto preyPoint = Xz(preyTransform.position);
	const glm::vec3 prey3(preyPoint.x, preyTransform.position.y - Ground(preyPoint), preyPoint.y);
	const auto wolfPoint = Metres(animal.move.position);
	const glm::vec3 wolf3(wolfPoint.x, animal.height, wolfPoint.y);
	const auto goal = flock_rules::EatingPosition(prey3, wolf3, registry.Get<const Transform>(wolf).scale.x);
	SetupMoveTo(animal, {goal.x, goal.z}, 0.0f, AnimalState::StartToEat);
	run.food = prey;
}

void AnimalSystem::Eat(entt::entity wolf, Animal& animal)
{
	auto& run = EntityRegistry().Get<SpellWolf>(wolf);
	if (--run.eatCount != 0)
	{
		// Another mouthful, for as long as its eating plays
		WaitForClip(animal, AnimalState::Eat);
		return;
	}
	// Fed, it decides again, which for a spell wolf is running on
	animal.hunger = 0;
	run.food = entt::null;
	SetTopState(animal, AnimalState::DecideWhatToDo);
}

entt::entity AnimalSystem::FindPrey(entt::entity wolf, const Animal& animal)
{
	auto& registry = EntityRegistry();
	auto& run = registry.Get<SpellWolf>(wolf);
	// What lives in each map cell
	std::map<std::pair<int32_t, int32_t>, std::vector<std::pair<entt::entity, flock_rules::PreyFacts>>> cells;
	const auto add = [&](entt::entity entity, const Transform& transform, flock_rules::PreyFacts facts) {
		const auto cell = map_coords::CellOf(Xz(transform.position));
		facts.heightAboveLand = transform.position.y - Ground(Xz(transform.position));
		cells[{cell.x, cell.y}].emplace_back(entity, facts);
	};
	registry.Each<const Villager, const Transform>([&](entt::entity entity, const Villager& villager, const Transform& t) {
		add(entity, t, {.isVillager = true, .helpless = villager.health == 0 || registry.AllOf<BeingEaten>(entity)});
	});
	registry.Each<const Animal, const Transform>([&](entt::entity entity, const Animal& other, const Transform& t) {
		if (entity != wolf)
		{
			add(entity, t,
			    {.isOtherAnimal = other.type != animal.type,
			     .hasMeat = InfoOf(other.type).foodValue > 0.0f,
			     .helpless = other.state == AnimalState::Downed || registry.AllOf<BeingEaten>(entity)});
		}
	});
	// So many cells spiralling out from its own, and the first prey met there is its prey
	auto cell = map_coords::CellOf(Metres(animal.move.position));
	map_coords::Spiral spiral;
	for (auto left = InfoOf(animal.type).farSightDistance; left > 0; --left)
	{
		if (const auto found = cells.find({cell.x, cell.y}); found != cells.end())
		{
			for (auto [entity, facts] : found->second)
			{
				facts.onCorridor = IsHuntingTargetValid(wolf, animal, entity);
				if (!flock_rules::IsPrey(facts))
				{
					continue;
				}
				// Once it remembers where it found prey, the game's mixed measure decides whether this one is near
				// enough to take instead
				const auto preyPoint = Xz(registry.Get<const Transform>(entity).position);
				if (run.remembered.has_value() &&
				    !flock_rules::CloserThanRemembered(animal.move.position, Fixed(preyPoint), Fixed(*run.remembered)))
				{
					continue;
				}
				run.remembered = preyPoint;
				return entity;
			}
		}
		const auto& step = spiral.Next();
		cell += glm::ivec2(step.x, step.z);
	}
	return entt::null;
}

void AnimalSystem::ProcessEaten()
{
	auto& registry = EntityRegistry();
	std::vector<entt::entity> eaten;
	registry.Each<BeingEaten>([&](entt::entity entity, BeingEaten& being) {
		++being.turns;
		// Its fall played out, it is eaten, a turn at a time
		if (being.turns == being.eatenFrom)
		{
			if (auto* action = registry.TryGet<LivingAction>(entity);
			    action != nullptr && Locator::livingActionSystem::has_value())
			{
				Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Top,
				                                                      VillagerStates::BeingEaten, true);
			}
		}
		else if (being.turns > being.eatenFrom && --being.left == 0)
		{
			eaten.push_back(entity);
		}
	});
	for (const auto entity : eaten)
	{
		registry.Remove<BeingEaten>(entity);
		if (registry.AllOf<Villager>(entity))
		{
			// Eaten, the villager dies as any villager dies: it falls, its town and the people about it hear of it, and
			// its body lies until it goes
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Animals: villager #{} eaten, dies", static_cast<uint32_t>(entity));
			villager_fire::DieByEffect(entity);
		}
		else
		{
			Remove(entity);
		}
	}
}

// Fading and going

void AnimalSystem::StartFading(entt::entity animal)
{
	// From full, at rest, to nothing over its fade's turns
	if (auto* spellAnimal = EntityRegistry().TryGet<SpellAnimal>(animal); spellAnimal != nullptr && !spellAnimal->Fading())
	{
		spellAnimal->fade = animals::Zoomer(flock_rules::k_FullAlpha);
		spellAnimal->fade->SetTarget(0.0f, 0.0f, static_cast<float>(flock_rules::k_FadeTurns) * k_TurnSeconds);
	}
}

void AnimalSystem::ProcessFading()
{
	auto& registry = EntityRegistry();
	std::vector<entt::entity> gone;
	registry.Each<SpellAnimal>([&gone](entt::entity entity, SpellAnimal& spellAnimal) {
		if (!spellAnimal.fade.has_value())
		{
			return;
		}
		// Gone once its alpha is nothing
		if (spellAnimal.fade->Step(k_TurnSeconds) == 0.0f)
		{
			gone.push_back(entity);
		}
	});
	for (const auto entity : gone)
	{
		Remove(entity);
	}
}

float AnimalSystem::RadiusOf(entt::entity animal) const
{
	return ::RadiusOf(animal);
}

glm::vec3 AnimalSystem::MovementOf(entt::entity animal) const
{
	const auto* data = EntityRegistry().TryGet<const Animal>(animal);
	if (data == nullptr)
	{
		return glm::vec3(0.0f);
	}
	const auto step = Metres(data->move.step);
	return {step.x, 0.0f, step.y};
}

void AnimalSystem::KillByEffect(entt::entity entity, glm::vec3 position)
{
	auto& registry = EntityRegistry();
	auto* animal = registry.TryGet<Animal>(entity);
	if (animal == nullptr)
	{
		return;
	}
	// Where it was put down, on the land
	animal->move.position = Fixed({position.x, position.z});
	animal->move.goal = animal->move.position;
	animal->height = 0.0f;
	SyncWorld(*animal);
	animal->previousPosition = animal->position;
	if (registry.AllOf<SpellAnimal>(entity))
	{
		StartFading(entity);
		return;
	}
	SetDying(entity);
}

void AnimalSystem::SetDying(entt::entity entity)
{
	auto& registry = EntityRegistry();
	auto* animal = registry.TryGet<Animal>(entity);
	// One killed in the air starts dying only once it has come down
	if (animal == nullptr || registry.AllOf<InPhysics>(entity))
	{
		return;
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Animals: #{} killed", static_cast<uint32_t>(entity));
	// It has no life left and falls dying; its body lies its time once dead. A bird falls out of the sky through the
	// physics while it dies. Killed again, its body only lies its full time afresh
	animal->deadTurns = animals::k_TurnsToDieOver;
	if (animal->state == AnimalState::Dying || animal->state == AnimalState::Dead)
	{
		return;
	}
	animal->life = 0.0f;
	animal->state = AnimalState::Dying;
	animal->turnsInState = 0;
	if (const auto clip = animals::DyingClip(animal->type))
	{
		animal->animation = *clip;
		animal->clipPlace = 0;
	}
}

void AnimalSystem::FallDying(entt::entity entity, const Animal& animal)
{
	if (!Locator::dynamicsSystem::has_value())
	{
		return;
	}
	// Forward at its speed across the land, tumbling about its side axis
	constexpr glm::vec3 k_DyingTumble {5.0f, 0.0f, 0.0f};
	const auto step = Metres(animal.move.step);
	const float speed = glm::length(step) * k_TurnsPerSecond;
	const glm::vec3 forward(std::cos(animal.heading), 0.0f, std::sin(animal.heading));
	Locator::dynamicsSystem::value().InitialisePhysics(entity, {.velocity = forward * speed, .spin = k_DyingTumble});
}

void AnimalSystem::ProcessDeath(entt::entity entity, Animal& animal)
{
	if (animal.state == AnimalState::Dying)
	{
		// A dying bird falls out of the sky with the speed it flew at, tumbling, until it comes down
		if (IsBird(animal.type))
		{
			FallDying(entity, animal);
			return;
		}
		// Its fall played out once, it lies dead, out of its flock
		if (animal.turnsInState * k_TurnMilliseconds < PlayTimeOf(animal.animation))
		{
			return;
		}
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Animals: #{} lies dead", static_cast<uint32_t>(entity));
		animal.state = AnimalState::Dead;
		animal.turnsInState = 0;
		if (const auto clip = animals::DeadClip(animal.type))
		{
			animal.animation = *clip;
			animal.clipPlace = 0;
		}
		LeaveFlock(entity, animal);
		return;
	}
	// Its time up, it goes. (The game lets out a puff of grey smoke as it goes; openblack doesn't draw that smoke yet.)
	if (animal.deadTurns-- == 0)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Animals: #{} dead body goes", static_cast<uint32_t>(entity));
		Remove(entity);
	}
}

void AnimalSystem::SetFlockCentre(entt::entity flock, glm::vec2 centre)
{
	if (auto* flockData = EntityRegistry().TryGet<Flock>(flock))
	{
		flockData->centre = centre;
	}
}

void AnimalSystem::SetScale(entt::entity animal, float scale)
{
	if (auto* transform = EntityRegistry().TryGet<Transform>(animal))
	{
		transform->scale = glm::vec3(scale);
	}
}

bool AnimalSystem::IsFrighteningToCreature(entt::entity animal) const
{
	const auto* data = EntityRegistry().TryGet<const Animal>(animal);
	// Bats, the evil flock's bats, vultures and lions frighten creatures
	return data != nullptr && (data->type == AnimalInfo::Bat || data->type == AnimalInfo::SpellBat ||
	                           data->type == AnimalInfo::Vulture || data->type == AnimalInfo::Lion);
}

bool AnimalSystem::IsAvailableForReaction(entt::entity entity) const
{
	const auto& registry = EntityRegistry();
	const auto* animal = registry.TryGet<const Animal>(entity);
	if (animal == nullptr || registry.AnyOf<CarriedByTornado, InHand, InPhysics>(entity))
	{
		return false;
	}
	// Judged by the state it is to end up in
	const auto state = animal->state == AnimalState::MoveToPos ? animal->finalState : animal->state;
	switch (state)
	{
	case AnimalState::Dying:
	case AnimalState::Dead:
	case AnimalState::Downed:
	case AnimalState::WaitForClip:
		return false;
	default:
		return true;
	}
}

bool AnimalSystem::SetupReactToFlyingObject(entt::entity entity, entt::entity object, float speed)
{
	auto& registry = EntityRegistry();
	auto* animal = registry.TryGet<Animal>(entity);
	const auto* at = registry.Valid(object) ? registry.TryGet<const Transform>(object) : nullptr;
	if (animal == nullptr || at == nullptr)
	{
		return false;
	}
	// Measured across the map, against how far the thing flies in two seconds
	const auto& here = registry.Get<const Transform>(entity).position;
	if (!living::AnimalFleesFlyingObject(living::MapDistance(here, at->position), speed))
	{
		return false;
	}
	animal->fleeing = object;
	SetState(*animal, AnimalState::FleeingFromObject);
	return true;
}

void AnimalSystem::StopReaction(entt::entity entity)
{
	auto* animal = EntityRegistry().TryGet<Animal>(entity);
	if (animal == nullptr)
	{
		return;
	}
	animal->fleeing = entt::null;
	if (animal->state == AnimalState::FleeingFromObject || animal->state == AnimalState::FleeingAndLookingAtObject ||
	    (animal->state == AnimalState::MoveToPos && animal->finalState == AnimalState::FleeingAndLookingAtObject))
	{
		SetState(*animal, AnimalState::DecideWhatToDo);
	}
}

bool AnimalSystem::Flee(entt::entity entity, Animal& animal)
{
	const bool running = animal.state == AnimalState::MoveToPos && animal.finalState == AnimalState::FleeingAndLookingAtObject;
	if (animal.state != AnimalState::FleeingFromObject && animal.state != AnimalState::FleeingAndLookingAtObject && !running)
	{
		return false;
	}
	auto& registry = EntityRegistry();
	const auto* at = registry.Valid(animal.fleeing) ? registry.TryGet<const Transform>(animal.fleeing) : nullptr;
	const auto& info = Locator::infoConstants::value().reaction.at(static_cast<size_t>(Reaction::ReactToFlyingObject));
	// With nothing left to flee it decides again
	if (at == nullptr)
	{
		StopReaction(entity);
		return true;
	}
	// Running to where it flees, it gets there first
	if (running)
	{
		if (MoveTo3D(animal))
		{
			SetState(animal, animal.finalState);
		}
		return true;
	}
	auto& transform = registry.Get<Transform>(entity);
	const auto& here = transform.position;
	const float distance = living::MapDistance(here, at->position);
	// Watching, it gives up once the thing is beyond the reaction's furthest
	if (animal.state == AnimalState::FleeingAndLookingAtObject)
	{
		if (!(distance <= info.maxDistanceToRunAwayFromObject))
		{
			StopReaction(entity);
		}
		return true;
	}
	const auto* entry = Locator::dynamicsSystem::has_value() ? Locator::dynamicsSystem::value().Find(animal.fleeing) : nullptr;
	const glm::vec3 velocity = entry != nullptr && entry->body != nullptr ? entry->body->velocity : glm::vec3(0.0f);
	const bool coming = magic::ComingTowards(here, at->position, velocity);
	switch (living::FleeFromObject(distance, info.minDistanceToRunAwayFromObject, info.maxDistanceToRunAwayFromObject, coming))
	{
	case living::FleeStep::GiveUp:
		StopReaction(entity);
		break;
	case living::FleeStep::Watch:
		SetState(animal, AnimalState::FleeingAndLookingAtObject);
		break;
	case living::FleeStep::Run:
	{
		// A step away across the land, across the thing's way when it moves, at its fleeing speed
		auto to = magic::FleePointFromStill(here, at->position);
		if (glm::length(velocity) > 0.0f)
		{
			const float x = Random(magic::k_FleeJitter);
			const float z = Random(magic::k_FleeJitter);
			to = magic::FleePointFromMoving(here, at->position, velocity, x, z);
		}
		if (OnMap(Xz(to)))
		{
			const auto clip = animal.animation;
			const auto place = animal.clipPlace;
			// The speed of its kind the state table gives fleeing
			const auto& row = Locator::infoConstants::value().villagerStateTable.at(
			    static_cast<size_t>(VillagerStates::FleeingFromObjectReaction));
			animal.move.speed = SpeedStateOf(InfoOf(animal.type), std::min<size_t>(row.speedIndex, 5));
			SetupMoveTo(animal, Xz(to), 0.0f, AnimalState::FleeingAndLookingAtObject);
			// An animal has no clip of its own for moving: it keeps the one it had
			animal.animation = clip;
			animal.clipPlace = place;
		}
		break;
	}
	}
	return true;
}

bool AnimalSystem::CanPlayerPickUp(entt::entity animal) const
{
	const auto* data = EntityRegistry().TryGet<const Animal>(animal);
	return data != nullptr && InfoOf(data->type).playerCanPickUp != 0;
}

void AnimalSystem::Remove(entt::entity animal)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(animal))
	{
		return;
	}
	if (const auto* data = registry.TryGet<const Animal>(animal))
	{
		if (auto* flockData = registry.Valid(data->flock) ? registry.TryGet<Flock>(data->flock) : nullptr)
		{
			std::erase(flockData->members, animal);
			if (flockData->members.empty())
			{
				registry.Destroy(data->flock);
			}
		}
	}
	registry.Destroy(animal);
}

void AnimalSystem::LeaveFlock(entt::entity entity, Animal& animal)
{
	auto& registry = EntityRegistry();
	if (auto* flockData = registry.Valid(animal.flock) ? registry.TryGet<Flock>(animal.flock) : nullptr)
	{
		std::erase(flockData->members, entity);
		if (flockData->members.empty())
		{
			registry.Destroy(animal.flock);
		}
	}
	animal.flock = entt::null;
}

// Drawing

void AnimalSystem::Update(uint32_t turn, float turnFraction)
{
	auto& registry = EntityRegistry();
	const float t = std::clamp(turnFraction, 0.0f, 1.0f);
	// The milliseconds of the game's clock since the last frame, none when the clock went back
	const auto now = animals::DrawTime(turn, t);
	const uint32_t elapsed = now >= _drawTime ? now - _drawTime : 0;
	_drawTime = now;
	const float elapsedSeconds = static_cast<float>(elapsed) * 0.001f;
	const auto& animations = Locator::resources::value().GetAnimations();
	auto& meshes = Locator::resources::value().GetMeshes();
	registry.Each<Animal, Transform, const Mesh>([&](entt::entity entity, Animal& animal, Transform& transform,
	                                                 const Mesh& mesh) {
		// A tornado carrying it places it, as does a hand holding it or the physics moving it; it still plays its clip
		if (!registry.AnyOf<CarriedByTornado, InHand, InPhysics>(entity))
		{
			// Drawn between its last two turns, tilted as far as its bank has glided
			transform.position = animal.previousPosition + ((animal.position - animal.previousPosition) * t);
			const float heading =
			    animal.previousHeading + (std::remainder(animal.heading - animal.previousHeading, k_TwoPi) * t);
			transform.rotation = animals::Orientation(heading, animal.bank.Step(elapsedSeconds));
		}
		auto* pose = registry.TryGet<AnimalPose>(entity);
		// The clips are kept by the hash of their number in the animation pack
		const auto clipId = resources::HashIdentifier(static_cast<uint32_t>(animal.animation));
		if (pose == nullptr || animal.animation == AnimId::Invalid || !animations.Contains(clipId) || !meshes.Contains(mesh.id))
		{
			return;
		}
		// Each animal plays its own clip: by the ground it covers while it moves, else by the clock
		const auto clip = animations.Handle(clipId);
		const animals::ClipTiming timing {.playTime = clip->GetPlayTime(),
		                                  .frameCount = clip->GetFrames().size(),
		                                  .looping = clip->IsLooping(),
		                                  .playedByTime = clip->IsPlayedByTime(),
		                                  .stride = clip->GetStride()};
		const bool moving = animal.move.stage != animals::MoveStage::AtGoal &&
		                    (animal.state == AnimalState::MoveToPos || animal.state == AnimalState::SpecialMoveToPos ||
		                     animal.state == AnimalState::FollowFlock || animal.state == AnimalState::Chase);
		const auto played = moving && !timing.playedByTime
		                        ? animals::MovingPlay(timing, animal.move.speed, elapsed, transform.scale.x)
		                        : static_cast<int32_t>(elapsed);
		animal.clipPlace = animals::AdvanceClip(timing, animal.clipPlace, played);

		// A wolf fades out of sight as it goes; the doves and bats stay whole until they go
		pose->alpha = 255;
		if (const auto* spellAnimal = registry.TryGet<const SpellAnimal>(entity);
		    pose->light == AnimalLight::White && spellAnimal != nullptr)
		{
			pose->alpha = static_cast<uint8_t>(std::clamp(std::nearbyint(spellAnimal->Alpha()), 0.0f, 255.0f));
		}

		// The pose between the two keyframes around its place, each bone then placed by its parent
		const auto model = meshes.Handle(mesh.id);
		const auto& frames = clip->GetFrames();
		const auto& parents = model->GetBoneParents();
		const auto span = animals::SpanAt(timing, animal.clipPlace);
		if (!model->IsBoned() || frames.empty() || frames[span.from].bones.size() != parents.size() ||
		    frames[span.to].bones.size() != parents.size())
		{
			pose->bones.clear();
			return;
		}
		auto& bones = pose->bones;
		bones.resize(parents.size());
		const auto& from = frames[span.from].bones;
		const auto& to = frames[span.to].bones;
		for (size_t i = 0; i < bones.size(); ++i)
		{
			bones[i] = from[i] + ((to[i] - from[i]) * span.t);
			if (parents[i] != std::numeric_limits<uint32_t>::max())
			{
				bones[i] = bones[parents[i]] * bones[i];
			}
		}
	});
	registry.SetDirty();
}

void AnimalSystem::Reset()
{
	_drawTime = 0;
	_turn = 0;
}
