/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "FireSystem.h"

#include <cmath>

#include <algorithm>
#include <unordered_map>

#include <LNDFile.h>
#include <entt/core/hashed_string.hpp>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Common/GUtilsDistance.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fire.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/MagicFireBall.h"
#include "ECS/Components/MagicForest.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/TempleExterior.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/VillagerDeath.h"
#include "ECS/Map.h"
#include "ECS/PosedModel.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "FileSystem/FileSystemInterface.h"
#include "Fire/FireGraphic.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Particles/ParticleDrawFrame.h"
#include "Particles/ParticleMaths.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerFire.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace world_objects = openblack::ecs::world_objects;

namespace
{
/// The flames' and puffs' sheets, and the light a burning building casts
constexpr std::string_view k_FlameTexture = "S_Fire";
constexpr std::string_view k_PuffTexture = "S_SpriteSheet3";
constexpr entt::hashed_string k_FlameSheet = entt::hashed_string("raw/S_Fire");
constexpr entt::hashed_string k_FlameSheetAlpha = entt::hashed_string("raw/S_Firea");
constexpr entt::hashed_string k_PuffSheet = entt::hashed_string("raw/S_SpriteSheet3");
constexpr entt::hashed_string k_PuffSheetAlpha = entt::hashed_string("raw/S_SpriteSheet3a");
constexpr std::string_view k_LightMapFile = "Spells/LightMaps/S_LMFireBall.raw";
constexpr int k_LightMapPitch = 6;
constexpr int k_LightMapChannels = 3;
/// A burning building this wide or wider lights the land
constexpr float k_LightingRadius = 2.0f;
/// The light lies this far across the land from what burns
constexpr glm::vec3 k_LightMapOffset {10.0f, 0.0f, 10.0f};
/// Its flicker moves on so fast with the turns
constexpr float k_FlickerRate = 0.6f;
/// A flame's sprite is twice as tall as it is wide, its base on its point
constexpr float k_FlameStretch = 2.0f;
/// The land's cells are this wide
constexpr float k_CellSize = 10.0f;
/// The spiral round a fire stops after this many cells at most
constexpr int k_MostSpiralSteps = 99999;
/// A fireball's sprite is this big for its miracle's magnitude; its heat capacity grows by its radius squared times this
constexpr float k_FireBallCapacityScale = 0.0625f;
/// The weather's wind bytes are this many metres a second
constexpr float k_WindByteSpeed = 0.125f;
/// A dead tree's fire reaches this share of its height
constexpr float k_DeadTreeFireRadius = 0.35f;

ecs::Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}

float LandHeight(glm::vec2 xz)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

/// What an object is made of for burning, none for what can't burn
std::optional<fire::Material> MaterialOf(entt::entity object)
{
	auto& registry = Entities();
	if (!registry.Valid(object) || !Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	if (const auto* ball = registry.TryGet<const MagicFireBall>(object))
	{
		// Every fireball burns as the first row of the fireball's table, whatever its power-up: the game picks the row
		// by a power-up type no fireball has
		const auto& info = Locator::infoConstants::value().magicFireBall.at(0);
		return fire::Material {
		    .combustion = info.combustionTemperature,
		    .capacity = ball->radius * ball->radius * k_FireBallCapacityScale * info.heatCapacity * ball->strength,
		    .defence = 0.0f,
		    .burningPriority = info.burningPriority,
		    .rainCooling = ball->affectedByRain ? fire::k_RainCooling : 0.0f,
		    .radius = ball->radius,
		    .height = ball->radius,
		    .fireRadius = ball->radius,
		};
	}
	const auto* info = world_objects::InfoOf(object);
	if (info == nullptr)
	{
		return std::nullopt;
	}
	const auto size = world_objects::SizeOf(object);
	// A dead tree's fire reaches by a share of its height rather than by its width
	const float fireRadius = registry.AllOf<DeadTree>(object) ? k_DeadTreeFireRadius * size.height : size.radius;
	return fire::Material {
	    .combustion = info->combustionTemperature,
	    .capacity = info->heatCapacity,
	    .defence = info->defenceMultiplierBurn,
	    .burningPriority = info->burningPriority,
	    .rainCooling = fire::k_RainCooling,
	    .radius = size.radius,
	    .height = size.height,
	    .fireRadius = fireRadius,
	};
}

/// Where a fire burns: across the land, and its height above it
glm::vec3 FireCentre(entt::entity object)
{
	const auto* transform = Entities().TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return glm::vec3(0.0f);
	}
	const auto& p = transform->position;
	return {p.x, p.y - LandHeight({p.x, p.z}), p.z};
}

float AcrossGround(const glm::vec3& a, const glm::vec3& b)
{
	return glm::distance(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z));
}

/// The land's cell under a point, none off the map
const lnd::LNDCell* CellAt(const glm::vec3& point)
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return nullptr;
	}
	return Locator::terrainSystem::value().FindCell(glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize)));
}

/// Whether something is in a hand: it neither joins a blaze it heats nor spreads its own fire about the land
bool IsHeld(entt::entity object)
{
	return Entities().AllOf<InHand>(object);
}

/// Whether an object is on the map: a fireball never is, nor is something held, flung through the air or carried by a
/// tornado, all of which are out of the map's cells
bool IsInMap(entt::entity object)
{
	const auto& registry = Entities();
	return !registry.AnyOf<MagicFireBall, InHand, InPhysics, CarriedByTornado>(object);
}

/// Whether an object takes a burn at all: a pot only with something in it, a field only with food to burn
bool TakesBurn(entt::entity object, float burn)
{
	auto& registry = Entities();
	if (const auto* pot = registry.TryGet<const Pot>(object))
	{
		return pot->amount != 0;
	}
	if (const auto* field = registry.TryGet<const Field>(object))
	{
		return burn <= 0.0f || field->crop.food > 0.0f;
	}
	return true;
}

/// What stands fixed over the land's cells, as the game builds its buildings, features, flowers, forests, rocks and the
/// like: the objects whose fire may light the land. A field is one too, but its fire never lights the land.
bool LightsLandWhenBurning(entt::entity object)
{
	const auto& registry = Entities();
	return !registry.AllOf<Field>(object) && registry.AnyOf<Abode, StoragePit, SpellDispenser, Feature, AnimatedStatic, Flowers,
	                                                        BigForest, MobileStatic, TeleportStone, TempleExterior>(object);
}

/// What an object's flames are sized for
fire::graphic::FlameShape FlameShapeOf(entt::entity object)
{
	const auto& registry = Entities();
	if (registry.AnyOf<Tree, DeadTree>(object))
	{
		return fire::graphic::FlameShape::Tree;
	}
	// A creature's body and the temple are models of many jointed parts; a villager's or an animal's isn't
	if (registry.AnyOf<Creature, Temple>(object))
	{
		return fire::graphic::FlameShape::Jointed;
	}
	return fire::graphic::FlameShape::Plain;
}

bool IsTree(entt::entity object)
{
	return Entities().AnyOf<Tree, DeadTree>(object);
}

const graphics::L3DMesh* MeshOf(entt::entity object)
{
	const auto* mesh = Entities().TryGet<const Mesh>(object);
	if (mesh == nullptr || !Locator::resources::has_value())
	{
		return nullptr;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(mesh->id) ? &*meshes.Handle(mesh->id) : nullptr;
}

/// A random point of a random triangle of the model's first detail, in its own frame; a random bone's place, as the
/// bones are posed now, for a model moved by bones. A tree's flames keep to its middle half.
std::optional<glm::vec3> RandomPointOn(entt::entity object)
{
	const auto* mesh = MeshOf(object);
	if (mesh == nullptr || !Locator::gameRandom::has_value())
	{
		return std::nullopt;
	}
	auto& random = Locator::gameRandom::value();
	if (mesh->IsBoned() && !mesh->GetBoneMatrices().empty())
	{
		const auto bones = ecs::posed_model::BonesOf(Entities(), object, *mesh);
		const auto bone = random.LocalRand(static_cast<int32_t>(bones.size()));
		return glm::vec3(bones[std::min<size_t>(static_cast<size_t>(bone), bones.size() - 1)][3]);
	}
	size_t triangles = 0;
	const auto firstDetail = [](const graphics::L3DSubMesh& subMesh) { return (subMesh.GetFlags().lodMask & 1u) != 0; };
	for (const auto& subMesh : mesh->GetSubMeshes())
	{
		if (firstDetail(*subMesh))
		{
			triangles += subMesh->GetSurface().indices.size() / 3;
		}
	}
	if (triangles == 0)
	{
		return mesh->GetBoundingBox().Center();
	}
	auto pick = static_cast<size_t>(random.LocalRand(static_cast<int32_t>(triangles)));
	float a = random.LocalFloatRand(1.0f);
	float b = random.LocalFloatRand(1.0f);
	if (a + b > 1.0f)
	{
		a = 1.0f - a;
		b = 1.0f - b;
	}
	for (const auto& subMesh : mesh->GetSubMeshes())
	{
		if (!firstDetail(*subMesh))
		{
			continue;
		}
		const auto& surface = subMesh->GetSurface();
		const size_t count = surface.indices.size() / 3;
		if (pick >= count)
		{
			pick -= count;
			continue;
		}
		const auto corner = [&](size_t i) { return surface.positions.at(surface.indices.at(pick * 3 + i)); };
		glm::vec3 point = corner(0) + (corner(1) - corner(0)) * b + (corner(2) - corner(0)) * a;
		if (IsTree(object))
		{
			point *= 0.5f;
		}
		return point;
	}
	return std::nullopt;
}

glm::vec3 ToWorld(entt::entity object, const glm::vec3& local)
{
	const auto* transform = Entities().TryGet<const Transform>(object);
	return transform != nullptr ? transform->position + transform->rotation * (transform->scale * local) : local;
}

/// The creators the fires' sprites are drawn with
struct FireCreators
{
	particles::Creator flame;
	particles::Creator steam;
	particles::Creator smoke;

	FireCreators()
	{
		const auto sheet = [](particles::Creator& creator, std::string_view texture, bool additive) {
			creator.kind = particles::Creator::Kind::Sprite;
			creator.className = "Fire";
			creator.texture = std::string(texture);
			creator.spritesPerRow = 8;
			creator.numFrames = 64;
			creator.loopAnim = true;
			creator.additive = additive;
			creator.writeDepth = false;
		};
		sheet(flame, k_FlameTexture, true);
		// The flame stands on its point, and so do the puffs: the game draws them with the origin the flames left set
		flame.centreAtBase = true;
		sheet(steam, k_PuffTexture, true);
		steam.centreAtBase = true;
		sheet(smoke, k_PuffTexture, false);
		smoke.centreAtBase = true;
	}
};

const FireCreators& Creators()
{
	static const FireCreators creators;
	return creators;
}

std::array<uint8_t, 3> Rgb(uint32_t colour)
{
	return {static_cast<uint8_t>((colour >> 16u) & 0xFFu), static_cast<uint8_t>((colour >> 8u) & 0xFFu),
	        static_cast<uint8_t>(colour & 0xFFu)};
}

/// The fires' light map, loaded once
std::optional<entt::id_type> FireLightMap()
{
	if (!Locator::resources::has_value() || !Locator::filesystem::has_value())
	{
		return std::nullopt;
	}
	const auto id = entt::hashed_string("fire/lightmap").value();
	auto& bitmaps = Locator::resources::value().GetParticleBitmaps();
	if (bitmaps.Contains(id))
	{
		return id;
	}
	auto& fileSystem = Locator::filesystem::value();
	try
	{
		bitmaps.Load(id, resources::ParticleBitmapLoader::FromDiskTag {},
		             fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / k_LightMapFile),
		             resources::ParticleBitmapLoader::Layout {
		                 .pitch = k_LightMapPitch, .channels = k_LightMapChannels, .framesInFile = 1, .framesInUse = 1});
	}
	catch (const std::exception&)
	{
		return std::nullopt;
	}
	return id;
}

const particles::maths::ValueNoise& Noise()
{
	static const particles::maths::ValueNoise noise;
	return noise;
}
} // namespace

FireSystem::FireSystem() = default;
FireSystem::~FireSystem() = default;

Fire* FireSystem::FindOrCreate(entt::entity object, std::optional<PlayerNames> player, entt::entity source)
{
	auto& registry = Entities();
	if (!registry.Valid(object))
	{
		return nullptr;
	}
	if (auto* existing = registry.TryGet<Fire>(object))
	{
		return existing;
	}
	if (const auto* proofing = registry.TryGet<const FireProofing>(object); proofing != nullptr && proofing->cannotBeSetOnFire)
	{
		return nullptr;
	}
	// What can burn: what has a combustion temperature in its table and takes the burn of a fire. A villager doesn't
	// while it is dying, at home or hiding in a building.
	constexpr float k_FireBurn = 100.0f;
	const auto material = MaterialOf(object);
	if (!material.has_value() || material->combustion == 0.0f || !TakesBurn(object, k_FireBurn) ||
	    (world_objects::IsVillager(object) && !villager_fire::CanCatchFire(object)))
	{
		return nullptr;
	}
	auto& fire = registry.Assign<Fire>(object);
	fire.source = source;
	fire.hasPlayer = player.has_value();
	fire.player = player.value_or(PlayerNames::NEUTRAL);
	fire.root = object;
	fire.createdTurn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
	fire.waits = _burnPointTurn == fire.createdTurn;
	registry.Assign<FireGroup>(object, FireGroup {.members = {object}, .firemen = {}});
	_fires.insert(_fires.begin(), object);

	// Its looks: flames, smoke and steam where it has a model to put them on. A fireball's fire shows only steam, with
	// no flames, smoke or light of its own: having no model it puffs none, but it hisses as the steam starts, as when
	// the rain cools it
	if (registry.AllOf<MagicFireBall>(object))
	{
		FireLook look;
		look.graphic.kinds = {.flames = false, .smoke = false, .steam = true, .lightMap = false, .followsLand = false};
		registry.Assign<FireLook>(object, std::move(look));
	}
	else if (MeshOf(object) != nullptr)
	{
		const bool tree = IsTree(object);
		FireLook look;
		look.graphic.kinds.followsLand = registry.AllOf<MorphWithTerrain>(object);
		look.graphic.maxFlames = fire::graphic::MaxFlames(tree, material->radius, material->height);
		look.graphic.localScale = fire::graphic::LocalFlameScale(FlameShapeOf(object), material->height);
		look.lightsLand = LightsLandWhenBurning(object) && material->radius > k_LightingRadius;
		registry.Assign<FireLook>(object, std::move(look));
	}
	// What the living thought of it before it burned is forgotten while it burns; a magic tree stops drawing its
	// caster's people to look at it
	if (registry.AnyOf<Abode, Feature, Field, BigForest, MobileStatic, AnimatedStatic, DeadTree, Pot>(object) &&
	    Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(object);
	}
	if (registry.AllOf<MagicTree>(object) && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(object, Reaction::ReactToMagicTree);
	}
	registry.SetDirty();
	return &fire;
}

void FireSystem::ApplyBurn(entt::entity object, float burn, std::optional<PlayerNames> player)
{
	if (burn == 0.0f || !Entities().Valid(object) || !TakesBurn(object, burn))
	{
		return;
	}
	// Water and beating only cool what is already hot
	auto* fire = burn > 0.0f ? FindOrCreate(object, player, entt::null) : Entities().TryGet<Fire>(object);
	if (fire != nullptr)
	{
		if (const auto material = MaterialOf(object))
		{
			fire->state.temperature = fire::ApplyBurn(fire->state.temperature, *material, burn);
		}
	}
	// A villager it reaches with a fire on it runs about, hot or being cooled
	if (fire != nullptr && world_objects::IsVillager(object) && !villager_fire::IsRunningOnFire(object))
	{
		villager_fire::SetupOnFire(object, entt::null);
	}
}

void FireSystem::SetTemperature(entt::entity object, float temperature, entt::entity source)
{
	auto& registry = Entities();
	auto* fire = registry.TryGet<Fire>(object);
	if (fire == nullptr)
	{
		// Only something made hotter than it is catches
		if (!(temperature > fire::k_AmbientTemperature))
		{
			return;
		}
		// The fire is the doing of the object's own player: a fireball's is its caster's
		fire = FindOrCreate(object, world_objects::PlayerOf(object), source);
		if (fire == nullptr)
		{
			return;
		}
	}
	fire->state.temperature = temperature;
}

void FireSystem::SetOnFire(entt::entity object, float speed)
{
	if (const auto material = MaterialOf(object))
	{
		SetTemperature(object, fire::SetOnFireTemperature(*material, speed), entt::null);
	}
}

void FireSystem::PutOut(entt::entity object)
{
	if (auto* fire = Entities().TryGet<Fire>(object))
	{
		fire->state.temperature = fire::k_AmbientTemperature;
	}
}

void FireSystem::HeatHeldObject(entt::entity object)
{
	// Every fire in the held object's cell heats it, as a tree held over a bonfire catches
	auto& registry = Entities();
	const auto* transform = registry.TryGet<const Transform>(object);
	if (transform == nullptr || !Locator::entitiesMap::has_value())
	{
		return;
	}
	const auto& map = Locator::entitiesMap::value();
	for (const auto& cell : {map.GetFixedInGridCell(transform->position), map.GetMobileInGridCell(transform->position)})
	{
		for (const auto other : cell)
		{
			if (other != object && registry.Valid(other) && registry.AllOf<Fire>(other))
			{
				HeatTransfer(other, object);
			}
		}
	}
}

void FireSystem::StartedMoving(entt::entity object, bool inHand)
{
	auto& registry = Entities();
	auto* fire = registry.TryGet<Fire>(object);
	if (fire == nullptr)
	{
		return;
	}
	LeaveBlaze(object);
	if (fire->reaction != 0 && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Remove(fire->reaction);
		fire->reaction = 0;
	}
	// A burning thing in the hand alarms the people round it
	const auto* transform = registry.TryGet<const Transform>(object);
	if (inHand && transform != nullptr && Locator::reactionSystem::has_value() &&
	    fire::IsAboveReactionTemperature(fire->state.temperature, MaterialOf(object).value_or(fire::Material {})))
	{
		fire->reaction = Locator::reactionSystem::value().Create({.initiator = object,
		                                                          .type = Reaction::ReactToBurningObjectInHand,
		                                                          .player = fire->player,
		                                                          .position = transform->position});
	}
}

void FireSystem::SetCanBeSetOnFire(entt::entity object, bool can)
{
	auto& registry = Entities();
	if (registry.Valid(object))
	{
		(registry.AllOf<FireProofing>(object) ? registry.Get<FireProofing>(object) : registry.Assign<FireProofing>(object))
		    .cannotBeSetOnFire = !can;
	}
}

void FireSystem::SetHurtByFire(entt::entity object, bool hurt)
{
	auto& registry = Entities();
	if (registry.Valid(object))
	{
		(registry.AllOf<FireProofing>(object) ? registry.Get<FireProofing>(object) : registry.Assign<FireProofing>(object))
		    .notHurtByFire = !hurt;
	}
}

float FireSystem::GetTemperature(entt::entity object) const
{
	const auto* fire = Entities().TryGet<const Fire>(object);
	return fire != nullptr ? fire->state.temperature : fire::k_AmbientTemperature;
}

bool FireSystem::IsOnFire(entt::entity object) const
{
	const auto* fire = Entities().TryGet<const Fire>(object);
	if (fire == nullptr)
	{
		return false;
	}
	const auto material = MaterialOf(object);
	return material.has_value() && fire::IsOnFire(fire->state.temperature, *material);
}

bool FireSystem::IsFireNear(const glm::vec3& point, float radius) const
{
	if (!Locator::entitiesMap::has_value())
	{
		return false;
	}
	const auto& registry = Entities();
	const auto& map = Locator::entitiesMap::value();
	// The objects fixed to and moving over the land's cells that the radius's square round the point touches, as a map
	// position finds them
	const auto coords = map_coords::FromMetres({point.x, point.z});
	const auto low = [radius](int32_t fixed) { return map_coords::ToFixed(map_coords::ToMetres(fixed) - radius) >> 16; };
	const auto high = [radius](int32_t fixed) { return map_coords::ToFixed(map_coords::ToMetres(fixed) + radius) >> 16; };
	const auto burningInReach = [&](entt::entity object) {
		const auto* transform = registry.TryGet<const Transform>(object);
		return transform != nullptr && IsOnFire(object) &&
		       gutils::GetDistanceInMetres(coords, map_coords::FromMetres({transform->position.x, transform->position.z})) <=
		           radius;
	};
	for (int32_t x = low(coords.x); x <= high(coords.x); ++x)
	{
		for (int32_t z = low(coords.z); z <= high(coords.z); ++z)
		{
			if (!map_coords::InBounds(glm::ivec2(x, z)))
			{
				continue;
			}
			const auto cell = ecs::MapInterface::CellId(x, z);
			if (std::ranges::any_of(map.GetFixedInGridCell(cell), burningInReach) ||
			    std::ranges::any_of(map.GetMobileInGridCell(cell), burningInReach))
			{
				return true;
			}
		}
	}
	return false;
}

float FireSystem::GetCharring(entt::entity object) const
{
	const auto* fire = Entities().TryGet<const Fire>(object);
	return fire != nullptr ? fire->state.charring : 0.0f;
}

std::optional<FireSystemInterface::Blaze> FireSystem::GetBlaze(entt::entity object) const
{
	const auto& registry = Entities();
	const auto* fire = registry.TryGet<const Fire>(object);
	if (fire == nullptr || !registry.Valid(fire->root))
	{
		return std::nullopt;
	}
	const auto* group = registry.TryGet<const FireGroup>(fire->root);
	if (group == nullptr)
	{
		return std::nullopt;
	}
	Blaze blaze {.root = fire->root, .burningRadius = 0.0f, .burningPriority = 0.0f, .firemen = group->firemen.size()};
	// The radius of only its burning fires, but the most urgent of all its fires, burning or not
	for (const auto member : group->members)
	{
		const auto* memberFire = registry.TryGet<const Fire>(member);
		const auto material = MaterialOf(member);
		if (memberFire == nullptr || !material.has_value())
		{
			continue;
		}
		blaze.burningPriority = std::max(blaze.burningPriority, material->burningPriority);
		if (fire::IsOnFire(memberFire->state.temperature, *material))
		{
			blaze.burningRadius += material->radius;
		}
	}
	return blaze;
}

std::optional<entt::entity> FireSystem::NearestHotMember(entt::entity root, const glm::vec3& point) const
{
	const auto& registry = Entities();
	const auto* group = registry.TryGet<const FireGroup>(root);
	if (group == nullptr)
	{
		return std::nullopt;
	}
	std::optional<entt::entity> nearest;
	float best = 0.0f;
	for (const auto member : group->members)
	{
		const auto reach = GetReach(member);
		if (!reach.has_value() || !reach->aboveReactionTemperature)
		{
			continue;
		}
		const float distance = AcrossGround(reach->centre, point);
		if (!nearest.has_value() || distance < best)
		{
			nearest = member;
			best = distance;
		}
	}
	return nearest;
}

std::optional<FireSystemInterface::Reach> FireSystem::GetReach(entt::entity object) const
{
	const auto& registry = Entities();
	const auto* fire = registry.TryGet<const Fire>(object);
	const auto* transform = registry.TryGet<const Transform>(object);
	const auto material = MaterialOf(object);
	if (fire == nullptr || transform == nullptr || !material.has_value())
	{
		return std::nullopt;
	}
	const float fraction = fire::FireFraction(fire->state.temperature, *material, world_objects::LifeOf(object));
	const float radius = fire::FireRadius(*material, fraction);
	const float maxRadius = fire::MaxFireRadius(*material);
	return Reach {
	    .position = transform->position,
	    .centre = transform->position,
	    .defaultRadius = material->fireRadius,
	    .radius = radius,
	    .maxRadius = maxRadius,
	    .safeRadius = fire::SafeFireRadius(radius, maxRadius),
	    .aboveReactionTemperature = fire::IsAboveReactionTemperature(fire->state.temperature, *material),
	    .burning = fire::IsOnFire(fire->state.temperature, *material),
	};
}

namespace
{
/// The group of an object's blaze, kept with its first fire
FireGroup* GroupOf(entt::entity object)
{
	auto& registry = Entities();
	const auto* fire = registry.Valid(object) ? registry.TryGet<const Fire>(object) : nullptr;
	if (fire == nullptr || !registry.Valid(fire->root))
	{
		return nullptr;
	}
	return registry.TryGet<FireGroup>(fire->root);
}
} // namespace

std::vector<entt::entity> FireSystem::GetBlazeMembers(entt::entity object) const
{
	const auto* group = GroupOf(object);
	return group != nullptr ? group->members : std::vector<entt::entity> {};
}

std::optional<entt::entity> FireSystem::NearestSafeFire(entt::entity object, const glm::vec3& point) const
{
	constexpr float k_FarAway = 1.0e7f;
	std::optional<entt::entity> nearest;
	float best = k_FarAway;
	for (const auto member : GetBlazeMembers(object))
	{
		const auto reach = GetReach(member);
		if (!reach.has_value())
		{
			continue;
		}
		const float radius = reach->safeRadius < reach->defaultRadius ? reach->defaultRadius : reach->safeRadius;
		const float distance = gutils::GetDistanceInMetres(point, reach->centre) - radius;
		if (distance < best && reach->aboveReactionTemperature)
		{
			best = distance;
			nearest = member;
		}
	}
	return nearest;
}

bool FireSystem::InSameBlaze(entt::entity a, entt::entity b) const
{
	const auto& registry = Entities();
	const auto* fireA = registry.Valid(a) ? registry.TryGet<const Fire>(a) : nullptr;
	const auto* fireB = registry.Valid(b) ? registry.TryGet<const Fire>(b) : nullptr;
	return fireA != nullptr && fireB != nullptr && fireA->root == fireB->root;
}

void FireSystem::MergeBlazes(entt::entity object, entt::entity other)
{
	const auto& registry = Entities();
	if (registry.Valid(object) && registry.Valid(other) && registry.AllOf<Fire>(object) && registry.AllOf<Fire>(other))
	{
		JoinBlaze(object, other);
	}
}

void FireSystem::MoveFire(entt::entity from, entt::entity to)
{
	auto& registry = Entities();
	if (!registry.Valid(from) || !registry.Valid(to) || registry.AllOf<Fire>(to))
	{
		return;
	}
	const auto* old = registry.TryGet<const Fire>(from);
	if (old == nullptr)
	{
		return;
	}
	auto moved = *old;
	// Its place in its blaze: first of it, the blaze goes with it; else it stands in the same place among the others
	if (moved.root == from)
	{
		moved.root = to;
		if (auto* group = registry.TryGet<FireGroup>(from))
		{
			auto blaze = std::move(*group);
			std::ranges::replace(blaze.members, from, to);
			for (const auto member : blaze.members)
			{
				if (auto* memberFire = member != to ? registry.TryGet<Fire>(member) : nullptr)
				{
					memberFire->root = to;
				}
			}
			registry.Assign<FireGroup>(to, std::move(blaze));
		}
	}
	else if (auto* blaze = registry.Valid(moved.root) ? registry.TryGet<FireGroup>(moved.root) : nullptr)
	{
		std::ranges::replace(blaze->members, from, to);
	}
	registry.Assign<Fire>(to, moved);
	if (auto* look = registry.TryGet<FireLook>(from))
	{
		registry.Assign<FireLook>(to, std::move(*look));
	}
	std::ranges::replace(_fires, from, to);
	// The alarm it raises now comes from what it became
	if (moved.reaction != 0 && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().SetInitiator(moved.reaction, to);
	}
	// Its crackle carries on from what it became
	for (auto& slot : _soundSlots)
	{
		if (slot.fire == from)
		{
			slot.fire = to;
		}
	}
	registry.Remove<FireGroup>(from);
	registry.Remove<FireLook>(from);
	registry.Remove<Fire>(from);
	registry.SetDirty();
}

void FireSystem::CopyFire(entt::entity from, entt::entity to)
{
	auto& registry = Entities();
	if (!registry.Valid(from) || !registry.Valid(to))
	{
		return;
	}
	const auto* source = registry.TryGet<const Fire>(from);
	if (source == nullptr)
	{
		return;
	}
	const auto player = source->hasPlayer ? std::optional(source->player) : std::nullopt;
	const auto temperature = source->state.temperature;
	auto* fire = FindOrCreate(to, player, source->source);
	if (fire == nullptr)
	{
		return;
	}
	JoinBlaze(from, to);
	// It is made as hot as the first, if it was cooler
	fire = &registry.Get<Fire>(to);
	if (fire->state.temperature < temperature)
	{
		fire->state.previous = temperature;
		fire->state.temperature = temperature;
	}
	else
	{
		fire->state.previous = fire->state.temperature;
	}
}

void FireSystem::AddFireman(entt::entity object, entt::entity villager)
{
	if (auto* group = GroupOf(object))
	{
		group->firemen.insert(group->firemen.begin(), villager);
	}
}

void FireSystem::RemoveFireman(entt::entity object, entt::entity villager)
{
	if (auto* group = GroupOf(object))
	{
		std::erase(group->firemen, villager);
	}
}

bool FireSystem::IsFiremanOf(entt::entity object, entt::entity villager) const
{
	const auto* group = GroupOf(object);
	return group != nullptr && std::ranges::find(group->firemen, villager) != group->firemen.end();
}

void FireSystem::ProcessTurn()
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Entities();
	// The camera's distance to the crackling fires changes as it moves
	if (Locator::camera::has_value())
	{
		const auto eye = Locator::camera::value().GetOrigin();
		_farthestSlot = 0.0f;
		for (auto& slot : _soundSlots)
		{
			if (slot.fire != entt::null)
			{
				if (const auto* transform = registry.TryGet<const Transform>(slot.fire))
				{
					slot.distance = gutils::GetDistanceInMetres(eye, transform->position);
				}
				_farthestSlot = std::max(_farthestSlot, slot.distance);
			}
		}
	}
	PruneBlazes();
	const uint32_t turn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
	// The fires made this turn after the burn point wait; those made during this walk are newer still and aren't in the
	// copy walked. One the living lit earlier in the turn burns at once.
	const auto fires = _fires;
	for (const auto object : fires)
	{
		if (!registry.Valid(object) || !registry.AllOf<Fire>(object))
		{
			std::erase(_fires, object);
			continue;
		}
		if (const auto& made = registry.Get<Fire>(object); made.waits && made.createdTurn == turn)
		{
			continue;
		}
		// A fireball whose particle no longer carries it goes
		if (const auto* ball = registry.TryGet<const MagicFireBall>(object); ball != nullptr && turn > ball->lastTurn + 1)
		{
			Delete(object);
			continue;
		}
		Process(object);
	}
	PlaySounds();
}

void FireSystem::MarkBurnPoint()
{
	_burnPointTurn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
}

void FireSystem::Process(entt::entity object)
{
	auto& registry = Entities();
	const auto material = MaterialOf(object);
	if (!material.has_value())
	{
		Delete(object);
		return;
	}
	auto& fire = registry.Get<Fire>(object);
	const float lifeBefore = world_objects::LifeOf(object);
	const auto centre = FireCentre(object);
	const auto* transform = registry.TryGet<const Transform>(object);
	const glm::vec3 position = transform != nullptr ? transform->position : glm::vec3(0.0f);

	fire::Surroundings surroundings {.life = lifeBefore};
	if (const auto* cell = CellAt(position);
	    cell == nullptr || cell->properties.hasWater != 0 || cell->properties.coastLine != 0)
	{
		surroundings.inWater = centre.y < fire::k_InWaterHeight;
	}
	if (Locator::weatherSystem::has_value())
	{
		const auto weather = Locator::weatherSystem::value().GetWeather(position);
		surroundings.rain = static_cast<float>(std::max(weather.rain, weather.snow));
	}
	if (fire.source != entt::null && !registry.Valid(fire.source))
	{
		fire.source = entt::null;
	}
	const auto outcome = fire::Step(fire.state, *material, surroundings);
	if (outcome.gone)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Fire: object {} has cooled", entt::to_integral(object));
		Delete(object);
		return;
	}
	if ((fire.state.flags & fire::k_JustIgnited) != 0)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Fire: object {} caught at {:.0f}", entt::to_integral(object),
		                    fire.state.temperature);
	}
	if ((fire.state.flags & fire::k_JustExtinguished) != 0)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Fire: object {} went out", entt::to_integral(object));
	}
	// TODO(blastfire): a creature that catches is burnt on its skin in three tries, each a ray from a random point
	// round its centre bone at the body posed by its bones: type 0 two times in three, else 6, size GameRand(8). The ray
	// onto the posed skin exists (posed_model::NearestSkinHit); this catching burn's own rules aren't ported yet.
	if (outcome.rainedOn && Locator::magicSystem::has_value())
	{
		// The people come to watch a storm miracle's rain put the fire out
		Locator::magicSystem::value().RainOnFire(position);
	}
	if (outcome.damage > 0.0f)
	{
		// A field loses its food to the fire even when a script keeps fire from hurting it, and its town isn't attacked
		const auto* proofing = registry.TryGet<const FireProofing>(object);
		const bool field = registry.AllOf<Field>(object);
		if (field || proofing == nullptr || !proofing->notHurtByFire)
		{
			const float life = world_objects::ReduceLife(object, outcome.damage);
			// Burning a town's people or buildings is an attack on the town by whoever lit the fire, by nobody's when
			// no one did
			const auto& lit = registry.Get<const Fire>(object);
			if (!field)
			{
				world_objects::AttackTown(object, outcome.damage, lit.hasPlayer ? lit.player : PlayerNames::NEUTRAL);
			}
			if (life <= 0.0f && !world_objects::IsCreature(object))
			{
				// Burnt down: a building stands with no life, a villager dies, anything else goes
				SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Fire: object {} burnt down", entt::to_integral(object));
				// A villager burnt to death is put down to whoever lit the fire, weighing nothing with its town
				world_objects::DestroyedByEffect(object, world_objects::EffectDeath {
				                                             .killer = lit.hasPlayer ? std::optional(lit.player) : std::nullopt,
				                                             .weight = 0.0f,
				                                         });
				if (!registry.Valid(object) || !registry.AllOf<Fire>(object))
				{
					std::erase(_fires, object);
					StopSound(object);
					return;
				}
			}
		}
	}
	auto& burning = registry.Get<Fire>(object);
	const float life = world_objects::LifeOf(object);
	const float fraction = fire::FireFraction(burning.state.temperature, *material, life);
	if (map_coords::InBounds(position) && !IsHeld(object))
	{
		Spread(object, fire::FireRadius(*material, fraction));
		if (!registry.Valid(object) || !registry.AllOf<Fire>(object))
		{
			return;
		}
	}
	auto& after = registry.Get<Fire>(object);
	const bool hot = fire::IsAboveReactionTemperature(after.state.temperature, *material);
	// A first fire that no longer burns hands its blaze to the first that does
	if (after.root == object && !hot)
	{
		if (const auto* group = registry.TryGet<const FireGroup>(object); group != nullptr && group->members.size() > 1)
		{
			const auto members = group->members;
			for (const auto member : members)
			{
				const auto* memberFire = registry.TryGet<const Fire>(member);
				const auto memberMaterial = MaterialOf(member);
				if (member != object && memberFire != nullptr && memberMaterial.has_value() &&
				    fire::IsAboveReactionTemperature(memberFire->state.temperature, *memberMaterial))
				{
					HandOverBlaze(object, member);
					break;
				}
			}
		}
	}
	// The living react to something hot; not to a burning villager, which runs about itself
	if (Locator::reactionSystem::has_value() && !IsHeld(object))
	{
		// One reaction for as long as it stays hot, never made again should it end sooner; once it cools, every fire
		// reaction it started goes, and any to it burning in the hand
		auto& reactions = Locator::reactionSystem::value();
		auto& current = registry.Get<Fire>(object);
		if (current.reaction == 0 && hot && !world_objects::IsVillager(object))
		{
			current.reaction = reactions.Create(
			    {.initiator = object, .type = Reaction::ReactToFire, .player = current.player, .position = position});
		}
		else if (current.reaction != 0 && !hot)
		{
			current.reaction = 0;
			reactions.RemoveFrom(object, Reaction::ReactToFire);
			reactions.RemoveFrom(object, Reaction::ReactToBurningObjectInHand);
		}
	}
	ConsiderSound(object, fraction);
	registry.Get<Fire>(object).state.previous = registry.Get<Fire>(object).state.temperature;
}

void FireSystem::Spread(entt::entity object, float radius)
{
	if (!Locator::entitiesMap::has_value())
	{
		return;
	}
	auto& registry = Entities();
	const auto& map = Locator::entitiesMap::value();
	const auto* transform = registry.TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return;
	}
	// Every object in the cells round the fire within its reach, in a spiral out from its own cell. One spanning several
	// cells is heated once for each.
	// The spiral stops at the first cell whose step from the fire's own map position, measured as the game measures
	// distances, is beyond the reach.
	const float reach = radius + fire::k_SpreadMargin;
	const auto start = map_coords::FromMetres({transform->position.x, transform->position.z});
	auto coords = start;
	map_coords::Spiral spiral;
	std::vector<entt::entity> objects;
	for (int step = 0; step < k_MostSpiralSteps; ++step)
	{
		if (gutils::GetDistanceInMetres(start, coords) > reach)
		{
			break;
		}
		const glm::ivec2 cell = map_coords::Cell(coords);
		if (map_coords::InBounds(cell))
		{
			const auto id = ecs::MapInterface::CellId(cell);
			objects.clear();
			const auto& fixed = map.GetFixedInGridCell(id);
			objects.insert(objects.end(), fixed.begin(), fixed.end());
			const auto& mobile = map.GetMobileInGridCell(id);
			objects.insert(objects.end(), mobile.begin(), mobile.end());
			for (const auto target : objects)
			{
				if (target == object || !registry.Valid(target))
				{
					continue;
				}
				HeatTransfer(object, target);
				if (!registry.Valid(object) || !registry.AllOf<Fire>(object))
				{
					return;
				}
			}
		}
		map_coords::AddCells(coords, spiral.Next());
	}
}

void FireSystem::HeatTransfer(entt::entity source, entt::entity target)
{
	auto& registry = Entities();
	auto& fire = registry.Get<Fire>(source);
	// The villagers fighting a fire don't feel its heat while what burns is on the map; a fireball never is, nor is
	// something held
	if (IsInMap(source) && world_objects::IsVillager(target) && villager_fire::IsFireMan(target))
	{
		return;
	}
	if (fire.source != entt::null && fire.source == target)
	{
		return;
	}
	const auto sourceMaterial = MaterialOf(source);
	const auto targetMaterial = MaterialOf(target);
	if (!sourceMaterial.has_value() || !targetMaterial.has_value())
	{
		return;
	}
	const float sourceLife = world_objects::LifeOf(source);
	const float radius =
	    fire::FireRadius(*sourceMaterial, fire::FireFraction(fire.state.temperature, *sourceMaterial, sourceLife));
	if (radius == 0.0f)
	{
		return;
	}
	const bool burning = fire::IsOnFire(fire.state.temperature, *sourceMaterial);
	// Something hot that doesn't burn only heats what catches below its temperature, by its table's own combustion
	// temperature rather than the least a fire burns at
	if (!burning && targetMaterial->combustion > fire.state.temperature)
	{
		return;
	}
	const auto sourceCentre = FireCentre(source);
	const auto targetCentre = FireCentre(target);
	if (!(gutils::GetDistanceInMetres(targetCentre, sourceCentre) < targetMaterial->fireRadius + radius))
	{
		return;
	}
	// High above the land, the flames must reach across the gap
	if (sourceCentre.y >= fire::k_LowFireHeight || targetCentre.y >= fire::k_LowFireHeight)
	{
		if (!(sourceCentre.y - targetCentre.y <
		      targetMaterial->height + fire::FlameHeight(fire.state.temperature, *sourceMaterial)))
		{
			return;
		}
	}
	const float targetTemperature = GetTemperature(target);
	if (!(fire.state.temperature - targetTemperature > 0.0f))
	{
		return;
	}
	const auto player = fire.hasPlayer ? std::optional(fire.player) : std::nullopt;
	auto* targetFire = FindOrCreate(target, player, entt::null);
	if (targetFire == nullptr)
	{
		return;
	}
	// The source may have moved in the registry as the target's fire was made
	auto& sourceFire = registry.Get<Fire>(source);
	const auto transfer =
	    fire::HeatTransfer(sourceFire.state.temperature, *sourceMaterial, targetFire->state.temperature, *targetMaterial);
	targetFire->state.temperature = transfer.targetTemperature;
	sourceFire.state.temperature = transfer.sourceTemperature;
	if (!IsHeld(target) && targetFire->root != sourceFire.root)
	{
		JoinBlaze(source, target);
	}
	// A villager the heat reaches, not running on fire already, runs from it
	if (world_objects::IsVillager(target) && !villager_fire::IsRunningOnFire(target))
	{
		villager_fire::SetupOnFire(target, source);
	}
}

void FireSystem::JoinBlaze(entt::entity fire, entt::entity other)
{
	auto& registry = Entities();
	const auto myRoot = registry.Get<Fire>(fire).root;
	const auto otherRoot = registry.Get<Fire>(other).root;
	if (myRoot == otherRoot || !registry.AllOf<FireGroup>(myRoot) || !registry.AllOf<FireGroup>(otherRoot))
	{
		return;
	}
	// The other's whole blaze joins this one, right after this fire, its firemen with it
	auto theirs = std::move(registry.Get<FireGroup>(otherRoot));
	registry.Remove<FireGroup>(otherRoot);
	auto& mine = registry.Get<FireGroup>(myRoot);
	const auto at = std::ranges::find(mine.members, fire);
	mine.members.insert(at == mine.members.end() ? at : std::next(at), theirs.members.begin(), theirs.members.end());
	mine.firemen.insert(mine.firemen.end(), theirs.firemen.begin(), theirs.firemen.end());
	for (const auto member : theirs.members)
	{
		if (auto* memberFire = registry.TryGet<Fire>(member))
		{
			memberFire->root = myRoot;
		}
	}
}

void FireSystem::HandOverBlaze(entt::entity root, entt::entity newRoot)
{
	auto& registry = Entities();
	auto group = std::move(registry.Get<FireGroup>(root));
	registry.Remove<FireGroup>(root);
	// The new first goes first, the old one right after it
	std::erase(group.members, newRoot);
	group.members.insert(group.members.begin(), newRoot);
	for (const auto member : group.members)
	{
		if (auto* memberFire = registry.TryGet<Fire>(member))
		{
			memberFire->root = newRoot;
		}
	}
	registry.Assign<FireGroup>(newRoot, std::move(group));
}

void FireSystem::StopFiremen(entt::entity root, std::optional<entt::entity> onlyFightingThis)
{
	const auto* group = Entities().TryGet<const FireGroup>(root);
	if (group == nullptr)
	{
		return;
	}
	// Stopping takes them off the list
	const auto firemen = group->firemen;
	for (const auto villager : firemen)
	{
		if (!onlyFightingThis.has_value() || villager_fire::FireOf(villager) == *onlyFightingThis)
		{
			villager_fire::StopFireFighting(villager);
		}
	}
}

void FireSystem::LeaveBlaze(entt::entity object)
{
	auto& registry = Entities();
	auto* fire = registry.TryGet<Fire>(object);
	if (fire == nullptr)
	{
		return;
	}
	const auto root = fire->root;
	if (root == object)
	{
		const auto* group = registry.TryGet<const FireGroup>(object);
		if (group == nullptr || group->members.size() <= 1)
		{
			StopFiremen(object, std::nullopt);
			return;
		}
		StopFiremen(object, object);
		const auto next = group->members.at(1);
		HandOverBlaze(object, next);
		auto& rest = registry.Get<FireGroup>(next);
		std::erase(rest.members, object);
		registry.Get<Fire>(object).root = object;
		registry.Assign<FireGroup>(object, FireGroup {.members = {object}, .firemen = {}});
		return;
	}
	if (!registry.Valid(root) || !registry.AllOf<FireGroup>(root))
	{
		fire->root = object;
		registry.Assign<FireGroup>(object, FireGroup {.members = {object}, .firemen = {}});
		return;
	}
	StopFiremen(root, object);
	std::erase(registry.Get<FireGroup>(root).members, object);
	registry.Get<Fire>(object).root = object;
	registry.Assign<FireGroup>(object, FireGroup {.members = {object}, .firemen = {}});
}

bool FireSystem::Detach(entt::entity object)
{
	auto& registry = Entities();
	std::erase(_fires, object);
	StopSound(object);
	if (!registry.Valid(object) || !registry.AllOf<Fire>(object))
	{
		return false;
	}
	LeaveBlaze(object);
	// Every fire reaction it started goes with it, and any to it burning in the hand
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(object, Reaction::ReactToFire);
		Locator::reactionSystem::value().RemoveFrom(object, Reaction::ReactToBurningObjectInHand);
	}
	registry.Remove<FireGroup>(object);
	registry.Remove<FireLook>(object);
	registry.Remove<Fire>(object);
	return true;
}

void FireSystem::Forget(entt::entity object)
{
	if (Locator::entitiesRegistry::has_value() && Detach(object))
	{
		Entities().SetDirty();
	}
}

void FireSystem::PruneBlazes()
{
	auto& registry = Entities();
	// A blaze whose first fire has gone, or that has lost track of a fire: those fires each burn on their own
	registry.Each<Fire>([&](entt::entity object, Fire& fire) {
		if (fire.root == object)
		{
			return;
		}
		const auto* group = registry.Valid(fire.root) ? registry.TryGet<const FireGroup>(fire.root) : nullptr;
		if (group == nullptr || std::ranges::find(group->members, object) == group->members.end())
		{
			fire.root = object;
		}
	});
	std::vector<entt::entity> alone;
	registry.Each<Fire>([&](entt::entity object, const Fire& fire) {
		if (fire.root == object && !registry.AllOf<FireGroup>(object))
		{
			alone.push_back(object);
		}
	});
	for (const auto object : alone)
	{
		registry.Assign<FireGroup>(object, FireGroup {.members = {object}, .firemen = {}});
	}
	// Each blaze keeps only the fires still in it and the villagers still about
	registry.Each<FireGroup>([&](entt::entity root, FireGroup& group) {
		std::erase_if(group.members, [&](entt::entity member) {
			const auto* fire = registry.Valid(member) ? registry.TryGet<const Fire>(member) : nullptr;
			return fire == nullptr || fire->root != root;
		});
		std::erase_if(group.firemen, [&](entt::entity villager) { return !registry.Valid(villager); });
	});
}

void FireSystem::Delete(entt::entity object)
{
	auto& registry = Entities();
	if (!Detach(object))
	{
		return;
	}
	// A dead tree that stops burning is wood the people want again
	if (registry.AllOf<DeadTree>(object) && Locator::reactionSystem::has_value())
	{
		if (const auto* transform = registry.TryGet<const Transform>(object))
		{
			Locator::reactionSystem::value().Create({.initiator = object,
			                                         .type = Reaction::ReactToWood,
			                                         .player = world_objects::PlayerOf(object).value_or(PlayerNames::NEUTRAL),
			                                         .position = transform->position});
		}
	}
	// A pot that stops burning calls its people again, when it holds something and is not one of a store's piles
	if (const auto* pot = registry.TryGet<const Pot>(object);
	    pot != nullptr && pot->amount > 0 && Locator::reactionSystem::has_value() && Locator::infoConstants::has_value())
	{
		const bool inStore =
		    Locator::resourceStoreSystem::has_value() && Locator::resourceStoreSystem::value().StoreOf(object).has_value();
		const auto reaction = Locator::infoConstants::value().pot.at(static_cast<size_t>(pot->type)).associatedReaction;
		const auto* transform = registry.TryGet<const Transform>(object);
		auto& reactions = Locator::reactionSystem::value();
		if (!inStore && reaction != Reaction::None && transform != nullptr && !reactions.HasReaction(object))
		{
			reactions.Create({.initiator = object,
			                  .type = reaction,
			                  .player = world_objects::PlayerOf(object).value_or(PlayerNames::NEUTRAL),
			                  .position = transform->position});
		}
	}
	// A magic tree that stops burning draws its caster's people again
	if (const auto* magic = registry.TryGet<const MagicTree>(object); magic != nullptr && Locator::reactionSystem::has_value())
	{
		const auto* transform = registry.TryGet<const Transform>(object);
		const auto* forest = registry.Valid(magic->forest) ? registry.TryGet<const MagicForest>(magic->forest) : nullptr;
		if (transform != nullptr)
		{
			Locator::reactionSystem::value().Create({.initiator = object,
			                                         .type = Reaction::ReactToMagicTree,
			                                         .player = forest != nullptr ? forest->player : PlayerNames::NEUTRAL,
			                                         .position = transform->position,
			                                         .impressiveValue = magic->impressiveValue});
		}
	}
	// A fireball's fire is the ball: when it has cooled away the ball goes too
	if (registry.AllOf<MagicFireBall>(object))
	{
		registry.Destroy(object);
	}
	registry.SetDirty();
}

void FireSystem::ConsiderSound(entt::entity object, float fraction)
{
	const auto playing = std::ranges::find(_soundSlots, object, &SoundSlot::fire);
	if (!(fraction > fire::k_SoundFraction))
	{
		if (playing != _soundSlots.end())
		{
			StopSound(object);
		}
		return;
	}
	if (playing != _soundSlots.end() || !Locator::camera::has_value())
	{
		return;
	}
	const auto* transform = Entities().TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return;
	}
	// The distance across the ground to the camera's eye. A fire takes a slot only when it is nearer than the farthest
	// one crackling, or when none crackles, and the slot it takes is the first free one or the first not nearer than
	// the farthest. With one fire crackling that is always the first slot, so only one fire is ever heard: the nearest,
	// its loop playing on unbroken while it stays the nearest.
	const float distance = gutils::GetDistanceInMetres(Locator::camera::value().GetOrigin(), transform->position);
	if (!(distance < _farthestSlot) && _farthestSlot != 0.0f)
	{
		return;
	}
	for (auto& slot : _soundSlots)
	{
		if (slot.fire == entt::null || !(slot.distance < _farthestSlot))
		{
			if (slot.fire != entt::null)
			{
				StopSound(slot.fire);
			}
			slot.fire = object;
			slot.distance = distance;
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Fire: object {} crackles, {:.1f} m from the camera",
			                    entt::to_integral(object), distance);
			break;
		}
	}
	_farthestSlot = 0.0f;
	for (const auto& slot : _soundSlots)
	{
		if (slot.fire != entt::null)
		{
			_farthestSlot = std::max(_farthestSlot, slot.distance);
		}
	}
}

void FireSystem::PlaySounds()
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	auto& audio = Locator::audio::value();
	auto& registry = Entities();
	for (auto& slot : _soundSlots)
	{
		if (slot.fire == entt::null)
		{
			continue;
		}
		const auto* transform = registry.TryGet<const Transform>(slot.fire);
		if (transform == nullptr)
		{
			StopSound(slot.fire);
			continue;
		}
		// Played again each turn, which a sound already playing ignores
		if (slot.emitter != entt::null && audio.EmitterExists(slot.emitter) &&
		    audio.GetStatus(slot.emitter) != audio::AudioStatus::Stopped)
		{
			audio.SetEmitterPosition(slot.emitter, transform->position);
			continue;
		}
		slot.emitter = audio.StartSoundEffect(static_cast<entt::id_type>(audio::SoundId::G_Fire_01),
		                                      {.position = transform->position, .owner = slot.fire});
	}
}

void FireSystem::StopSound(entt::entity object)
{
	for (auto& slot : _soundSlots)
	{
		if (slot.fire != object)
		{
			continue;
		}
		if (slot.emitter != entt::null && Locator::audio::has_value() && Locator::audio::value().EmitterExists(slot.emitter))
		{
			Locator::audio::value().StopEmitter(slot.emitter);
		}
		slot = SoundSlot {};
	}
}

void FireSystem::Update(float seconds)
{
	if (!Locator::entitiesRegistry::has_value() || seconds <= 0.0f)
	{
		return;
	}
	_frameTime += seconds;
	auto& registry = Entities();
	const uint32_t turn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
	auto* random = Locator::gameRandom::has_value() ? &Locator::gameRandom::value() : nullptr;
	registry.Each<const Fire, FireLook, const Transform>(
	    [&](entt::entity object, const Fire& fire, FireLook& look, const Transform& transform) {
		    const auto material = MaterialOf(object);
		    if (!material.has_value())
		    {
			    return;
		    }
		    glm::vec3 wind(0.0f);
		    if (Locator::weatherSystem::has_value())
		    {
			    // The steam and smoke drift with the wind blended between the weather's nearest cells, the storms' too
			    const auto weather = Locator::weatherSystem::value().GetWeatherSmooth(transform.position);
			    wind = glm::vec3(static_cast<float>(weather.windX), 0.0f, static_cast<float>(weather.windZ)) * k_WindByteSpeed;
		    }
		    const fire::graphic::Inputs inputs {
		        .fraction = fire::FireFraction(fire.state.temperature, *material, world_objects::LifeOf(object)),
		        .temperature = fire.state.temperature,
		        .flags = fire.state.flags,
		        .turn = turn,
		        .wind = wind,
		    };
		    const fire::graphic::Sampler sampler {
		        .localPoint = [object] { return RandomPointOn(object); },
		        .toWorld = [object](const glm::vec3& local) { return ToWorld(object, local); },
		        .random = [random](float range) { return random != nullptr ? random->LocalFloatRand(range) : 0.0f; },
		    };
		    if (fire::graphic::Update(look.graphic, inputs, seconds, sampler) && Locator::audio::has_value())
		    {
			    // The steam hisses as it starts
			    SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Fire: object {} hisses with steam at {:.0f}",
			                        entt::to_integral(object), fire.state.temperature);
			    Locator::audio::value().StartSoundEffect(static_cast<entt::id_type>(audio::SoundId::G_Steam_01),
			                                             {.position = transform.position});
		    }
	    });
}

void FireSystem::CollectDrawFrame(float turnFraction, particles::draw::Frame& frame) const
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	const auto& registry = Entities();
	const particles::draw::Sources sources {
	    .textures = [](std::string_view texture) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    if (texture == k_FlameTexture)
		    {
			    return std::pair {k_FlameSheet.value(), k_FlameSheetAlpha.value()};
		    }
		    return std::pair {k_PuffSheet.value(), k_PuffSheetAlpha.value()};
	    },
	    .playerColour = {},
	    .random = {},
	};
	const auto& creators = Creators();
	const auto lightMap = FireLightMap();
	const float turnTime =
	    (Locator::time::has_value() ? static_cast<float>(Locator::time::value().GetTurn()) : 0.0f) + turnFraction;
	particles::Effect::DrawWalk walk;
	registry.Each<const Fire, const FireLook, const Transform>([&](entt::entity object, const Fire& fire, const FireLook& look,
	                                                               const Transform& transform) {
		walk.Clear();
		const auto& graphic = look.graphic;
		const float ground = graphic.kinds.followsLand ? LandHeight({transform.position.x, transform.position.z}) : 0.0f;
		const auto add = [&](const particles::Creator& creator, const glm::vec3& position, float scale, float alpha, int cell,
		                     uint32_t colour, float age) {
			walk.steps.push_back({.chain = false, .index = static_cast<uint32_t>(walk.atoms.size())});
			walk.atoms.push_back({
			    .creator = &creator,
			    .position = position,
			    .rotation = glm::mat3(1.0f),
			    .scale = scale,
			    .stretch = k_FlameStretch,
			    .alpha = alpha,
			    .frame = static_cast<float>(cell),
			    .rgb = Rgb(colour),
			    .age = age,
			});
		};
		for (const auto& flame : graphic.flames)
		{
			auto position = ToWorld(object, flame.position);
			if (graphic.kinds.followsLand)
			{
				// The flames rise and fall with the land under them
				position.y += LandHeight({position.x, position.z}) - ground;
			}
			add(creators.flame, position, flame.scale, flame.alpha, fire::graphic::FlameCell(flame.age),
			    fire::graphic::k_FlameColour, flame.age);
		}
		for (const auto& puff : graphic.steam)
		{
			add(creators.steam, puff.position, puff.scale, puff.alpha, fire::graphic::PuffCell(puff.age), 0xFFFFFFu, puff.age);
		}
		for (const auto& puff : graphic.smoke)
		{
			add(creators.smoke, puff.position, puff.scale, puff.alpha, fire::graphic::PuffCell(puff.age),
			    fire::graphic::k_SmokeColour, puff.age);
		}
		if (!walk.atoms.empty())
		{
			particles::draw::AddEffect(frame, walk, particles::draw::DrawPath::Sorted, transform.position,
			                           particles::draw::k_NeutralPlayer, sources);
		}
		// A large building burning lights the land round it, flickering
		if (look.lightsLand && lightMap.has_value())
		{
			const auto material = MaterialOf(object);
			if (!material.has_value())
			{
				return;
			}
			const float fraction = fire::FireFraction(fire.state.temperature, *material, world_objects::LifeOf(object));
			const float flicker =
			    Noise().Line(k_FlickerRate * turnTime + static_cast<float>(entt::to_integral(object) & 0xFFFFu));
			const float strength = fire::graphic::LightStrength(fraction, fire.state.charring, flicker);
			if (strength > 0.0f)
			{
				frame.lightStamps.push_back({
				    .bitmap = *lightMap,
				    .frame = 0,
				    .pitch = k_LightMapPitch,
				    .centre = transform.position + k_LightMapOffset,
				    .strength = strength,
				});
			}
		}
	});
}

std::optional<uint32_t> FireSystem::GetCharredColour(entt::entity object) const
{
	const auto* fire = Entities().TryGet<const Fire>(object);
	if (fire == nullptr || fire->state.charring <= 0.0f)
	{
		return std::nullopt;
	}
	const uint32_t grey = fire::graphic::CharredGrey(fire->state.charring);
	return (grey << 16u) | (grey << 8u) | grey;
}

std::optional<uint32_t> FireSystem::GetGlowColour(entt::entity object) const
{
	const auto* fire = Entities().TryGet<const Fire>(object);
	if (fire == nullptr)
	{
		return std::nullopt;
	}
	// It flickers with the same noise as a burning building's light on the land, by the game time
	const float turnTime = Locator::time::has_value()
	                           ? static_cast<float>(Locator::time::value().GetTurn()) + Locator::time::value().GetTurnFraction()
	                           : 0.0f;
	const float flicker = Noise().Line(k_FlickerRate * turnTime + static_cast<float>(entt::to_integral(object) & 0xFFFFu));
	return fire::graphic::GlowColour(fire->state.temperature, fire->state.charring, flicker);
}

std::optional<FireSystemInterface::TreeLook> FireSystem::GetBurningTreeLook(entt::entity tree) const
{
	const auto* fire = Entities().TryGet<const Fire>(tree);
	const auto material = fire != nullptr ? MaterialOf(tree) : std::nullopt;
	if (!material.has_value())
	{
		return std::nullopt;
	}
	const auto look = fire::graphic::BurningTree(fire->state.temperature, material->combustion, world_objects::LifeOf(tree));
	return TreeLook {.grey = look.grey, .alphaReference = look.alphaReference, .scale = look.scale};
}

std::vector<FireSystemInterface::FireInfo> FireSystem::GetFires() const
{
	std::vector<FireInfo> fires;
	const auto& registry = Entities();
	for (const auto object : _fires)
	{
		const auto* fire = registry.TryGet<const Fire>(object);
		const auto* transform = registry.TryGet<const Transform>(object);
		const auto material = MaterialOf(object);
		if (fire == nullptr || transform == nullptr || !material.has_value())
		{
			continue;
		}
		const auto* group = registry.TryGet<const FireGroup>(fire->root);
		const float life = world_objects::LifeOf(object);
		fires.push_back({
		    .object = object,
		    .position = transform->position,
		    .temperature = fire->state.temperature,
		    .combustion = fire::CombustionTemperature(*material),
		    .charring = fire->state.charring,
		    .fraction = fire::FireFraction(fire->state.temperature, *material, life),
		    .life = life,
		    .burning = fire::IsOnFire(fire->state.temperature, *material),
		    .root = fire->root,
		    .blazeSize = group != nullptr ? group->members.size() : 0,
		    .firemen = group != nullptr ? group->firemen.size() : 0,
		});
	}
	return fires;
}

void FireSystem::Reset()
{
	for (const auto& slot : _soundSlots)
	{
		if (slot.emitter != entt::null && Locator::audio::has_value() && Locator::audio::value().EmitterExists(slot.emitter))
		{
			Locator::audio::value().StopEmitter(slot.emitter);
		}
	}
	_soundSlots = {};
	_farthestSlot = 0.0f;
	_burnPointTurn.reset();
	_fires.clear();
	if (Locator::entitiesRegistry::has_value())
	{
		auto& registry = Entities();
		std::vector<entt::entity> burning;
		registry.Each<const Fire>([&](entt::entity object, const Fire&) { burning.push_back(object); });
		for (const auto object : burning)
		{
			registry.Remove<Fire>(object);
			registry.Remove<FireGroup>(object);
			registry.Remove<FireLook>(object);
		}
	}
}
