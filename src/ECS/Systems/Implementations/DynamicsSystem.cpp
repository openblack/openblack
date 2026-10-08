/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "DynamicsSystem.h"

#include <chrono>
#include <vector>

#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "Creature/CreatureRig.h"
#include "ECS/Archetypes/DeadTreeArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/AudioEmitter.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/PhysicsGround.h"
#include "ECS/Registry.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleEffect.h"
#include "Physics/BodyShapes.h"
#include "Physics/ObjectRules.h"
#include "Physics/PairRules.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace turn = openblack::physics::turn;
using physics_classes::BodyKind;

namespace
{
/// The puffs of dust are drawn from this sheet, eight frames a row
constexpr entt::hashed_string k_DustSheet = entt::hashed_string("raw/blobs");
constexpr entt::hashed_string k_DustSheetAlpha = entt::hashed_string("raw/blobsa");
/// The bank the collision sounds are picked from
constexpr std::string_view k_CollisionBank = "editor.sad";
/// The list grows by this many slots at a time
constexpr size_t k_SlotsPerGrowth = 16;
/// A raise smaller than this is not one
constexpr float k_LeastRaise = 0.001f;
/// A dropped body is raised out of what is under it, looking down through it and up through what is under it
constexpr glm::vec3 k_Down(0.0f, -1.0f, 0.0f);
constexpr glm::vec3 k_Up(0.0f, 1.0f, 0.0f);

Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

/// An object the physics may still work on: one that exists
bool IsAvailable(entt::entity object)
{
	return object != entt::null && Entities().Valid(object);
}

uint32_t Id(entt::entity object)
{
	return object == entt::null ? 0u : entt::to_integral(object) + 1u;
}

/// The dust's sprites
particles::Creator MakeDustCreator()
{
	particles::Creator dust;
	dust.kind = particles::Creator::Kind::Sprite;
	dust.className = "PhysicsDust";
	dust.texture = "blobs";
	dust.spritesPerRow = 8;
	dust.numFrames = 64;
	dust.additive = false;
	return dust;
}

/// Upright axes with the same heading across the ground as the given ones
glm::mat3 UprightAxes(const glm::mat3& axes)
{
	return physics::objects::HeadingOnly(axes);
}

/// The info weight of an object's kind
float InfoWeight(entt::entity object)
{
	if (!Locator::infoConstants::has_value())
	{
		return 0.0f;
	}
	const auto& info = Locator::infoConstants::value();
	const auto& registry = Entities();
	const auto row = [](const auto& rows, auto index) -> float {
		const auto i = static_cast<size_t>(index);
		return i < rows.size() ? rows[i].weight : 0.0f;
	};
	if (const auto* animal = registry.TryGet<const Animal>(object))
	{
		return row(info.animal, animal->type);
	}
	if (const auto* dead = registry.TryGet<const DeadTree>(object))
	{
		// A dead tree weighs what its tree did
		return row(info.tree, dead->type);
	}
	if (registry.AllOf<OneOffSpellSeed>(object))
	{
		return row(info.mobileObject, MobileObjectInfo::OneOffSpellSeed);
	}
	if (registry.AllOf<MagicShield>(object))
	{
		// The physical shield's row
		return info.mapShield[1].weight;
	}
	const auto* found = world_objects::InfoOf(object);
	return found != nullptr ? found->weight : 0.0f;
}

/// The parts of a model the physics may build a body from: its triangles and whether they are its collision shape or of
/// its nearest detail
struct ModelParts
{
	std::vector<std::vector<uint32_t>> indices;
	std::vector<physics::shapes::ModelPart> parts;
};
ModelParts PartsOf(const graphics::L3DMesh& mesh)
{
	ModelParts model;
	const auto& subMeshes = mesh.GetSubMeshes();
	model.indices.reserve(subMeshes.size());
	for (const auto& subMesh : subMeshes)
	{
		// A boned model's vertices count as the file holds them, unposed, as the game takes them
		const auto& geometry = subMesh->GetBodyGeometry();
		model.indices.emplace_back(geometry.indices.begin(), geometry.indices.end());
		model.parts.push_back({.positions = geometry.positions,
		                       .indices = model.indices.back(),
		                       .isPhysics = subMesh->IsPhysics(),
		                       .nearestDetail = (subMesh->GetFlags().lodMask & 1u) != 0});
	}
	return model;
}
} // namespace

DynamicsSystem::DynamicsSystem()
    : _hooks(std::make_unique<PhysicsClassHooks>())
    , _dustCreator(MakeDustCreator())
{
}

DynamicsSystem::~DynamicsSystem() = default;

// The objects' physics

void DynamicsSystem::ResetSimulation()
{
	_entries.clear();
	_capacity = 0;
	_inTurnUpdate = false;
	_soundPairs.Clear();
	_dust.clear();
	_followingSounds.clear();
	_creatureBoxes.clear();
	_hitObject = entt::null;
	_objectWhichHit = entt::null;
	_ground.reset();
	_groundLand = nullptr;
}

void DynamicsSystem::SetClassHooks(std::unique_ptr<PhysicsClassHooks> hooks)
{
	_hooks = hooks != nullptr ? std::move(hooks) : std::make_unique<PhysicsClassHooks>();
}

PhysicsClassHooks& DynamicsSystem::Hooks()
{
	return *_hooks;
}

const PhysicsGround* DynamicsSystem::Land()
{
	if (!Locator::terrainSystem::has_value())
	{
		return nullptr;
	}
	const auto& land = Locator::terrainSystem::value();
	if (_groundLand != &land || _ground == nullptr)
	{
		_ground = std::make_unique<PhysicsGround>(land);
		_groundLand = &land;
	}
	return _ground.get();
}

const physics::Ground* DynamicsSystem::GetGround() const
{
	return _ground.get();
}

physics_classes::ClassFacts DynamicsSystem::FactsOf(entt::entity object) const
{
	if (!Locator::infoConstants::has_value() || !IsAvailable(object))
	{
		return {};
	}
	const auto& registry = Entities();
	const auto* progress = registry.TryGet<const BuildProgress>(object);
	physics_classes::ClassInputs inputs {.life = world_objects::LifeOf(object),
	                                     .percentBuilt = progress != nullptr ? progress->built : 1.0f,
	                                     .immovable = registry.AllOf<Immovable>(object)};
	if (registry.AllOf<Villager>(object))
	{
		// Not reachable at home, in a hand or hiding in a building
		const auto* action = registry.TryGet<const LivingAction>(object);
		const bool hiding = action != nullptr && action->states[static_cast<size_t>(LivingAction::Index::Top)] ==
		                                             static_cast<uint8_t>(VillagerStates::GoAndHideInNearbyBuilding);
		inputs.villagerReachable = !registry.AllOf<AtHome>(object) && !registry.AllOf<InHand>(object) && !hiding;
	}
	if (registry.AllOf<Pot>(object))
	{
		registry.Each<const StoragePit>([object, &inputs](const StoragePit& pit) {
			if (pit.foodPile == object || std::ranges::find(pit.woodPiles, object) != pit.woodPiles.end())
			{
				inputs.partOfStoragePit = true;
			}
		});
	}
	auto facts = physics_classes::Classify(registry, object, Locator::infoConstants::value(), inputs);
	// Every model moved by bones is drawn from its body turned a quarter about its up axis
	if (const auto* mesh = registry.TryGet<const Mesh>(object); mesh != nullptr && Locator::resources::has_value())
	{
		const auto& meshes = Locator::resources::value().GetMeshes();
		if (meshes.Contains(mesh->id) && meshes.Handle(mesh->id)->IsBoned())
		{
			facts.animated = true;
		}
	}
	return facts;
}

std::unique_ptr<physics::Body> DynamicsSystem::MakeBody(entt::entity object, const physics_classes::ClassFacts& facts)
{
	auto& registry = Entities();
	if (facts.body == BodyKind::None || !Locator::resources::has_value())
	{
		return nullptr;
	}
	const auto& materials = Locator::resources::value().GetPhysicsMaterials();
	const auto material = materials.Contains(physics::k_MaterialsId.value())
	                          ? (*materials.Handle(physics::k_MaterialsId.value()))[facts.row]
	                          : physics::Material {};
	if (facts.body == BodyKind::Creature)
	{
		return MakeCreatureBody(object, material);
	}

	// The model the body is made from, its scale and where it stands
	const auto* transform = registry.TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return nullptr;
	}
	entt::id_type meshId = 0;
	float scale = transform->scale.x;
	glm::mat3 axes = transform->rotation;
	glm::vec3 origin = transform->position;
	if (facts.body == BodyKind::ShieldDome)
	{
		// The dome's solid shape stands where it was raised at its full size, unturned
		const auto& dome = registry.Get<const ShieldDome>(object);
		meshId = dome.mesh;
		scale = dome.hullScale;
		axes = glm::mat3(1.0f);
		origin = dome.hullOrigin;
	}
	else if (facts.body == BodyKind::CollisionModel)
	{
		if (!facts.collisionMesh.has_value())
		{
			return nullptr;
		}
		meshId = resources::HashIdentifier(static_cast<MeshId>(*facts.collisionMesh));
	}
	else if (const auto* meshComponent = registry.TryGet<const Mesh>(object))
	{
		meshId = meshComponent->id;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (meshId == 0 || !meshes.Contains(meshId))
	{
		return nullptr;
	}
	const auto& mesh = *meshes.Handle(meshId);
	const auto size = mesh.GetBoundingBox().Size();
	const float halfHeight = 0.5f * size.y;
	const float height = size.y * scale;
	const float radius = 0.5f * std::max(size.x, size.z) * scale;
	const float weight = physics_classes::Weight(InfoWeight(object), scale);
	const float mass = facts.fixedMass.value_or(facts.unclampedMass ? weight : physics_classes::BodyMass(weight));
	const physics::BodySetup setup {
	    .scale = scale, .halfHeight = halfHeight, .mass = mass, .material = material, .dynamic = facts.dynamic};

	physics::Shape shape;
	switch (facts.body)
	{
	case BodyKind::Tree:
		shape = physics::shapes::Tree(height, radius, scale, facts.rooted);
		break;
	case BodyKind::VillagerBox:
	case BodyKind::AnimalBox:
		shape = physics::shapes::LivingBody(facts.body == BodyKind::VillagerBox ? physics::shapes::Living::Villager
		                                                                        : physics::shapes::Living::Animal,
		                                    height, radius, scale);
		break;
	default:
	{
		const auto model = PartsOf(mesh);
		shape = physics::shapes::FromModel(model.parts, scale);
		break;
	}
	}
	if (shape.points.empty())
	{
		return nullptr;
	}
	// A model moved by bones has its body in the axes it is drawn from, before the quarter turn it is drawn with
	if (facts.animated)
	{
		axes = physics::QuarterTurnedBack(axes);
	}
	// A model that lies over the land's shape has its points lifted with it, the land under each point against the land
	// under its origin; its centre of mass stays where its unlifted points put it, and its reach takes the lift in
	if (facts.body == BodyKind::Model && registry.AllOf<MorphWithTerrain>(object))
	{
		if (const auto* land = Land())
		{
			const float baseHeight = land->HeightAt(glm::vec2(origin.x, origin.z));
			const auto centreOfMass = shape.centreOfMass * scale;
			shape.radius = 0.0f;
			for (auto& point : shape.points)
			{
				const auto world = origin + axes * (point + centreOfMass);
				point.y += land->HeightAt(glm::vec2(world.x, world.z)) - baseHeight;
				shape.radius = std::max(shape.radius, glm::length(point));
			}
		}
	}
	auto body = std::make_unique<physics::Body>(setup, shape);
	body->SetUpPose({.axes = axes * scale, .origin = origin});
	return body;
}

std::vector<physics::Ellipsoid> DynamicsSystem::CreatureSkeleton(entt::entity creature)
{
	auto& registry = Entities();
	const auto* animation = registry.TryGet<const CreatureAnimation>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	const auto* kind = registry.TryGet<const Creature>(creature);
	if (animation == nullptr || transform == nullptr || kind == nullptr || animation->boneMatrices.empty() ||
	    !Locator::resources::has_value())
	{
		return {};
	}
	// Each bone's box about the vertices it moves in the species' base model, worked out once
	const auto meshId = creature::GetIdFromType(kind->species, creature::CreatureBody::Appearance::Base);
	auto boxes = _creatureBoxes.find(meshId);
	if (boxes == _creatureBoxes.end())
	{
		std::vector<glm::vec3> positions;
		std::vector<uint16_t> bones;
		const auto& meshes = Locator::resources::value().GetMeshes();
		if (meshes.Contains(meshId))
		{
			for (const auto& subMesh : meshes.Handle(meshId)->GetSubMeshes())
			{
				const auto& geometry = subMesh->GetBodyGeometry();
				positions.insert(positions.end(), geometry.positions.begin(), geometry.positions.end());
				if (geometry.bones.empty())
				{
					bones.insert(bones.end(), geometry.positions.size(), uint16_t {0});
				}
				else
				{
					bones.insert(bones.end(), geometry.bones.begin(), geometry.bones.end());
				}
			}
		}
		boxes =
		    _creatureBoxes.emplace(meshId, physics::shapes::BoneBoxes(positions, bones, animation->boneMatrices.size())).first;
	}
	// Placed by the bones as they are posed now
	const auto placement = creature::PlacementMatrix(transform->position, transform->rotation, transform->scale);
	std::vector<physics::Ellipsoid> skeleton;
	skeleton.reserve(animation->boneMatrices.size());
	for (uint32_t bone = 0; bone < animation->boneMatrices.size(); ++bone)
	{
		const auto posed = creature::PosedBone(bone, animation->boneMatrices, placement);
		const auto box = bone < boxes->second.size() ? boxes->second[bone] : physics::shapes::BoneBox {};
		skeleton.push_back({.boxMin = box.min, .boxMax = box.max, .localToWorld = posed, .worldToLocal = glm::inverse(posed)});
	}
	return skeleton;
}

std::unique_ptr<physics::Body> DynamicsSystem::MakeCreatureBody(entt::entity creature, const physics::Material& material)
{
	auto skeleton = CreatureSkeleton(creature);
	if (skeleton.empty())
	{
		return nullptr;
	}
	// Its body is the sphere about its bones' origins as they are posed: a point at each, though each sits on the centre
	// in the body's own frame
	std::vector<glm::vec3> origins;
	origins.reserve(skeleton.size());
	glm::vec3 centre(0.0f);
	for (const auto& part : skeleton)
	{
		origins.emplace_back(part.localToWorld[3]);
		centre += origins.back();
	}
	centre /= static_cast<float>(origins.size());
	float radius = 0.0f;
	for (const auto& point : origins)
	{
		radius = std::max(radius, glm::distance(point, centre));
	}
	auto body = std::make_unique<physics::Body>(
	    physics::BodySetup {.mass = physics::shapes::k_CreatureMass, .material = material, .dynamic = false},
	    physics::shapes::Creature(radius, origins.size()));
	body->PlacePoints(centre, origins);
	body->SetSkeleton(std::move(skeleton));
	return body;
}

void DynamicsSystem::MakeRoomForOne()
{
	if (_entries.size() >= _capacity)
	{
		_capacity += k_SlotsPerGrowth;
	}
}

std::optional<size_t> DynamicsSystem::IndexOf(entt::entity object) const
{
	for (size_t i = 0; i < _entries.size(); ++i)
	{
		if (_entries[i]->entity == object)
		{
			return i;
		}
	}
	return std::nullopt;
}

PhysicsEntry* DynamicsSystem::Find(entt::entity object)
{
	const auto index = IndexOf(object);
	return index.has_value() ? _entries[*index].get() : nullptr;
}

bool DynamicsSystem::PhysicallyDestroysAbodes(entt::entity object) const
{
	return Entities().Valid(object) && FactsOf(object).physicallyDestroysAbodes;
}

bool DynamicsSystem::IsFlying(entt::entity object) const
{
	const auto index = IndexOf(object);
	return index.has_value() && _entries[*index]->IsFlying();
}

void DynamicsSystem::RemoveAt(size_t index)
{
	auto* gone = _entries[index].get();
	for (auto& entry : _entries)
	{
		if (entry->hitBy == gone)
		{
			entry->hitBy = nullptr;
		}
		if (entry->body != nullptr && entry->body->lastHit == gone->body.get())
		{
			entry->body->lastHit = nullptr;
		}
	}
	if (IsAvailable(gone->entity))
	{
		Entities().Remove<PhysicsDrawPose>(gone->entity);
	}
	// The last entry moves into its place
	if (index + 1 != _entries.size())
	{
		_entries[index] = std::move(_entries.back());
	}
	_entries.pop_back();
}

PhysicsEntry* DynamicsSystem::AddObject(entt::entity object, const PhysicsStart& start)
{
	const auto facts = FactsOf(object);
	if (!facts.canBecomePhysicsObject || facts.immovable)
	{
		return nullptr;
	}
	if (const auto index = IndexOf(object))
	{
		if (_entries[*index]->IsFlying())
		{
			return nullptr;
		}
		// A resting obstacle makes way for the moving body
		RemoveAt(*index);
	}
	auto body = MakeBody(object, facts);
	if (body == nullptr)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Physics: #{} has no shape to move with", entt::to_integral(object));
		return nullptr;
	}
	MakeRoomForOne();
	body->velocity = start.velocity;
	if (const float speed = glm::length(body->velocity); speed > physics::k_MaxSpeed)
	{
		body->velocity *= physics::k_MaxSpeed / speed;
	}
	// The spin comes the game's way round, the opposite of the body's
	body->SetAngularVelocityInBodyAxes(-start.spin);
	auto entry = std::make_unique<PhysicsEntry>();
	entry->entity = object;
	entry->thrower = start.thrower;
	entry->player = start.player;
	entry->body = std::move(body);
	entry->kind = Entities().AllOf<Villager>(object) ? PhysicsEntry::Kind::Villager : PhysicsEntry::Kind::Other;
	entry->flags = PhysicsEntry::k_Awake | (facts.alwaysStays ? PhysicsEntry::k_AlwaysStays : 0);
	TakeKind(*entry, facts);
	if (start.fromHand)
	{
		entry->flags |= PhysicsEntry::k_FromHand;
	}
	_entries.push_back(std::move(entry));
	return _entries.back().get();
}

PhysicsEntry* DynamicsSystem::AddProxy(entt::entity object)
{
	const auto facts = FactsOf(object);
	auto body = MakeBody(object, facts);
	if (body == nullptr)
	{
		return nullptr;
	}
	MakeRoomForOne();
	body->resting = true;
	auto entry = std::make_unique<PhysicsEntry>();
	entry->entity = object;
	entry->body = std::move(body);
	entry->kind = Entities().AllOf<Villager>(object) ? PhysicsEntry::Kind::Villager : PhysicsEntry::Kind::Other;
	entry->flags = PhysicsEntry::k_Awake | (facts.alwaysStays ? PhysicsEntry::k_AlwaysStays : 0);
	TakeKind(*entry, facts);
	_entries.push_back(std::move(entry));
	return _entries.back().get();
}

void DynamicsSystem::LetGoOfLeashesTiedTo(entt::entity object)
{
	if (!Locator::leashSystem::has_value())
	{
		return;
	}
	auto& leashes = Locator::leashSystem::value();
	Entities().Each<const Creature>([&leashes, object](entt::entity creature, const Creature&) {
		if (leashes.TiedTo(creature) == object)
		{
			leashes.UntieToHand(creature);
		}
	});
}

PhysicsStarted DynamicsSystem::InitialisePhysics(entt::entity object, const PhysicsStart& start)
{
	return Hooks().InitialisePhysics(*this, object, start);
}

PhysicsStarted DynamicsSystem::StartPhysicsAsObject(entt::entity object, const PhysicsStart& start)
{
	auto& registry = Entities();
	if (!IsAvailable(object) || registry.AllOf<InPhysics>(object) || registry.AllOf<Immovable>(object))
	{
		return {};
	}
	LetGoOfLeashesTiedTo(object);
	registry.AssignOrReplace<InPhysics>(object);
	// Out of the map's cells while it moves
	registry.Remove<MapCellResident, MapCellMover>(object);
	PhysicsEntry* entry = nullptr;
	if (start.add)
	{
		entry = AddObject(object, start);
		if (entry == nullptr)
		{
			// Refused: it stays marked and out of the map, as the game leaves it
			return {};
		}
		Hooks().OfferToCatchingCreatures(object, *entry);
	}
	// A burning thing that starts to move leaves its fire's group
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().StartedMoving(object, false);
	}
	registry.SetDirty();
	return {.entry = entry, .started = true};
}

entt::entity DynamicsSystem::EndPhysicsAsObject(entt::entity object, bool insert, bool hasBody)
{
	auto& registry = Entities();
	if (!IsAvailable(object) || (hasBody && !registry.AllOf<InPhysics>(object)))
	{
		return object;
	}
	registry.Remove<InPhysics>(object);
	if (insert)
	{
		const auto* transform = registry.TryGet<const Transform>(object);
		if (transform != nullptr && map_coords::InBounds(transform->position))
		{
			if (Locator::entitiesMap::has_value())
			{
				Locator::entitiesMap::value().Refile(object);
			}
		}
		else if (transform != nullptr)
		{
			// It came to rest off the map: it goes, and its entry leaves the physics at the next turn's start
			world_objects::Remove(object);
			return entt::null;
		}
	}
	// What it set the people and animals near it reacting to, flying by, is over, and the creature may copy what landed
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(object, Reaction::ReactToFlyingObject);
	}
	Hooks().ConsiderMimickingLanding(object, std::nullopt);
	return object;
}

void DynamicsSystem::SyncObject(const PhysicsEntry& entry, Place place)
{
	auto& registry = Entities();
	if (!IsAvailable(entry.entity))
	{
		return;
	}
	auto* transform = registry.TryGet<Transform>(entry.entity);
	if (transform == nullptr || entry.body == nullptr)
	{
		return;
	}
	const auto& body = *entry.body;
	auto axes = body.Axes();
	if (entry.upright)
	{
		// A living thing, a tree or a feature takes only its heading and stands upright
		axes = UprightAxes(axes);
	}
	transform->rotation = entry.animated ? physics::QuarterTurned(axes) : axes;
	switch (place)
	{
	case Place::Origin:
		transform->position = body.ObjectPose().origin;
		break;
	case Place::Centre:
		transform->position = body.Centre();
		break;
	case Place::Kept:
		break;
	}
	registry.SetDirty();
}

void DynamicsSystem::TakeKind(PhysicsEntry& entry, const physics_classes::ClassFacts& facts)
{
	entry.checksPoints = facts.checksPoints;
	entry.animated = facts.animated;
	entry.upright = facts.upright;
}

void DynamicsSystem::SettleOnLand(PhysicsEntry& entry, bool noPullDown, bool alignToSlope)
{
	if (const auto* land = Land(); land != nullptr && entry.body != nullptr)
	{
		entry.body->SettleOnLand(*land, noPullDown, alignToSlope);
	}
}

void DynamicsSystem::RaiseClearOfWhatIsUnder(PhysicsEntry& entry)
{
	auto& registry = Entities();
	auto* body = entry.body.get();
	if (body == nullptr)
	{
		return;
	}
	// Resting bodies for what may be under it: not what threw it, nor what already has a body, nor what doesn't raise
	// what is dropped on it
	const auto cells = turn::SquareCells(body->Centre(), body->Radius());
	for (int32_t x = cells.low.x; x <= cells.high.x; ++x)
	{
		for (int32_t z = cells.low.y; z <= cells.high.y; ++z)
		{
			if (!Locator::entitiesMap::has_value())
			{
				continue;
			}
			for (const auto other : Locator::entitiesMap::value().GetAllInCell({x, z}))
			{
				if (other == entry.entity || other == entry.thrower || !registry.AllOf<Mesh>(other) ||
				    !Hooks().RaisesObjects(other) || IndexOf(other).has_value() || !FactsOf(other).interacts)
				{
					continue;
				}
				AddProxy(other);
			}
		}
	}
	// Then it is raised by the deepest overlap with any body near it, looking down through it and up through the other,
	// starting again from the first body after each raise, until nothing pushes it up
	const bool villager = entry.kind == PhysicsEntry::Kind::Villager;
	bool raised = true;
	while (raised)
	{
		raised = false;
		for (auto& other : _entries)
		{
			if (other.get() == &entry || other->body == nullptr)
			{
				continue;
			}
			// A villager isn't raised over what a living thing pushed, nor that over a villager
			if ((villager && other->Has(PhysicsEntry::k_PushedByLiving)) ||
			    (other->kind == PhysicsEntry::Kind::Villager && entry.Has(PhysicsEntry::k_PushedByLiving)))
			{
				continue;
			}
			const auto offset = body->Centre() - other->body->Centre();
			const float reach = body->Radius() + other->body->Radius();
			if (glm::dot(offset, offset) >= reach * reach)
			{
				continue;
			}
			const float depth = std::max(body->OverlapDepth(*other->body, k_Down), other->body->OverlapDepth(*body, k_Up));
			if (depth > k_LeastRaise)
			{
				body->Raise(depth);
				raised = true;
				break;
			}
		}
	}
}

entt::entity DynamicsSystem::RemoveObject(entt::entity object, bool insert, bool endPhysics)
{
	if (_inTurnUpdate)
	{
		return object;
	}
	auto kept = object;
	for (size_t i = 0; i < _entries.size(); ++i)
	{
		auto& entry = *_entries[i];
		if (entry.entity != object)
		{
			continue;
		}
		SyncObject(entry, Place::Origin);
		if (endPhysics)
		{
			kept = Hooks().EndPhysics(*this, &entry, object, insert);
			if (entry.Has(PhysicsEntry::k_Landed) && IsAvailable(kept))
			{
				if (const auto* land = Land(); land != nullptr)
				{
					const auto& position = Entities().Get<const Transform>(kept).position;
					if (land->IsLand(glm::vec2(position.x, position.z)))
					{
						Hooks().DropSound(kept);
					}
				}
			}
		}
		if (IsAvailable(object))
		{
			Entities().Remove<InPhysics>(object);
		}
		RemoveAt(i);
		break;
	}
	Hooks().ForgetBuildingHitter(object);
	return kept;
}

float DynamicsSystem::PushObject(entt::entity object)
{
	if (!IsAvailable(object))
	{
		return 0.0f;
	}
	const float force =
	    turn::PushForce(physics_classes::Weight(InfoWeight(object), Entities().Get<const Transform>(object).scale.x));
	PhysicsEntry* entry = nullptr;
	if (!Entities().AllOf<InPhysics>(object))
	{
		entry = InitialisePhysics(object, {}).entry;
	}
	else
	{
		entry = Find(object);
	}
	if (entry == nullptr || entry->body == nullptr)
	{
		return 0.0f;
	}
	entry->flags |= PhysicsEntry::k_PushedByLiving;
	const auto offset = entry->body->Centre() - Entities().Get<const Transform>(object).position;
	if (const float length = glm::length(offset); length > 0.0f)
	{
		entry->body->externalForce += force * offset / length;
	}
	return 0.0f;
}

void DynamicsSystem::ForgetAsBuildingHitter(entt::entity object)
{
	// Only what breaks buildings is remembered by them: the buildings' bodies stop passing it through, and they forget it
	// struck them
	if (!FactsOf(object).physicallyDestroysAbodes)
	{
		return;
	}
	for (auto& entry : _entries)
	{
		if (entry->thrower == object && IsAvailable(entry->entity) && Entities().AllOf<Abode>(entry->entity))
		{
			entry->thrower = entt::null;
		}
	}
	Hooks().ForgetBuildingHitter(object);
}

void DynamicsSystem::RecordHit(entt::entity hit, entt::entity hitter)
{
	_hitObject = IsAvailable(hit) ? hit : entt::null;
	_objectWhichHit = IsAvailable(hitter) ? hitter : entt::null;
}

entt::entity DynamicsSystem::GetHitObject() const
{
	return IsAvailable(_hitObject) ? _hitObject : entt::null;
}

entt::entity DynamicsSystem::GetObjectWhichHit() const
{
	return IsAvailable(_objectWhichHit) ? _objectWhichHit : entt::null;
}

void DynamicsSystem::ForEachEntry(const std::function<void(const PhysicsEntry&)>& visit) const
{
	for (const auto& entry : _entries)
	{
		visit(*entry);
	}
}

// The turn

void DynamicsSystem::ProcessTurn()
{
	const auto* land = Land();
	if (land == nullptr || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	_inTurnUpdate = true;
	BeginTurn();
	for (int step = 0; step < physics::k_StepsPerTurn; ++step)
	{
		Step(*land);
	}
	EndTurn();
	_inTurnUpdate = false;
}

void DynamicsSystem::BeginTurn()
{
	for (size_t i = 0; i < _entries.size();)
	{
		auto& entry = *_entries[i];
		// What is no longer there leaves the physics
		if (!IsAvailable(entry.entity))
		{
			RemoveAt(i);
			continue;
		}
		// The dead grow heavier, so that corpses sink
		if (world_objects::LifeOf(entry.entity) < turn::k_DeadLife)
		{
			entry.body->density += turn::k_CorpseSoaking;
		}
		entry.body->BeginTurn();
		// A creature's skeleton is hit as it is posed now
		if (Entities().AllOf<Creature>(entry.entity))
		{
			if (auto skeleton = CreatureSkeleton(entry.entity); !skeleton.empty())
			{
				entry.body->SetSkeleton(std::move(skeleton));
			}
		}
		entry.forceSum = glm::vec3(0.0f);
		entry.body->lastHit = nullptr;
		++i;
	}
	// Moving bodies are awake; resting ones only when something moving is near them, or they always stay
	for (auto& entry : _entries)
	{
		if (entry->body->resting && !entry->Has(PhysicsEntry::k_AlwaysStays))
		{
			entry->flags &= static_cast<uint8_t>(~PhysicsEntry::k_Awake);
		}
		else
		{
			entry->flags |= PhysicsEntry::k_Awake;
		}
	}
	WakeNearMovingBodies();
	for (size_t i = 0; i < _entries.size();)
	{
		if (!_entries[i]->Has(PhysicsEntry::k_Awake))
		{
			RemoveAt(i);
			continue;
		}
		++i;
	}
}

void DynamicsSystem::WakeNearMovingBodies()
{
	if (!Locator::entitiesMap::has_value())
	{
		return;
	}
	const auto& map = Locator::entitiesMap::value();
	auto& registry = Entities();
	// Each cell is walked once for all the bodies, though only so many are remembered
	constexpr size_t k_MostWalkedCells = 0x200;
	std::vector<glm::ivec2> walked;
	walked.reserve(k_MostWalkedCells);
	const auto full = [this]() { return _entries.size() >= _capacity; };
	const auto wake = [&](entt::entity object) {
		if (!FactsOf(object).interacts)
		{
			return;
		}
		if (auto* known = Find(object))
		{
			known->flags |= PhysicsEntry::k_Awake;
		}
		else if (registry.AllOf<Mesh>(object))
		{
			AddProxy(object);
		}
	};
	const size_t bodies = _entries.size();
	for (size_t b = 0; b < bodies && !full(); ++b)
	{
		const auto& moving = *_entries[b];
		if (moving.body->resting)
		{
			continue;
		}
		const auto cells = turn::WakeCells(moving.body->Centre(), moving.body->velocity, moving.body->Radius());
		for (int32_t x = cells.low.x; x <= cells.high.x && !full(); ++x)
		{
			for (int32_t z = cells.low.y; z <= cells.high.y && !full(); ++z)
			{
				const glm::ivec2 cell(x, z);
				if (std::ranges::find(walked, cell) != walked.end())
				{
					continue;
				}
				if (walked.size() < k_MostWalkedCells)
				{
					walked.push_back(cell);
				}
				// The things that stay put first, then those that move
				const MapInterface::CellId id(static_cast<uint16_t>(x), static_cast<uint16_t>(z));
				const auto fixed =
				    std::vector<entt::entity>(map.GetFixedInGridCell(id).begin(), map.GetFixedInGridCell(id).end());
				for (const auto object : fixed)
				{
					if (full())
					{
						break;
					}
					wake(object);
				}
				const auto mobile =
				    std::vector<entt::entity>(map.GetMobileInGridCell(id).begin(), map.GetMobileInGridCell(id).end());
				for (const auto object : mobile)
				{
					if (full())
					{
						break;
					}
					wake(object);
				}
			}
		}
	}
}

void DynamicsSystem::Step(const physics::Ground& ground)
{
	auto& registry = Entities();
	for (auto& entry : _entries)
	{
		entry->body->ClearForces();
		entry->body->TouchGround(ground);
	}
	// Which bodies' points are tested against which bodies' faces
	std::vector<physics::PairMember> members;
	members.reserve(_entries.size());
	for (const auto& entry : _entries)
	{
		members.push_back({.centre = entry->body->Centre(),
		                   .radius = entry->body->Radius(),
		                   .resting = entry->body->resting,
		                   .justSetUp = entry->body->justSetUp,
		                   .checksPoints = entry->checksPoints,
		                   .noObjectCollision = entry->Has(PhysicsEntry::k_NoObjectCollision),
		                   .isVillager = entry->kind == PhysicsEntry::Kind::Villager,
		                   .pushedByLiving = entry->Has(PhysicsEntry::k_PushedByLiving),
		                   .object = Id(entry->entity),
		                   .thrower = Id(entry->thrower)});
	}
	for (size_t a = 0; a < _entries.size(); ++a)
	{
		for (size_t b = 0; b < _entries.size(); ++b)
		{
			if (a != b && physics::TestsPoints(members[a], members[b]))
			{
				_entries[a]->body->TouchFaces(*_entries[b]->body);
			}
		}
	}
	for (auto& entry : _entries)
	{
		entry->body->ApplyContacts();
	}
	const auto camera = Locator::camera::has_value() ? std::optional(Locator::camera::value().GetOrigin()) : std::nullopt;
	for (size_t i = 0; i < _entries.size();)
	{
		auto& entry = *_entries[i];
		auto& body = *entry.body;
		const float upwardBefore = body.velocity.y;
		const auto centreBefore = body.Centre();
		auto result = body.Integrate();
		if (turn::NearSeaLevel(body.resting, body.Centre().y, body.Radius()))
		{
			if (body.density > 1.0f && IsAvailable(entry.entity) && Hooks().HasSunk(*this, entry))
			{
				body.velocity = glm::vec3(0.0f);
				body.angularMomentum = glm::vec3(0.0f);
				result = physics::StepResult::Stopped;
			}
			if (turn::Bobbed(upwardBefore, body.velocity.y) && Locator::waterRingSystem::has_value())
			{
				Locator::waterRingSystem::value().Add(turn::BobRing(body.Centre(), body.Radius()));
			}
		}
		if (camera.has_value() && turn::PassesCamera(centreBefore, body.Centre(), *camera, body.velocity) &&
		    Locator::audio::has_value())
		{
			// One of the five whooshes, picked by the system clock's milliseconds
			constexpr std::array<audio::SoundId, turn::k_Whooshes> k_RockPasts = {
			    audio::SoundId::G_RockPast_01, audio::SoundId::G_RockPast_02, audio::SoundId::G_RockPast_03,
			    audio::SoundId::G_RockPast_04, audio::SoundId::G_RockPast_05};
			Locator::audio::value().PlaySoundEffect(
			    static_cast<entt::id_type>(k_RockPasts.at(static_cast<size_t>(_ticks() % turn::k_Whooshes))), std::nullopt);
		}
		switch (result)
		{
		case physics::StepResult::Moved:
			SyncObject(entry, Place::Origin);
			break;
		case physics::StepResult::Stopped:
		{
			// It comes to rest: it stops breaking buildings, takes the body's place (a villager stands at its centre) and its
			// kind ends its physics. Whatever happens to its object, the body stays in the list as a resting obstacle until
			// the next turn's start.
			const auto object = entry.entity;
			if (IsAvailable(object))
			{
				ForgetAsBuildingHitter(object);
				SyncObject(entry, entry.kind == PhysicsEntry::Kind::Villager ? Place::Centre : Place::Origin);
				const auto kept = Hooks().EndPhysics(*this, &entry, object, true);
				if (kept != object && IsAvailable(object))
				{
					registry.Remove<PhysicsDrawPose>(object);
				}
				if (IsAvailable(kept))
				{
					// What stays rests here as an obstacle, turned as the body is
					entry.entity = kept;
					TakeKind(entry, FactsOf(kept));
					SyncObject(entry, Place::Kept);
					registry.Remove<PhysicsDrawPose>(kept);
					// A dead tree covers the ground as it lies
					if (registry.AllOf<DeadTree>(kept))
					{
						archetypes::DeadTreeArchetype::FitObstacle(kept);
					}
				}
				else
				{
					entry.entity = entt::null;
				}
			}
			body.resting = true;
			break;
		}
		case physics::StepResult::Knocked:
		{
			// A resting obstacle hit harder than it holds starts to move, if its kind lets it and no script holds it
			if (IsAvailable(entry.entity))
			{
				const auto facts = FactsOf(entry.entity);
				if (facts.canBecomePhysicsObject && !facts.immovable && InitialisePhysics(entry.entity, {.add = false}).started)
				{
					body.resting = false;
				}
			}
			break;
		}
		case physics::StepResult::Delete:
		{
			// Fallen deep under the sea: its object goes, but the body is still stepped and hit for the rest of the turn
			// and leaves the physics at the next turn's start
			if (IsAvailable(entry.entity))
			{
				const auto object = entry.entity;
				entry.entity = entt::null;
				Entities().Remove<PhysicsDrawPose>(object);
				world_objects::Remove(object);
			}
			break;
		}
		default:
			break;
		}
		if (body.touched)
		{
			entry.forceSum += body.Force();
		}
		++i;
	}
}

void DynamicsSystem::EndTurn()
{
	auto& registry = Entities();
	for (size_t i = 0; i < _entries.size(); ++i)
	{
		auto& entry = *_entries[i];
		auto& body = *entry.body;
		// The steady pushes last one turn
		body.externalForce = glm::vec3(0.0f);
		body.externalTorque = glm::vec3(0.0f);
		// An entry whose object has gone this turn still takes its knock and passes on its credit; only its object's own
		// parts are skipped
		const bool available = IsAvailable(entry.entity);
		// A felled tree makes one sound as it topples, if it is tall
		if (available && entry.kind == PhysicsEntry::Kind::FelledTree && body.Axes()[1].y < turn::k_FelledUpright)
		{
			if (world_objects::SizeOf(entry.entity).height > turn::k_FelledSoundHeight)
			{
				Hooks().FelledTreeToppled(entry.entity);
			}
			entry.kind = PhysicsEntry::Kind::FelledTreeToppled;
		}
		// A resting obstacle follows its object about
		if (available && body.resting)
		{
			if (const auto* transform = registry.TryGet<const Transform>(entry.entity))
			{
				body.MoveCentre(transform->position + body.Axes() * (body.CentreOfMass() * body.Scale()));
			}
		}
		const auto impact = turn::Impact(entry.forceSum);
		if (!impact.has_value())
		{
			continue;
		}
		entry.impact = *impact;
		// The body it last hit is the one that hit it; one hit by nothing was hit by nobody
		if (body.lastHit == nullptr)
		{
			entry.hitBy = nullptr;
		}
		else
		{
			for (auto& other : _entries)
			{
				if (other->body.get() == body.lastHit)
				{
					entry.hitBy = other.get();
				}
			}
		}
		// Credit for a throw passes on through what it hits
		if (!entry.player.has_value() && entry.hitBy != nullptr)
		{
			entry.player = entry.hitBy->player;
		}
		if (!available)
		{
			continue;
		}
		if (turn::WantsCollisionSound(body.resting, entry.hitBy != nullptr, entry.impact, body.Mass()))
		{
			AttemptCollisionSound(entry);
		}
		const ImpactInfo info {
		    .g = turn::GLoad(entry.impact, body.Mass()),
		    .impact = entry.impact,
		    .hitBy = entry.hitBy != nullptr ? entry.hitBy->entity : entt::null,
		    .thrower = entry.thrower,
		    .player = entry.player,
		    .fromHand = entry.Has(PhysicsEntry::k_FromHand),
		};
		Hooks().ReactToImpact(*this, entry, info);
		// The entry may have been replaced by what the reaction made
		if (i >= _entries.size() || _entries[i].get() != &entry)
		{
			continue;
		}
		Hooks().ImpactFeedback(*this, entry, false);
		if (entry.hitBy != nullptr)
		{
			RecordHit(entry.entity, entry.hitBy->entity);
			Hooks().ImpactFeedback(*this, entry, true);
		}
	}
	_soundPairs.EndTurn();
}

void DynamicsSystem::AttemptCollisionSound(PhysicsEntry& entry)
{
	auto& registry = Entities();
	const auto hitter = entry.entity;
	const auto hit = entry.hitBy != nullptr ? entry.hitBy->entity : entt::null;
	const auto centre = entry.body->Centre();
	// A rock hitting a building leaves the building to sound
	if (IsAvailable(hit) && FactsOf(hitter).physicallyDestroysAbodes && registry.AllOf<Abode>(hit))
	{
		return;
	}
	if (!_soundPairs.MaySound(Id(hitter), Id(hit)))
	{
		return;
	}
	const auto hitterType = Hooks().CollideSoundType(hitter);
	if (hitterType == SoundCollisionType::Fragment)
	{
		return;
	}
	auto hitType = IsAvailable(hit) ? Hooks().CollideSoundType(hit) : SoundCollisionType::Ground;
	if (hitType == SoundCollisionType::Fragment)
	{
		return;
	}
	if (hitType == SoundCollisionType::Ground)
	{
		// Over land it throws up dust; over water a ring and foam
		const auto* land = Land();
		const glm::vec2 xz(centre.x, centre.z);
		auto colour = turn::k_LandDust;
		if (land != nullptr && !land->IsDryLand(xz))
		{
			if (land->IsDeepWater(xz))
			{
				hitType = SoundCollisionType::Water;
				colour = turn::k_SeaFoam;
			}
			if (Locator::waterRingSystem::has_value())
			{
				Locator::waterRingSystem::value().Add(turn::ImpactRing(centre, entry.body->Radius()));
			}
		}
		AddLandingDust(glm::vec3(centre.x, land != nullptr ? land->HeightAt(xz) : centre.y, centre.z), entry.body->Radius(),
		               colour);
	}
	// How loud it is goes by its kind's weight, unscaled; a thing of no kind makes no sound
	const auto infoWeight = InfoWeight(hitter);
	if (infoWeight > 0.0f && Locator::audio::has_value())
	{
		const auto level = turn::CollisionLevel(entry.impact, infoWeight, hitterType == SoundCollisionType::Grain);
		const auto keys = turn::CollisionKeys(level, static_cast<int32_t>(hitterType), static_cast<int32_t>(hitType));
		const auto played = Locator::audio::value().PlayAnimEffect(std::string(k_CollisionBank), keys, hitter, centre);
		// The sound follows the thing that made it, unless its code says it stays where it was made
		if (played.emitter != entt::null && turn::CollisionSoundFollows(keys))
		{
			_followingSounds.push_back({.emitter = played.emitter, .owner = hitter});
		}
	}
	_soundPairs.Add(Id(hitter), Id(hit));
}

void DynamicsSystem::AddLandingDust(glm::vec3 centre, float radius, uint32_t argb)
{
	auto* random = Locator::gameRandom::has_value() ? &Locator::gameRandom::value() : nullptr;
	const auto speed = [random]() {
		const auto r = random != nullptr ? static_cast<float>(random->LocalRand(201)) : 100.0f;
		return (r - 100.0f) * turn::k_PuffSpeed;
	};
	for (int32_t i = 0; i < turn::k_PuffsPerLanding; ++i)
	{
		// Each axis of its speed is drawn z first, then y, then x, whether or not there is room for the puff
		const float z = speed();
		const float y = speed();
		const float x = speed();
		if (_dust.size() >= turn::k_MostPuffs)
		{
			continue;
		}
		_dust.push_back({.position = centre,
		                 .velocity = glm::vec3(x, y, z),
		                 .size = turn::LandingPuffSize(radius),
		                 .variant = random != nullptr ? random->CrtRand() & 15 : 0,
		                 .argb = argb});
	}
}

void DynamicsSystem::UpdateFrame(float turnFraction, float gameSeconds)
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Entities();
	bool drawn = false;
	for (const auto& entry : _entries)
	{
		if (!IsAvailable(entry->entity) || !registry.AllOf<Transform>(entry->entity))
		{
			continue;
		}
		const auto& body = *entry->body;
		if (body.resting)
		{
			registry.Remove<PhysicsDrawPose>(entry->entity);
			continue;
		}
		// Sunk deeper than its radius, it keeps the pose it was last drawn at
		if (body.Centre().y <= -body.Radius())
		{
			continue;
		}
		const auto pose = body.DrawPose(turnFraction, entry->animated);
		registry.AssignOrReplace<PhysicsDrawPose>(entry->entity, PhysicsDrawPose {.axes = pose.axes, .origin = pose.origin});
		drawn = true;
	}
	std::erase_if(_dust, [gameSeconds](turn::DustPuff& puff) { return !turn::AdvanceDustPuff(puff, gameSeconds); });
	// The collision sounds follow what made them while they play
	if (Locator::audio::has_value())
	{
		auto& audio = Locator::audio::value();
		std::erase_if(_followingSounds, [&registry, &audio](const FollowingSound& sound) {
			const auto* emitter = registry.Valid(sound.emitter) ? registry.TryGet<const AudioEmitter>(sound.emitter) : nullptr;
			if (emitter == nullptr || emitter->state == audio::AudioStatus::Stopped || !IsAvailable(sound.owner))
			{
				return true;
			}
			const auto* drawn = registry.TryGet<const PhysicsDrawPose>(sound.owner);
			const auto* transform = registry.TryGet<const Transform>(sound.owner);
			if (drawn != nullptr || transform != nullptr)
			{
				audio.SetEmitterPosition(sound.emitter, drawn != nullptr ? drawn->origin : transform->position);
			}
			return false;
		});
	}
	if (drawn)
	{
		registry.SetDirty();
	}
}

void DynamicsSystem::CollectDrawFrame(particles::draw::Frame& frame) const
{
	if (_dust.empty())
	{
		return;
	}
	const particles::draw::Sources sources {
	    .textures = [](std::string_view /*texture*/) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    return std::pair {k_DustSheet.value(), k_DustSheetAlpha.value()};
	    },
	    .playerColour = {},
	    .random = {},
	};
	particles::Effect::DrawWalk walk;
	for (const auto& puff : _dust)
	{
		walk.steps.push_back({.chain = false, .index = static_cast<uint32_t>(walk.atoms.size())});
		walk.atoms.push_back({
		    .creator = &_dustCreator,
		    .position = puff.position,
		    .rotation = glm::mat3(1.0f),
		    .scale = turn::DustPuffSize(puff),
		    .stretch = 1.0f,
		    .alpha = static_cast<float>(puff.argb >> 24u),
		    .frame = static_cast<float>(turn::DustPuffFrame(puff)),
		    .rgb = {static_cast<uint8_t>((puff.argb >> 16u) & 0xFFu), static_cast<uint8_t>((puff.argb >> 8u) & 0xFFu),
		            static_cast<uint8_t>(puff.argb & 0xFFu)},
		});
	}
	particles::draw::AddEffect(frame, walk, particles::draw::DrawPath::Sorted, _dust.front().position,
	                           particles::draw::k_NeutralPlayer, sources);
}
