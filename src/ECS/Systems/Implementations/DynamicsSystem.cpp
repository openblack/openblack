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

#include <BulletCollision/BroadphaseCollision/btDbvtBroadphase.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcher.h>
#include <BulletCollision/CollisionDispatch/btCollisionObject.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/MagicShield.h"
#include "ECS/Components/MapCellResident.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/RigidBody.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/PhysicsGround.h"
#include "ECS/Registry.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleEffect.h"
#include "Physics/BodyShapes.h"
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
const particles::Creator& DustCreator()
{
	static const particles::Creator creator = [] {
		particles::Creator dust;
		dust.kind = particles::Creator::Kind::Sprite;
		dust.className = "PhysicsDust";
		dust.texture = "blobs";
		dust.spritesPerRow = 8;
		dust.numFrames = 64;
		dust.additive = false;
		return dust;
	}();
	return creator;
}

/// Upright axes with the same heading across the ground as the given ones
glm::mat3 UprightAxes(const glm::mat3& axes)
{
	glm::vec3 forward(axes[2].x, 0.0f, axes[2].z);
	const float length = glm::length(forward);
	forward = length > 0.0f ? forward / length : glm::vec3(0.0f, 0.0f, 1.0f);
	glm::mat3 upright;
	upright[1] = glm::vec3(0.0f, 1.0f, 0.0f);
	upright[2] = forward;
	upright[0] = glm::cross(upright[1], upright[2]);
	return upright;
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
		const auto& surface = subMesh->GetSurface();
		model.indices.emplace_back(surface.indices.begin(), surface.indices.end());
		model.parts.push_back({.positions = surface.positions,
		                       .indices = model.indices.back(),
		                       .isPhysics = subMesh->IsPhysics(),
		                       .nearestDetail = (subMesh->GetFlags().lodMask & 1u) != 0});
	}
	return model;
}
} // namespace

DynamicsSystem::DynamicsSystem()
    : _configuration(std::make_unique<btDefaultCollisionConfiguration>())
    , _dispatcher(std::make_unique<btCollisionDispatcher>(_configuration.get()))
    , _broadphase(std::make_unique<btDbvtBroadphase>())
    , _solver(std::make_unique<btSequentialImpulseConstraintSolver>())
    , _world(
          std::make_unique<btDiscreteDynamicsWorld>(_dispatcher.get(), _broadphase.get(), _solver.get(), _configuration.get()))
    , _hooks(std::make_unique<PhysicsClassHooks>())
{
	// The ray-cast world only answers rays: nothing in it falls
	_world->setGravity(btVector3(0, 0, 0));
}

void DynamicsSystem::Reset()
{
	std::vector<btRigidBody*> toRemove;
	for (int i = 0; i < _world->getNumCollisionObjects(); ++i)
	{
		auto* obj = _world->getCollisionObjectArray()[i];
		btRigidBody* body = btRigidBody::upcast(obj);
		if (body != nullptr)
		{
			toRemove.push_back(body);
		}
	}
	for (auto& obj : toRemove)
	{
		_world->removeRigidBody(obj);
	}
}

DynamicsSystem::~DynamicsSystem() = default;

void DynamicsSystem::Update(std::chrono::microseconds& dt)
{
	std::chrono::duration<float> seconds = dt;
	_world->stepSimulation(seconds.count());
}

void DynamicsSystem::AddRigidBody(btRigidBody* object)
{
	_world->addRigidBody(object);
}

void DynamicsSystem::RemoveRigidBody(btRigidBody* object)
{
	_world->removeRigidBody(object);
}

void DynamicsSystem::RegisterRigidBodies()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<RigidBody>([this](RigidBody& body) {
		body.handle.setUserIndex(static_cast<int>(RigidBodyType::Entity));
		body.handle.setUserIndex2(0);
		body.handle.setUserPointer(this);
		AddRigidBody(&body.handle);
	});
}

void DynamicsSystem::RegisterIslandRigidBodies(LandIslandInterface& island)
{
	auto& landBlocks = island.GetBlocks();
	for (uint32_t i = 0; i < landBlocks.size(); ++i)
	{
		auto& rigidBody = landBlocks[i].GetRigidBody();
		rigidBody->setUserIndex(static_cast<int>(RigidBodyType::Terrain));
		rigidBody->setUserIndex2(i);
		rigidBody->setUserPointer(reinterpret_cast<void*>(this));
		AddRigidBody(rigidBody.get());
	}
}

void DynamicsSystem::UpdatePhysicsTransforms()
{
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<Transform, const RigidBody>([&registry](Transform& transform, const RigidBody& body) {
		btTransform trans;
		body.motionState->getWorldTransform(trans);

		transform.position.x = trans.getOrigin().getX();
		transform.position.y = trans.getOrigin().getY();
		transform.position.z = trans.getOrigin().getZ();

		glm::quat quaternion(trans.getRotation().getW(), trans.getRotation().getX(), trans.getRotation().getY(),
		                     trans.getRotation().getZ());

		transform.rotation = glm::mat3_cast(quaternion);

		registry.SetDirty();
	});
}

std::optional<glm::vec3> DynamicsSystem::RayCastLand(const glm::vec3& origin, const glm::vec3& direction, float tMax) const
{
	// The closest hit among the land's bodies only
	struct LandOnly final: btCollisionWorld::ClosestRayResultCallback
	{
		using ClosestRayResultCallback::ClosestRayResultCallback;
		[[nodiscard]] bool needsCollision(btBroadphaseProxy* proxy) const override
		{
			const auto* object = static_cast<const btCollisionObject*>(proxy->m_clientObject);
			return object != nullptr && static_cast<RigidBodyType>(object->getUserIndex()) == RigidBodyType::Terrain &&
			       ClosestRayResultCallback::needsCollision(proxy);
		}
	};
	const auto from = btVector3(origin.x, origin.y, origin.z);
	const auto to = from + tMax * btVector3(direction.x, direction.y, direction.z);
	LandOnly callback(from, to);
	_world->rayTest(from, to, callback);
	if (!callback.hasHit())
	{
		return std::nullopt;
	}
	return glm::vec3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());
}

std::optional<std::pair<Transform, RigidBodyDetails>>
DynamicsSystem::RayCastClosestHit(const glm::vec3& origin, const glm::vec3& direction, float tMax) const
{
	auto from = btVector3(origin.x, origin.y, origin.z);
	auto to = from + tMax * btVector3(direction.x, direction.y, direction.z);

	btCollisionWorld::ClosestRayResultCallback callback(from, to);

	_world->rayTest(from, to, callback);

	if (!callback.hasHit())
	{
		return std::nullopt;
	}

	auto translation = glm::vec3(callback.m_hitPointWorld.x(), callback.m_hitPointWorld.y(), callback.m_hitPointWorld.z());
	auto normal = glm::vec3(callback.m_hitNormalWorld.x(), callback.m_hitNormalWorld.y(), callback.m_hitNormalWorld.z());
	const auto up = glm::vec3(0, 1, 0);
	auto rotation = glm::mat4(1.f);
	if (abs(normal) != abs(up))
	{
		rotation = glm::orientation(normal, up);
	}

	return std::make_optional(std::make_pair(
	    Transform {translation, rotation, glm::vec3(1.0f)},
	    RigidBodyDetails {static_cast<RigidBodyType>(callback.m_collisionObject->getUserIndex()),
	                      callback.m_collisionObject->getUserIndex2(), callback.m_collisionObject->getUserPointer()}));
}

// The objects' physics

void DynamicsSystem::ResetSimulation()
{
	_entries.clear();
	_capacity = 0;
	_inTurnUpdate = false;
	_soundPairs.Clear();
	_dust.clear();
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
	physics_classes::ClassInputs inputs {.life = world_objects::LifeOf(object)};
	if (registry.AllOf<Villager>(object))
	{
		// Not reachable at home, in a hand or hiding in a building
		const auto* action = registry.TryGet<const LivingAction>(object);
		const bool hiding = action != nullptr && action->states[static_cast<size_t>(LivingAction::Index::Top)] ==
		                                             static_cast<uint8_t>(VillagerStates::GoAndHideInNearbyBuilding);
		inputs.villagerReachable = !registry.AllOf<AtHome>(object) && !hiding;
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
	return physics_classes::Classify(registry, object, Locator::infoConstants::value(), inputs);
}

std::unique_ptr<physics::Body> DynamicsSystem::MakeBody(entt::entity object, const physics_classes::ClassFacts& facts)
{
	auto& registry = Entities();
	if (facts.body == BodyKind::None)
	{
		return nullptr;
	}
	if (facts.body == BodyKind::Creature)
	{
		return Hooks().CreatureBody(object);
	}
	const auto* materials = Locator::resources::has_value() ? &Locator::resources::value().GetPhysicsMaterials() : nullptr;
	const auto material = materials != nullptr && materials->Contains(physics::k_MaterialsId.value())
	                          ? (*materials->Handle(physics::k_MaterialsId.value()))[facts.row]
	                          : physics::Material {};

	if (facts.body == BodyKind::ShieldDome)
	{
		// The dome's solid shape, as it stands in the world
		const auto& dome = registry.Get<const ShieldDome>(object);
		if (dome.hull.empty())
		{
			return nullptr;
		}
		glm::vec3 centre(0.0f);
		std::vector<glm::vec3> corners;
		for (const auto& triangle : dome.hull)
		{
			corners.insert(corners.end(), triangle.begin(), triangle.end());
		}
		for (const auto& corner : corners)
		{
			centre += corner;
		}
		centre /= static_cast<float>(corners.size());
		physics::Shape shape;
		for (uint32_t i = 0; i < corners.size(); ++i)
		{
			shape.points.push_back(corners[i] - centre);
			shape.radius = std::max(shape.radius, glm::length(shape.points.back()));
		}
		for (uint32_t i = 0; i + 2 < corners.size(); i += 3)
		{
			shape.faces.push_back({i, i + 1, i + 2});
		}
		const float scale = dome.solidScale;
		auto body = std::make_unique<physics::Body>(
		    physics::BodySetup {.scale = 1.0f,
		                        .halfHeight = dome.halfExtent.y,
		                        .mass = physics_classes::BodyMass(physics_classes::Weight(InfoWeight(object), scale)),
		                        .material = material,
		                        .dynamic = false},
		    shape);
		body->SetPoseDirect(glm::mat3(1.0f), centre);
		return body;
	}

	const auto* transform = registry.TryGet<const Transform>(object);
	const auto* meshComponent = registry.TryGet<const Mesh>(object);
	if (transform == nullptr || meshComponent == nullptr || !Locator::resources::has_value())
	{
		return nullptr;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(meshComponent->id))
	{
		return nullptr;
	}
	const auto& mesh = *meshes.Handle(meshComponent->id);
	const float scale = transform->scale.x;
	const auto size = mesh.GetBoundingBox().Size();
	const float halfHeight = 0.5f * size.y;
	const float height = size.y * scale;
	const float radius = 0.5f * std::max(size.x, size.z) * scale;
	const float mass = facts.fixedMass.value_or(physics_classes::BodyMass(physics_classes::Weight(InfoWeight(object), scale)));
	const physics::BodySetup setup {
	    .scale = scale, .halfHeight = halfHeight, .mass = mass, .material = material, .dynamic = facts.canBecomePhysicsObject};

	physics::Shape shape;
	auto axes = transform->rotation;
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
		// The body takes the axes the model is drawn from, before the quarter turn it is drawn with
		axes = physics::QuarterTurnedBack(axes);
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
	// Things that lie over the land's shape have their points lifted with it
	if (registry.AllOf<MorphWithTerrain>(object))
	{
		if (const auto* land = Land())
		{
			const glm::vec2 base(transform->position.x, transform->position.z);
			const float baseHeight = land->HeightAt(base);
			const auto centreOfMass = shape.centreOfMass * scale;
			for (auto& point : shape.points)
			{
				const auto world = transform->position + axes * (point + centreOfMass);
				point.y += land->HeightAt(glm::vec2(world.x, world.z)) - baseHeight;
			}
		}
	}
	auto body = std::make_unique<physics::Body>(setup, shape);
	body->SetUpPose({.axes = axes * scale, .origin = transform->position});
	return body;
}

void DynamicsSystem::MakeSureEndSlotIsFree()
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
	if (!facts.canBecomePhysicsObject)
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
	MakeSureEndSlotIsFree();
	body->velocity = start.velocity;
	if (const float speed = glm::length(body->velocity); speed > physics::k_MaxSpeed)
	{
		body->velocity *= physics::k_MaxSpeed / speed;
	}
	body->SetAngularVelocityInBodyAxes(start.spin);
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
	MakeSureEndSlotIsFree();
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

PhysicsStarted DynamicsSystem::InitialisePhysics(entt::entity object, const PhysicsStart& start)
{
	return Hooks().InitialisePhysics(*this, object, start);
}

PhysicsStarted DynamicsSystem::ObjectInitialisePhysics(entt::entity object, const PhysicsStart& start)
{
	auto& registry = Entities();
	if (!IsAvailable(object) || registry.AllOf<InPhysics>(object))
	{
		return {};
	}
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
		Hooks().CheckAllCreaturesForCatching(object, *entry);
	}
	// A burning thing that starts to move leaves its fire's group
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().StartedMoving(object, false);
	}
	registry.SetDirty();
	return {.entry = entry, .started = true};
}

entt::entity DynamicsSystem::ObjectEndPhysics(entt::entity object, bool insert)
{
	auto& registry = Entities();
	if (!IsAvailable(object) || !registry.AllOf<InPhysics>(object))
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
		else
		{
			// It came to rest off the map
			world_objects::Remove(object);
			return entt::null;
		}
	}
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

void DynamicsSystem::AdjustToGroundLevel(PhysicsEntry& entry, bool noPullDown, bool alignToSlope)
{
	if (const auto* land = Land(); land != nullptr && entry.body != nullptr)
	{
		entry.body->AdjustToGroundLevel(*land, noPullDown, alignToSlope);
	}
}

void DynamicsSystem::RaiseUntilNotIntersecting(PhysicsEntry& entry)
{
	auto& registry = Entities();
	auto* body = entry.body.get();
	if (body == nullptr)
	{
		return;
	}
	// Resting bodies for what may be under it
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
	const bool villager = entry.kind == PhysicsEntry::Kind::Villager;
	bool raised = true;
	while (raised)
	{
		raised = false;
		for (auto& other : _entries)
		{
			if (other.get() == &entry || other->body == nullptr || other->entity == entry.thrower ||
			    !IsAvailable(other->entity) || !registry.AllOf<Mesh>(other->entity) || !Hooks().RaisesObjects(other->entity))
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

void DynamicsSystem::SetHitObject(entt::entity hit, entt::entity hitter)
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

void DynamicsSystem::GameTurnUpdate()
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
			// One of the five whooshes, picked by the clock's milliseconds
			const auto ticks =
			    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
			        .count();
			constexpr std::array<audio::SoundId, turn::k_Whooshes> k_RockPasts = {
			    audio::SoundId::G_RockPast_01, audio::SoundId::G_RockPast_02, audio::SoundId::G_RockPast_03,
			    audio::SoundId::G_RockPast_04, audio::SoundId::G_RockPast_05};
			Locator::audio::value().PlaySoundEffect(
			    static_cast<entt::id_type>(k_RockPasts.at(static_cast<size_t>(ticks % turn::k_Whooshes))), std::nullopt);
		}
		switch (result)
		{
		case physics::StepResult::Moved:
			SyncObject(entry, Place::Origin);
			break;
		case physics::StepResult::Stopped:
		{
			// It comes to rest: it stops breaking buildings, takes the body's place (a villager stands at its centre) and its
			// kind ends its physics
			const auto object = entry.entity;
			Hooks().ForgetBuildingHitter(object);
			SyncObject(entry, entry.kind == PhysicsEntry::Kind::Villager ? Place::Centre : Place::Origin);
			const auto kept = IsAvailable(object) ? Hooks().EndPhysics(*this, &entry, object, true) : entt::null;
			if (!IsAvailable(kept))
			{
				RemoveAt(i);
				continue;
			}
			if (kept != object && IsAvailable(object))
			{
				registry.Remove<PhysicsDrawPose>(object);
			}
			// What stays rests here as an obstacle, turned as the body is
			entry.entity = kept;
			TakeKind(entry, FactsOf(kept));
			SyncObject(entry, Place::Kept);
			body.resting = true;
			registry.Remove<PhysicsDrawPose>(kept);
			break;
		}
		case physics::StepResult::Knocked:
		{
			// A resting obstacle hit harder than it holds starts to move, if its kind lets it
			if (IsAvailable(entry.entity) && FactsOf(entry.entity).canBecomePhysicsObject &&
			    InitialisePhysics(entry.entity, {.add = false}).started)
			{
				body.resting = false;
			}
			break;
		}
		case physics::StepResult::Delete:
		{
			// Fallen deep under the sea
			const auto object = entry.entity;
			RemoveAt(i);
			if (IsAvailable(object))
			{
				world_objects::Remove(object);
			}
			continue;
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
		if (!IsAvailable(entry.entity))
		{
			continue;
		}
		// A felled tree makes one sound as it topples, if it is tall
		if (entry.kind == PhysicsEntry::Kind::FelledTree && body.Axes()[1].y < turn::k_FelledUpright)
		{
			if (world_objects::SizeOf(entry.entity).height > turn::k_FelledSoundHeight)
			{
				Hooks().FelledTreeToppled(entry.entity);
			}
			entry.kind = PhysicsEntry::Kind::FelledTreeToppled;
		}
		// A resting obstacle follows its object about
		if (body.resting)
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
		entry.hitBy = nullptr;
		if (body.lastHit != nullptr)
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
			SetHitObject(entry.entity, entry.hitBy->entity);
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
	const auto* info = world_objects::InfoOf(hitter);
	const float infoWeight = info != nullptr ? info->weight : entry.body->Mass();
	const auto level = turn::CollisionLevel(entry.impact, infoWeight, hitterType == SoundCollisionType::Grain);
	if (Locator::audio::has_value())
	{
		const auto keys = turn::CollisionKeys(level, static_cast<int32_t>(hitterType), static_cast<int32_t>(hitType));
		Locator::audio::value().PlayAnimEffect(std::string(k_CollisionBank), keys, hitter, centre);
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
	for (int32_t i = 0; i < turn::k_PuffsPerLanding && _dust.size() < turn::k_MostPuffs; ++i)
	{
		// Each axis of its speed is drawn z first, then y, then x
		const float z = speed();
		const float y = speed();
		const float x = speed();
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
		    .creator = &DustCreator(),
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
