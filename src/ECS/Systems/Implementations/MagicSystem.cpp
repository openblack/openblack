/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "MagicSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <numbers>

#include <LNDFile.h>
#include <entt/core/hashed_string.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/WaterRings.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "Creature/CreatureAudio.h"
#include "Creature/CreatureLocomotion.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Archetypes/SpellSeedArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/MagicPile.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"
#include "Magic/MagicTables.h"
#include "Magic/SpellSeedRules.h"
#include "Particles/ParticleEffect.h"
#include "Particles/ParticleShields.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnsPerSecond = 1.0f / k_TurnSeconds;
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;

/// The hand's speed is smoothed over about this many seconds
constexpr float k_HandSpeedSmoothing = 0.1f;
/// The kind of building a dispenser is
constexpr auto k_DispenserBuilding = AbodeInfo::NorseSpellDispenser;
/// The size a miracle cast from the hand takes without a gesture: a sizing miracle its tables' usual size, any other
/// the plain size
constexpr float k_PlainHandMagnitude = 1.0f;
/// The id of the bubble's model in the resource cache
constexpr auto k_BubbleMeshId = entt::hashed_string("spells/o_bibble_up");
/// How far round a point a struck shield is looked for, to show its spark
constexpr float k_ShieldStrikeMargin = 2.0f;
/// A bubble is easier to tap than its model is big
constexpr float k_OrbTapLeeway = 1.3f;
constexpr float k_DefaultOrbRadius = 2.5f;

// The game's world
/// Piles a miracle puts down closer than this to one of the same kind go onto it
constexpr float k_PileMergeRadius = 3.0f;
/// The food and wood piles' sizes
constexpr float k_FoodPileScale = 0.3f;
constexpr float k_WoodPileScale = 0.7f;
/// Something set alight shows flames for this many turns after it was last heated
constexpr int k_BurnTurns = 100;
/// Flames on something alight are this share of its height, within limits
constexpr float k_FlameShareOfHeight = 0.6f;
/// and rise from this share of its height
constexpr float k_FlameRise = 0.3f;
constexpr float k_SmallestFlames = 1.0f;
constexpr float k_LargestFlames = 8.0f;
/// A villager's health out of this is its life
constexpr float k_VillagerHealthScale = 100.0f;
/// Living things this close to an effect's point, besides its radius, take it
constexpr float k_LivingReach = 1.0f;
/// The colours of the rings the water's drops leave on the land, picked at random
constexpr std::array<uint32_t, 5> k_RippleColours = {0xFF80CBC5u, 0xFF8599C5u, 0xFFBA97B2u, 0xFFB9CA86u, 0xFFBD9C8Au};

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

const GObjectInfo* InfoOfLiving(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	const auto& info = Locator::infoConstants::value();
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		const auto row = creature::InfoRow(creature->species);
		return row < info.creature.size() ? &info.creature.at(row) : nullptr;
	}
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		const auto kind = static_cast<size_t>(GVillagerInfo::Find(villager->tribe, villager->number));
		return kind < info.villager.size() ? &info.villager.at(kind) : nullptr;
	}
	return nullptr;
}

/// A living thing's life, 0 to 1, none for anything else
std::optional<float> LifeOf(entt::entity entity)
{
	const auto& registry = EntityRegistry();
	if (const auto* needs = registry.TryGet<const CreatureNeeds>(entity))
	{
		return needs->needs.life;
	}
	if (const auto* villager = registry.TryGet<const Villager>(entity))
	{
		return static_cast<float>(villager->health) / k_VillagerHealthScale;
	}
	return std::nullopt;
}

void SetLife(entt::entity entity, float life)
{
	auto& registry = EntityRegistry();
	life = std::clamp(life, 0.0f, 1.0f);
	if (auto* needs = registry.TryGet<CreatureNeeds>(entity))
	{
		needs->needs.life = life;
	}
	else if (auto* villager = registry.TryGet<Villager>(entity))
	{
		villager->health = static_cast<uint32_t>(std::lround(life * k_VillagerHealthScale));
	}
}

/// Calls the function with every living thing (creatures and villagers) and where it stands
template <typename F>
void EachLiving(F&& function)
{
	const auto& registry = EntityRegistry();
	registry.Each<const Creature, const Transform>(
	    [&](entt::entity entity, const Creature&, const Transform& transform) { function(entity, transform.position); });
	registry.Each<const Villager, const Transform>(
	    [&](entt::entity entity, const Villager&, const Transform& transform) { function(entity, transform.position); });
}

float AcrossGround(glm::vec3 a, glm::vec3 b)
{
	return glm::distance(glm::vec2(a.x, a.z), glm::vec2(b.x, b.z));
}

/// How tall a model stands at a scale
float HeightOfMesh(entt::id_type mesh, float scale)
{
	auto& meshes = Locator::resources::value().GetMeshes();
	return meshes.Contains(mesh) ? meshes.Handle(mesh)->GetBoundingBox().Size().y * scale : 0.0f;
}
} // namespace

// The game's world

float GameMagicWorld::LandHeight(glm::vec2 xz) const
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(xz) : 0.0f;
}

bool GameMagicWorld::InBounds(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value())
	{
		return false;
	}
	const auto extent = Locator::terrainSystem::value().GetExtent();
	return point.x >= extent.minimum.x && point.z >= extent.minimum.y && point.x < extent.maximum.x &&
	       point.z < extent.maximum.y;
}

bool GameMagicWorld::IsDryLand(glm::vec3 point) const
{
	if (!InBounds(point) || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	constexpr float k_CellSize = 10.0f;
	const auto cell = glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize));
	const auto* landCell = Locator::terrainSystem::value().FindCell(cell);
	return landCell != nullptr && landCell->properties.hasWater == 0;
}

bool GameMagicWorld::InInfluence(PlayerNames player, glm::vec3 point) const
{
	return Locator::influenceSystem::has_value() && Locator::influenceSystem::value().PlayerInfluence(player, point) > 0.0f;
}

std::optional<glm::vec3> GameMagicWorld::PositionOf(entt::entity object) const
{
	const auto& registry = EntityRegistry();
	if (!registry.Valid(object))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(object);
	return transform != nullptr ? std::optional(transform->position) : std::nullopt;
}

bool GameMagicWorld::ApplyEffect(entt::entity object, const magic::EffectValues& values)
{
	if (!EntityRegistry().Valid(object))
	{
		return false;
	}
	const float burn = values[magic::EffectKind::Burn];
	// Water puts out flames
	if (burn < 0.0f)
	{
		PutOut(object);
	}
	const auto life = LifeOf(object);
	const auto* info = InfoOfLiving(object);
	if (!life.has_value() || info == nullptr)
	{
		// Something that doesn't live: a bolt sets it alight
		if (burn > 0.0f && EntityRegistry().AnyOf<Tree, Abode, Feature>(object))
		{
			SetAlight(object);
		}
		return false;
	}
	const auto defence = magic::EffectDefence::From(*info);
	// There is no fire to spread the burn yet: it hurts as heat at once
	const float heat = burn > 0.0f ? magic::HeatDamage(burn, defence) : 0.0f;
	SetLife(object, magic::LifeAfter(*life, values, defence) - heat);
	return true;
}

entt::entity GameMagicWorld::ApplyEffectAt(glm::vec3 point, const magic::EffectValues& values)
{
	entt::entity nearest = entt::null;
	float best = values.radius + k_LivingReach;
	EachLiving([&](entt::entity entity, const glm::vec3& position) {
		const float distance = AcrossGround(point, position);
		if (distance < best)
		{
			best = distance;
			nearest = entity;
		}
	});
	if (nearest != entt::null)
	{
		ApplyEffect(nearest, values);
	}
	return nearest;
}

void GameMagicWorld::Heat(glm::vec3 point, float radius, float temperature)
{
	if (!(temperature > 0.0f))
	{
		return;
	}
	auto& registry = EntityRegistry();
	EachLiving([&](entt::entity entity, const glm::vec3& position) {
		const auto* info = InfoOfLiving(entity);
		const auto life = LifeOf(entity);
		if (info == nullptr || !life.has_value() || glm::distance(point, position) > radius)
		{
			return;
		}
		SetLife(entity, *life - magic::HeatDamage(temperature, magic::EffectDefence::From(*info)));
	});
	// What burns nearby catches
	std::vector<entt::entity> caught;
	const auto consider = [&](entt::entity entity, const Transform& transform) {
		if (AcrossGround(point, transform.position) < radius && point.y - transform.position.y < radius * 2.0f)
		{
			caught.push_back(entity);
		}
	};
	registry.Each<const Tree, const Transform>([&](entt::entity e, const Tree&, const Transform& t) { consider(e, t); });
	registry.Each<const Abode, const Transform>([&](entt::entity e, const Abode&, const Transform& t) { consider(e, t); });
	for (const auto entity : caught)
	{
		SetAlight(entity);
	}
}

void GameMagicWorld::Water(glm::vec3 drop, float reach, std::optional<float> ringGrowth)
{
	std::vector<entt::entity> wet;
	for (const auto& [entity, burning] : _burning)
	{
		if (const auto position = PositionOf(entity); position && AcrossGround(*position, drop) < reach)
		{
			wet.push_back(entity);
		}
	}
	for (const auto entity : wet)
	{
		PutOut(entity);
	}
	if (ringGrowth.has_value() && Locator::waterRingSystem::has_value() && Locator::gameRandom::has_value())
	{
		auto& random = Locator::gameRandom::value();
		const auto colour = k_RippleColours.at(random.GameRand(static_cast<uint32_t>(k_RippleColours.size())));
		Locator::waterRingSystem::value().Add(
		    {.position = drop, .growth = *ringGrowth, .angle = random.GameFloatRand(k_TwoPi), .argb = colour});
	}
}

std::vector<entt::entity> GameMagicWorld::HealTargets(glm::vec3 point, float radius, size_t maximum) const
{
	std::vector<std::pair<float, entt::entity>> found;
	EachLiving([&](entt::entity entity, const glm::vec3& position) {
		const float distance = AcrossGround(point, position);
		if (distance < radius)
		{
			found.emplace_back(distance, entity);
		}
	});
	std::ranges::sort(found, {}, &std::pair<float, entt::entity>::first);
	std::vector<entt::entity> targets;
	for (const auto& [distance, entity] : found)
	{
		if (targets.size() >= maximum)
		{
			break;
		}
		targets.push_back(entity);
	}
	return targets;
}

bool GameMagicWorld::AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool sparkles)
{
	if (amount == 0 || (type != ResourceType::Food && type != ResourceType::Wood))
	{
		return false;
	}
	auto& registry = EntityRegistry();
	point.y = LandHeight({point.x, point.z});
	// Onto a pile of the same nearby with room left
	entt::entity pile = entt::null;
	registry.Each<MagicPile, Pot, const Transform>(
	    [&](entt::entity entity, const MagicPile& magicPile, const Pot& pot, const Transform& transform) {
		    if (pile == entt::null && magicPile.resource == type && pot.amount < pot.maxAmount &&
		        AcrossGround(point, transform.position) < k_PileMergeRadius)
		    {
			    pile = entity;
		    }
	    });
	if (pile != entt::null)
	{
		auto& pot = registry.Get<Pot>(pile);
		pot.amount = static_cast<uint16_t>(std::min<uint32_t>(pot.amount + amount, pot.maxAmount));
		registry.Get<MagicPile>(pile).sparkles |= sparkles;
		return true;
	}
	const auto potType = type == ResourceType::Food ? PotInfo::MagicFood : PotInfo::MagicWood;
	const auto created = archetypes::PotArchetype::Create(point, 0.0f, potType, static_cast<int32_t>(amount));
	if (created == entt::null)
	{
		return false;
	}
	registry.Get<Transform>(created).scale = glm::vec3(type == ResourceType::Food ? k_FoodPileScale : k_WoodPileScale);
	registry.Assign<MagicPile>(created, type, sparkles);
	// A pile appears with a puff
	if (Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().StartSpotVisual(SpotVisualType::MagicObjectCreated, point, std::nullopt, entt::null);
	}
	return true;
}

void GameMagicWorld::SetAlight(entt::entity object)
{
	if (auto found = _burning.find(object); found != _burning.end())
	{
		found->second.turnsLeft = k_BurnTurns;
		return;
	}
	const auto position = PositionOf(object);
	if (!position.has_value() || !Locator::particleSystem::has_value())
	{
		return;
	}
	// There is no fire yet to burn it: it shows a bonfire's flames, as big as it is
	float height = 1.0f;
	if (const auto* mesh = EntityRegistry().TryGet<const Mesh>(object))
	{
		height = HeightOfMesh(mesh->id, EntityRegistry().Get<const Transform>(object).scale.y);
	}
	// Among its branches rather than at its foot
	const auto flames = *position + glm::vec3(0.0f, height * k_FlameRise, 0.0f);
	const auto effect = Locator::particleSystem::value().Start(
	    ParticleType::Bonfire, flames, std::clamp(height * k_FlameShareOfHeight, k_SmallestFlames, k_LargestFlames), true);
	_burning.emplace(object, Burning {.effect = effect, .turnsLeft = k_BurnTurns});
}

void GameMagicWorld::PutOut(entt::entity object)
{
	if (const auto found = _burning.find(object); found != _burning.end())
	{
		if (Locator::particleSystem::has_value())
		{
			Locator::particleSystem::value().CloseDown(found->second.effect);
		}
		_burning.erase(found);
	}
}

void GameMagicWorld::ProcessTurn()
{
	std::erase_if(_burning, [this](auto& entry) {
		auto& [object, burning] = entry;
		if (--burning.turnsLeft > 0 && PositionOf(object).has_value())
		{
			return false;
		}
		if (Locator::particleSystem::has_value())
		{
			Locator::particleSystem::value().CloseDown(burning.effect);
		}
		return true;
	});
}

void GameMagicWorld::Reset()
{
	_burning.clear();
}

// The links between a miracle and its effect

/// What a miracle's particle effect tells it, and asks of it
class MagicSystem::SpellLink final: public particles::SpellSink
{
public:
	SpellLink(MagicSystem& system, entt::entity spell)
	    : _system(system)
	    , _spell(spell)
	{
	}
	bool SpellEvent(const particles::SpellEventInfo& event) override { return _system.OnSpellEvent(_spell, event); }
	[[nodiscard]] int PowerUpLevel() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr ? magic::GetPowerUpLevel(Locator::infoConstants::value(), spell->magicType)
		                        : magic::k_BasePowerUpLevel;
	}
	[[nodiscard]] bool IsMyInterfaceCasting() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr && spell->castFromHand;
	}
	[[nodiscard]] bool IsHumanPlayerCasting() const override
	{
		const auto* spell = EntityRegistry().TryGet<const components::Spell>(_spell);
		return spell != nullptr && spell->humanCasting;
	}
	[[nodiscard]] entt::entity Spell() const override { return _spell; }

private:
	MagicSystem& _system;
	entt::entity _spell;
};

/// The held miracle's in-hand effect: this computer's player holds it, and it sends nothing back
class MagicSystem::HandEffectLink final: public particles::SpellSink
{
public:
	explicit HandEffectLink(const MagicSystem& system)
	    : _system(system)
	{
	}
	bool SpellEvent(const particles::SpellEventInfo& /*event*/) override { return false; }
	[[nodiscard]] int PowerUpLevel() const override
	{
		if (const auto held = _system.GetHeldSeed())
		{
			if (const auto* seed = EntityRegistry().TryGet<const SpellSeed>(*held))
			{
				return seed->powerUp;
			}
		}
		return magic::k_BasePowerUpLevel;
	}
	[[nodiscard]] bool IsMyInterfaceCasting() const override { return true; }
	[[nodiscard]] bool IsHumanPlayerCasting() const override { return true; }

private:
	const MagicSystem& _system;
};

// The system

MagicSystem::MagicSystem()
    : _handEffectLink(std::make_unique<HandEffectLink>(*this))
{
	for (size_t p = 0; p < _players.size(); ++p)
	{
		_players.at(p) = std::make_unique<magic::PlayerSpellCaster>(static_cast<PlayerNames>(p));
	}
}

MagicSystem::~MagicSystem() = default;

const InfoConstants& MagicSystem::Info() const
{
	return Locator::infoConstants::value();
}

magic::SpellCasterInterface* MagicSystem::CasterOf(const Spell& spell)
{
	switch (spell.caster.kind)
	{
	case SpellCaster::Kind::Player:
		return _players.at(static_cast<size_t>(spell.caster.player)).get();
	case SpellCaster::Kind::Object:
		return EntityRegistry().Valid(spell.caster.entity) ? &_objectCaster : nullptr;
	case SpellCaster::Kind::Creature:
		// Creatures paying with their energy come with their casting
	case SpellCaster::Kind::None:
		break;
	}
	return nullptr;
}

float MagicSystem::PlayerTribalPower(PlayerNames /*player*/, MagicType type) const
{
	// The players' tribal power multipliers come with worship; until then every tribe's is 1
	std::array<float, magic::k_TribeCount> multipliers {};
	multipliers.fill(1.0f);
	return magic::GetTribalPower(magic::GetMagicEffectInfo(Info(), type), multipliers);
}

float MagicSystem::TribalPower(const Spell& spell) const
{
	return spell.caster.kind == SpellCaster::Kind::Player ? PlayerTribalPower(spell.caster.player, spell.magicType) : 1.0f;
}

float MagicSystem::SeedPower(const Spell& spell) const
{
	const auto& registry = EntityRegistry();
	if (spell.seed != entt::null && registry.Valid(spell.seed))
	{
		if (const auto* seed = registry.TryGet<const SpellSeed>(spell.seed))
		{
			return seed->power;
		}
	}
	return 1.0f;
}

void MagicSystem::AddEffectTarget(const Spell& spell, entt::entity target)
{
	if (spell.effect != ParticleSystemInterface::k_NoEffect)
	{
		Locator::particleSystem::value().AddTarget(spell.effect, target);
	}
}

std::optional<entt::entity> MagicSystem::ShieldAt(glm::vec3 point, float margin)
{
	const auto shield = Locator::particleSystem::value().FindShield(point, margin);
	if (!shield || shield->spell == entt::null)
	{
		return std::nullopt;
	}
	return shield->spell;
}

void MagicSystem::StrikeShield(entt::entity shieldSpell, glm::vec3 point)
{
	if (const auto shield = Locator::particleSystem::value().FindShield(point, k_ShieldStrikeMargin);
	    shield && shield->spell == shieldSpell)
	{
		particles::StrikeShield(*shield, point);
	}
}

Spell* MagicSystem::FindSpell(entt::entity spell)
{
	auto& registry = EntityRegistry();
	return registry.Valid(spell) ? registry.TryGet<Spell>(spell) : nullptr;
}

float MagicSystem::GameRandom(float max)
{
	return Locator::gameRandom::has_value() ? Locator::gameRandom::value().GameFloatRand(max) : 0.0f;
}

entt::entity MagicSystem::CastAtPoint(MagicType type, PlayerNames player, glm::vec3 point, const magic::SpellCastData& cast,
                                      const particles::ProcessInfo& info)
{
	if (!Locator::infoConstants::has_value() || static_cast<size_t>(type) >= magic::k_MagicTypeCount || type == MagicType::None)
	{
		return entt::null;
	}
	auto& registry = EntityRegistry();
	const auto entity = registry.Create();
	auto& spell = registry.Assign<Spell>(entity);
	spell.magicType = type;
	spell.spellClass = magic::ClassOf(type);
	spell.caster = {.kind = SpellCaster::Kind::Player, .player = player, .entity = entt::null};
	spell.humanCasting = player == PlayerNames::PLAYER_ONE;
	magic::SetChants(spell.chants, cast.chants);
	spell.duration = cast.duration;
	spell.magnitude = cast.magnitude;
	spell.maxObjectsToCreate = cast.maxObjectsToCreate;
	spell.position = point;
	spell.castPosition = point;
	spell.originalCastPosition = point;
	spell.direction = info.direction;
	spell.processInfo = info;
	magic::spells::Prepare(*this, spell);

	auto link = std::make_unique<SpellLink>(*this, entity);
	const auto particleType = magic::GetMagicInfo(Info(), type).particleType;
	auto& particles = Locator::particleSystem::value();
	spell.effect = particles.StartForSpell(particleType, point, spell.direction, spell.magnitude, *link);
	if (spell.effect != ParticleSystemInterface::k_NoEffect)
	{
		particles.SetPlayer(spell.effect, static_cast<int>(player));
		if (auto* effect = particles.Find(spell.effect))
		{
			effect->SetProcessInfo(spell.processInfo);
		}
	}
	_links.insert_or_assign(entity, std::move(link));
	_spells.push_front(entity);
	if (!magic::spells::Start(*this, registry.Get<Spell>(entity)))
	{
		Delete(entity);
		return entt::null;
	}
	return entity;
}

entt::entity MagicSystem::CastOnObject(MagicType type, PlayerNames player, entt::entity target,
                                       const magic::SpellCastData& cast, const particles::ProcessInfo& info)
{
	const auto position = _world.PositionOf(target);
	if (!position.has_value() || !CanCastOn(type, target))
	{
		return entt::null;
	}
	const auto entity = CastAtPoint(type, player, *position, cast, info);
	if (entity == entt::null)
	{
		return entt::null;
	}
	auto& spell = EntityRegistry().Get<Spell>(entity);
	spell.target = target;
	AddEffectTarget(spell, target);
	if (spell.spellClass == magic::SpellClass::Creature)
	{
		ReceiveCreatureSpell(target, entity, spell);
	}
	return entity;
}

bool MagicSystem::CanCastOn(MagicType type, entt::entity target) const
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(target))
	{
		return false;
	}
	// The creature spells are cast on creatures only
	if (magic::ClassOf(type) == magic::SpellClass::Creature)
	{
		return registry.AllOf<Creature>(target) && creature_spells::SpellOf(type).has_value();
	}
	return true;
}

void MagicSystem::ReceiveCreatureSpell(entt::entity creature, entt::entity miracle, Spell& spell)
{
	auto& registry = EntityRegistry();
	const auto which = creature_spells::SpellOf(spell.magicType);
	if (!which.has_value() || !registry.AllOf<Creature>(creature))
	{
		return;
	}
	auto* component = registry.TryGet<CreatureSpells>(creature);
	if (component == nullptr)
	{
		component = &registry.Assign<CreatureSpells>(creature);
	}
	// The creature holds the spell for the miracle's time, made longer by the caster's tribal power; the miracle itself
	// then runs until the creature lets it go
	const float seconds = spell.duration > 0.0f ? spell.duration * TribalPower(spell) : spell.duration;
	const auto result =
	    creature_spells::Receive(component->spells, *which, creature_spells::TurnsOf(seconds, k_TurnsPerSecond), miracle);
	spell.duration = magic::k_NoTimeLimit;
	if (result.replaced != entt::null && result.replaced != miracle)
	{
		CloseDown(result.replaced);
	}
}

void MagicSystem::ProcessCreatureSpells()
{
	auto& registry = EntityRegistry();
	std::array<creature_spells::Timing, creature_spells::k_SpellCount> timings {};
	for (size_t i = 0; i < timings.size(); ++i)
	{
		const auto type = static_cast<MagicType>(static_cast<size_t>(MagicType::CreatureSpellFreeze) + i);
		if (const auto* info = magic::GetMagicInfoAs<GMagicCreatureSpellInfo>(Info(), type))
		{
			timings.at(i) = {.startSeconds = info->startTransitionDuration, .finishSeconds = info->finishTransitionDuration};
		}
	}
	std::vector<entt::entity> creatures;
	registry.Each<const CreatureSpells>([&](entt::entity entity, const CreatureSpells&) { creatures.push_back(entity); });
	for (const auto entity : creatures)
	{
		auto& component = registry.Get<CreatureSpells>(entity);
		const auto turn = creature_spells::Step(component.spells, timings, k_TurnsPerSecond);
		for (const auto& event : turn.events)
		{
			ApplyCreatureSpell(entity, event);
		}
		for (const auto miracle : turn.ended)
		{
			CloseDown(miracle);
		}
	}
}

void MagicSystem::ApplyCreatureSpell(entt::entity entity, const creature_spells::TurnEvent& event)
{
	using creature_spells::Event;
	using creature_spells::Spell;
	auto& registry = EntityRegistry();
	auto* creature = registry.TryGet<Creature>(entity);
	auto& component = registry.Get<CreatureSpells>(entity);
	if (creature == nullptr)
	{
		return;
	}
	auto& slot = component.spells[event.spell];
	const auto effect = creature_spells::EffectOf(event.spell);
	// The body value a spell pulls, if it pulls one
	float* value = nullptr;
	switch (event.spell)
	{
	case Spell::Small:
	case Spell::Big:
		value = &creature->size;
		break;
	case Spell::Weak:
	case Spell::Strong:
		value = &creature->strength;
		break;
	case Spell::Fat:
	case Spell::Thin:
		value = &creature->fatness;
		break;
	case Spell::Nice:
	case Spell::Nasty:
		value = &creature->alignment;
		break;
	default:
		break;
	}
	auto* desires = registry.TryGet<CreatureMindState>(entity);
	auto* animation = registry.TryGet<CreatureAnimation>(entity);
	const auto desireOf = [&]() -> creature_desires::DesireState* {
		if (!effect.desire.has_value() || desires == nullptr || !desires->desires.has_value())
		{
			return nullptr;
		}
		return &(*desires->desires)[static_cast<creature_desires::Desire>(*effect.desire)];
	};
	switch (event.event)
	{
	case Event::Start:
		if (value != nullptr)
		{
			slot.before = *value;
		}
		if (event.spell == Spell::Freeze)
		{
			// Frozen where it stands, its mind still
			if (desires != nullptr && !desires->paused)
			{
				desires->paused = true;
				component.pausedMind = true;
			}
			if (Locator::creatureLocomotionSystem::has_value())
			{
				Locator::creatureLocomotionSystem::value().Stop(entity);
			}
		}
		if (event.spell == Spell::Invisible)
		{
			component.invisible = true;
			component.fizz = 0.0f;
		}
		if (auto* desire = desireOf())
		{
			// It wants this above all else
			desire->activated = true;
			desire->value = desire->max;
		}
		if (effect.soundAction != 0 && Locator::audio::has_value())
		{
			const std::array<int32_t, 5> keys {0, 0, 0, 0, effect.soundAction};
			if (const auto* transform = registry.TryGet<const Transform>(entity))
			{
				Locator::audio::value().PlayAnimEffect(std::string(creature_audio::k_GenericBank), keys, entity,
				                                       transform->position);
			}
		}
		break;
	case Event::Ease:
		if (value != nullptr)
		{
			const float target = event.spell == Spell::Small || event.spell == Spell::Big
			                         ? creature_spells::SizeTarget(event.spell, slot.before, creature_locomotion::k_MinSize,
			                                                       creature_locomotion::k_MaxSize)
			                         : creature_spells::Target(event.spell).value_or(slot.before);
			*value = creature_spells::Ease(slot.before, target, event.ratio);
		}
		if (event.spell == Spell::Freeze)
		{
			component.freeze = event.ratio;
			if (animation != nullptr)
			{
				animation->playbackScale = 1.0f - event.ratio;
			}
		}
		if (event.spell == Spell::Invisible)
		{
			component.fizz = creature_spells::k_InvisibleFizz * event.ratio;
		}
		break;
	case Event::Hold:
		if (auto* desire = desireOf())
		{
			desire->value = desire->max;
		}
		// An itchy creature won't be led
		if (event.spell == Spell::Itchy && Locator::leashSystem::has_value())
		{
			Locator::leashSystem::value().SetWorks(entity, false);
		}
		break;
	case Event::BeginFinish:
		break;
	case Event::Finish:
		if (value != nullptr)
		{
			*value = slot.before;
		}
		if (event.spell == Spell::Freeze)
		{
			component.freeze = 0.0f;
			if (animation != nullptr)
			{
				animation->playbackScale = 1.0f;
			}
			if (desires != nullptr && component.pausedMind)
			{
				desires->paused = false;
			}
			component.pausedMind = false;
		}
		if (event.spell == Spell::Invisible)
		{
			component.invisible = false;
			component.fizz = 0.0f;
		}
		if (event.spell == Spell::Itchy && Locator::leashSystem::has_value())
		{
			Locator::leashSystem::value().SetWorks(entity, true);
		}
		if (auto* desire = desireOf())
		{
			// What it wanted most it now wants least of all
			float least = desire->max;
			for (const auto& other : desires->desires->desires)
			{
				if (&other != desire && other.activated)
				{
					least = std::min(least, other.value);
				}
			}
			desire->value = least / creature_spells::k_LeastDominantFactor;
		}
		break;
	}
}

void MagicSystem::CloseDown(entt::entity entity)
{
	auto* spell = FindSpell(entity);
	if (spell == nullptr)
	{
		return;
	}
	spell->closedDown = true;
	if (spell->effect != ParticleSystemInterface::k_NoEffect)
	{
		Locator::particleSystem::value().CloseDown(spell->effect);
	}
}

bool MagicSystem::CanCastAt(MagicType type, PlayerNames player, glm::vec3 point)
{
	return Locator::infoConstants::has_value() && magic::spells::CanCastAt(*this, type, player, point, _ignoreInfluence);
}

bool MagicSystem::OnSpellEvent(entt::entity entity, const particles::SpellEventInfo& event)
{
	auto* spell = FindSpell(entity);
	return spell != nullptr && magic::spells::OnEvent(*this, *spell, event);
}

particles::ProcessInfo MagicSystem::HandInfo() const
{
	return {
	    .handPosition = _hand.handPosition,
	    .cameraForward = _hand.cameraForward,
	    .direction = _handVelocity,
	    .power = 1.0f,
	    .enabled = true,
	    .spin = 0.0f,
	};
}

void MagicSystem::Maintain(entt::entity entity, Spell& spell)
{
	if (magic::AgeOneTurn(spell.age, spell.duration, k_TurnSeconds))
	{
		CloseDown(entity);
	}
	if (CasterOf(spell) == nullptr || (spell.target != entt::null && !EntityRegistry().Valid(spell.target)))
	{
		CloseDown(entity);
	}
	spell.processInfo.enabled = true;
	if (spell.castFromHand)
	{
		// A miracle held in the hand follows it
		const float power = spell.processInfo.power;
		spell.processInfo = HandInfo();
		spell.processInfo.power = power;
		const auto under = _hand.point.value_or(_hand.handPosition);
		spell.castPosition = {under.x, _world.LandHeight({under.x, under.z}), under.z};
	}
	if (!spell.closedDown)
	{
		const float strength = magic::spells::StrengthOf(*this, spell);
		magic::PayForOneTurn(spell.chants, magic::spells::RulesOf(*this, spell), CasterOf(spell));
		_grid.Mark({spell.position.x, spell.position.z});
		spell.processInfo.power = strength;
	}
	else
	{
		spell.processInfo.enabled = false;
		spell.processInfo.power = 0.0f;
	}
}

bool MagicSystem::Process(entt::entity entity, Spell& spell)
{
	if (!spell.closedDown)
	{
		if (auto* caster = CasterOf(spell))
		{
			magic::Recharge(spell.chants, magic::spells::RulesOf(*this, spell), *caster);
		}
		if (!(spell.processInfo.power > 0.0f))
		{
			CloseDown(entity);
		}
	}
	if (spell.effect != ParticleSystemInterface::k_NoEffect &&
	    !Locator::particleSystem::value().ProcessForSpell(spell.effect, spell.processInfo, k_TurnSeconds))
	{
		spell.effect = ParticleSystemInterface::k_NoEffect;
	}
	// The effect's events may have closed it down or cast others: look it up again
	auto* after = FindSpell(entity);
	if (after == nullptr)
	{
		return false;
	}
	magic::spells::ProcessTurn(*this, *after);
	return after->effect != ParticleSystemInterface::k_NoEffect;
}

void MagicSystem::Delete(entt::entity entity)
{
	auto& registry = EntityRegistry();
	if (auto* spell = FindSpell(entity))
	{
		if (spell->effect != ParticleSystemInterface::k_NoEffect)
		{
			Locator::particleSystem::value().Delete(spell->effect);
		}
		if (spell->seed != entt::null && registry.Valid(spell->seed))
		{
			if (auto* seed = registry.TryGet<SpellSeed>(spell->seed); seed != nullptr && seed->spell == entity)
			{
				seed->spell = entt::null;
			}
		}
		registry.Destroy(entity);
	}
	_links.erase(entity);
	std::erase(_spells, entity);
}

void MagicSystem::ProcessTurn()
{
	if (!Locator::infoConstants::has_value() || !Locator::particleSystem::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();
	_world.ProcessTurn();
	_grid.Fade();
	ProcessDispensers();
	ProcessCreatureSpells();

	// The held seed waits until it is ready, and its in-hand effect steps with the hand
	if (_held.has_value())
	{
		if (!registry.Valid(*_held))
		{
			_held.reset();
		}
		else
		{
			auto& seed = registry.Get<SpellSeed>(*_held);
			++seed.turnsInHand;
			seed.ready =
			    seed.ready || magic::SeedReadyAfter(seed.turnsInHand, k_TurnSeconds, Info().spellSystem.delayBeforeSeedActive);
			if (seed.handEffect != ParticleSystemInterface::k_NoEffect)
			{
				particles::ProcessInfo info = HandInfo();
				if (!Locator::particleSystem::value().ProcessForSpell(seed.handEffect, info, k_TurnSeconds))
				{
					seed.handEffect = ParticleSystemInterface::k_NoEffect;
				}
				_handAtStep = _hand.handPosition;
				Locator::particleSystem::value().SetDrawOffset(seed.handEffect, glm::vec3(0.0f));
			}
			// A held miracle whose prayer power or time ran out is over, and its seed with it
			if (_holding && seed.spell != entt::null)
			{
				const auto* spell = FindSpell(seed.spell);
				if (spell == nullptr || spell->closedDown)
				{
					_holding = false;
					LetGoOfSeed(true);
				}
			}
		}
	}

	for (const auto entity : std::vector<entt::entity>(_spells.begin(), _spells.end()))
	{
		if (auto* spell = FindSpell(entity))
		{
			Maintain(entity, *spell);
		}
	}
	for (const auto entity : std::vector<entt::entity>(_spells.begin(), _spells.end()))
	{
		auto* spell = FindSpell(entity);
		if (spell == nullptr || !Process(entity, *spell))
		{
			Delete(entity);
		}
	}
}

void MagicSystem::Update(float seconds)
{
	auto& registry = EntityRegistry();
	if (_held.has_value() && registry.Valid(*_held))
	{
		auto& transform = registry.Get<Transform>(*_held);
		transform.position = _hand.handPosition;
		const auto& seed = registry.Get<const SpellSeed>(*_held);
		if (seed.handEffect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
		{
			Locator::particleSystem::value().SetDrawOffset(seed.handEffect, _hand.handPosition - _handAtStep);
		}
	}
	// The seeds in the bubbles spin at their middles
	const auto bubble = BubbleMesh();
	auto& meshes = Locator::resources::value().GetMeshes();
	const glm::vec3 centre = meshes.Contains(bubble) ? meshes.Handle(bubble)->GetBoundingBox().Center() : glm::vec3(0.0f);
	const auto eye = Locator::camera::has_value() ? Locator::camera::value().GetOrigin() : glm::vec3(0.0f);
	registry.Each<OneOffSpellSeed, Transform>([&](entt::entity, OneOffSpellSeed& orb, Transform& transform) {
		orb.spin = std::fmod(orb.spin + magic::k_OrbSeedSpin * seconds, k_TwoPi);
		// The dome's top faces the camera, turning about the middle of the bubble
		const auto middle = orb.position + centre * transform.scale;
		transform.rotation = magic::FaceTowards(eye - middle);
		transform.position = middle - transform.rotation * (centre * transform.scale);
		if (orb.seedGraphic == entt::null || !registry.Valid(orb.seedGraphic))
		{
			return;
		}
		auto& graphic = registry.Get<Transform>(orb.seedGraphic);
		graphic.position = middle;
		graphic.rotation = glm::mat3(glm::eulerAngleY(orb.spin));
	});
}

void MagicSystem::Reset()
{
	_spells.clear();
	_links.clear();
	_held.reset();
	_readied = false;
	_holding = false;
	_lastHandPosition.reset();
	_handVelocity = glm::vec3(0.0f);
	_lastHandResult = HandResult::None;
	_world.Reset();
	_grid.Clear();
}

// Dispensers and one-shot miracles

entt::id_type MagicSystem::BubbleMesh() const
{
	auto& meshes = Locator::resources::value().GetMeshes();
	const auto id = k_BubbleMeshId.value();
	if (meshes.Contains(id) || !Locator::filesystem::has_value())
	{
		return id;
	}
	auto& fileSystem = Locator::filesystem::value();
	try
	{
		meshes.Load(id, resources::L3DLoader::FromDiskTag {},
		            fileSystem.FindPath(fileSystem.GetPath<filesystem::Path::Data>() / OneOffSpellSeed::k_MeshFile));
	}
	catch (const std::exception& error)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Magic: cannot load the one-shot miracle's bubble: {}", error.what());
	}
	return id;
}

entt::entity MagicSystem::CreateDispenser(glm::vec3 position, MagicType type, float yAngleRadians)
{
	if (!Locator::infoConstants::has_value())
	{
		return entt::null;
	}
	position.y = _world.LandHeight({position.x, position.z});
	const auto entity = archetypes::SpellDispenserArchetype::Create(position, type, k_DispenserBuilding, yAngleRadians, 1.0f);
	auto& dispenser = EntityRegistry().Get<SpellDispenser>(entity);
	if (Locator::particleSystem::has_value())
	{
		dispenser.effect = Locator::particleSystem::value().Start(ParticleType::SpelldispenserVortex, position, 1.0f, true);
	}
	// It floats its first bubble at once
	if (dispenser.timer.active)
	{
		MakeOrb(entity);
	}
	return entity;
}

void MagicSystem::SetDispenserPeriod(entt::entity dispenser, float seconds)
{
	if (auto* component = EntityRegistry().TryGet<SpellDispenser>(dispenser))
	{
		component->timer.period = magic::PeriodTurns(seconds, TimeSystemInterface::k_TurnDuration);
		component->timer.active = component->timer.period != 0;
	}
}

entt::entity MagicSystem::CreateOneOffSeed(glm::vec3 position, SpellSeedType seed, int powerUp, float multiplier)
{
	return archetypes::OneOffSpellSeedArchetype::Create(position, seed, powerUp, multiplier, BubbleMesh());
}

entt::entity MagicSystem::CreateOneOffSeedFor(glm::vec3 position, MagicType type)
{
	const auto seed = magic::FindFirstSpellSeedForMagicType(Info(), type);
	if (!seed.has_value())
	{
		return entt::null;
	}
	const auto step = magic::GetPowerUpGesture(magic::GetSpellSeedInfo(Info(), *seed), type);
	const auto orb = CreateOneOffSeed(position, *seed, step.level, 1.0f);
	if (orb != entt::null)
	{
		EntityRegistry().Get<OneOffSpellSeed>(orb).magicType = type;
	}
	return orb;
}

entt::entity MagicSystem::MakeOrb(entt::entity entity)
{
	auto& registry = EntityRegistry();
	auto& dispenser = registry.Get<SpellDispenser>(entity);
	const auto& transform = registry.Get<const Transform>(entity);
	const auto& mesh = registry.Get<const Mesh>(entity);
	const auto position = magic::OrbPosition(transform.position, HeightOfMesh(mesh.id, transform.scale.y));
	const auto magicType = dispenser.magicType;
	const auto orb = CreateOneOffSeedFor(position, magicType);
	auto& after = registry.Get<SpellDispenser>(entity);
	after.orb = orb;
	after.orbPosition = position;
	if (orb != entt::null)
	{
		registry.Get<OneOffSpellSeed>(orb).dispenser = entity;
		Locator::particleSystem::value().StartSpotVisual(SpotVisualType::MagicObjectCreated, position, std::nullopt,
		                                                 entt::null);
	}
	return orb;
}

void MagicSystem::ProcessDispensers()
{
	auto& registry = EntityRegistry();
	std::vector<entt::entity> making;
	registry.Each<SpellDispenser>([&](entt::entity entity, SpellDispenser& dispenser) {
		const bool hasOrb = dispenser.orb != entt::null;
		bool stillThere = false;
		if (hasOrb && registry.Valid(dispenser.orb))
		{
			if (const auto* orb = registry.TryGet<const OneOffSpellSeed>(dispenser.orb))
			{
				stillThere = magic::OrbStillThere(orb->position, dispenser.orbPosition);
			}
		}
		switch (magic::StepDispenser(dispenser.timer, hasOrb, stillThere, dispenser.magicType != MagicType::None))
		{
		case magic::DispenserStep::OrbTaken:
			dispenser.orb = entt::null;
			break;
		case magic::DispenserStep::MakeOrb:
			making.push_back(entity);
			break;
		case magic::DispenserStep::Wait:
			break;
		}
	});
	for (const auto entity : making)
	{
		MakeOrb(entity);
	}
}

bool MagicSystem::Remove(entt::entity entity)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(entity))
	{
		return false;
	}
	if (const auto* dispenser = registry.TryGet<const SpellDispenser>(entity))
	{
		if (Locator::particleSystem::has_value() && dispenser->effect != 0)
		{
			Locator::particleSystem::value().Delete(dispenser->effect);
		}
		DestroyOrb(dispenser->orb);
		registry.Destroy(entity);
		return true;
	}
	if (registry.AllOf<OneOffSpellSeed>(entity))
	{
		DestroyOrb(entity);
		return true;
	}
	return false;
}

void MagicSystem::DestroyOrb(entt::entity orb)
{
	auto& registry = EntityRegistry();
	if (!registry.Valid(orb))
	{
		return;
	}
	if (const auto* component = registry.TryGet<const OneOffSpellSeed>(orb);
	    component != nullptr && component->seedGraphic != entt::null && registry.Valid(component->seedGraphic))
	{
		registry.Destroy(component->seedGraphic);
	}
	registry.Destroy(orb);
}

entt::entity MagicSystem::GiveSeedToHand(PlayerNames player, SpellSeedType seedType, int powerUp, float multiplier)
{
	if (_held.has_value() || !Locator::infoConstants::has_value())
	{
		return entt::null;
	}
	const auto entity = archetypes::SpellSeedArchetype::Create(_hand.handPosition, seedType, player, powerUp, multiplier);
	if (entity == entt::null)
	{
		return entt::null;
	}
	auto& registry = EntityRegistry();
	auto& seed = registry.Get<SpellSeed>(entity);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seedType);
	const auto type = magic::GetMagicTypeFromPowerUpLevel(seedInfo, powerUp);
	// Charged in full, for free, and ready at once
	seed.chantStore = magic::ChantNeeded(magic::GetChantsRequiredToCreate(Info(), type), 0.0f);
	seed.ready = true;
	if (magic::GetMagicInfo(Info(), type).isSpellSeedDrawnInHand == 1)
	{
		registry.Assign<Mesh>(entity, resources::HashIdentifier(seedInfo.mesh), static_cast<int8_t>(0), static_cast<int8_t>(0));
	}
	_held = entity;
	StartHandEffect(entity);
	return entity;
}

bool MagicSystem::TapOrb(entt::entity orb)
{
	auto& registry = EntityRegistry();
	const auto component = registry.Get<const OneOffSpellSeed>(orb);
	const auto position = component.position;
	if (GiveSeedToHand(PlayerNames::PLAYER_ONE, component.seedType, component.powerUp, component.multiplier) == entt::null)
	{
		return false;
	}
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_SpellBubblePop_04), position);
	}
	DestroyOrb(orb);
	_lastHandResult = HandResult::TookMiracle;
	return true;
}

std::optional<entt::entity> MagicSystem::OrbAlong(glm::vec3 origin, glm::vec3 direction) const
{
	const auto& registry = EntityRegistry();
	auto& meshes = Locator::resources::value().GetMeshes();
	const auto bubble = k_BubbleMeshId.value();
	glm::vec3 centre(0.0f);
	float radius = k_DefaultOrbRadius;
	if (meshes.Contains(bubble))
	{
		const auto box = meshes.Handle(bubble)->GetBoundingBox();
		centre = box.Center();
		radius = std::max({box.Size().x, box.Size().y, box.Size().z}) * 0.5f;
	}
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	registry.Each<const OneOffSpellSeed, const Transform>(
	    [&](entt::entity entity, const OneOffSpellSeed& orb, const Transform& transform) {
		    const auto middle = orb.position + centre * transform.scale;
		    const float reach = radius * transform.scale.x * k_OrbTapLeeway;
		    const float along = glm::dot(middle - origin, direction);
		    if (along <= 0.0f)
		    {
			    return;
		    }
		    const auto closest = origin + direction * along;
		    if (glm::distance(closest, middle) < reach && along < best)
		    {
			    best = along;
			    nearest = entity;
		    }
	    });
	return nearest;
}

// The hand

void MagicSystem::UpdateHand(const HandFrame& frame, float seconds)
{
	if (_lastHandPosition.has_value() && seconds > 0.0f)
	{
		const auto moved = (frame.handPosition - *_lastHandPosition) / seconds;
		_handVelocity = glm::mix(_handVelocity, moved, std::min(1.0f, seconds / k_HandSpeedSmoothing));
	}
	_lastHandPosition = frame.handPosition;
	_hand = frame;
}

void MagicSystem::StartHandEffect(entt::entity entity)
{
	auto& registry = EntityRegistry();
	auto& seed = registry.Get<SpellSeed>(entity);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	const auto type = magic::GetMagicTypeFromPowerUpLevel(seedInfo, seed.powerUp);
	auto& particles = Locator::particleSystem::value();
	seed.handEffect = particles.StartForSpell(magic::GetMagicInfo(Info(), type).particleTypeInHand, _hand.handPosition,
	                                          glm::vec3(0.0f), 1.0f, *_handEffectLink);
	if (seed.handEffect != ParticleSystemInterface::k_NoEffect)
	{
		// Drawn at once just after the hand, rather than sorted with everything else
		particles.SetDrawPath(seed.handEffect, particles::draw::DrawPath::Immediate);
		particles.SetPlayer(seed.handEffect, static_cast<int>(seed.player));
		if (auto* effect = particles.Find(seed.handEffect))
		{
			effect->SetProcessInfo(HandInfo());
		}
		_handAtStep = _hand.handPosition;
	}
}

void MagicSystem::StopHandEffect(SpellSeed& seed)
{
	if (seed.handEffect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().Delete(seed.handEffect);
	}
	seed.handEffect = ParticleSystemInterface::k_NoEffect;
}

void MagicSystem::LetGoOfSeed(bool destroy)
{
	auto& registry = EntityRegistry();
	if (_held.has_value() && registry.Valid(*_held))
	{
		auto& seed = registry.Get<SpellSeed>(*_held);
		StopHandEffect(seed);
		if (destroy)
		{
			if (auto* spell = FindSpell(seed.spell); spell != nullptr && spell->seed == *_held)
			{
				spell->seed = entt::null;
			}
			registry.Destroy(*_held);
		}
	}
	_held.reset();
	_readied = false;
	_holding = false;
}

entt::entity MagicSystem::CastHeldSeed()
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !_hand.point.has_value())
	{
		return entt::null;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	const auto type = magic::GetMagicTypeFromPowerUpLevel(seedInfo, seed.powerUp);
	const auto point = *_hand.point;
	std::optional<entt::entity> target;
	if (seedInfo.castOnObject != 0)
	{
		if (Locator::creatureHandSystem::has_value())
		{
			target = Locator::creatureHandSystem::value().CreatureAlong(_hand.rayOrigin, _hand.rayDirection);
		}
		if (!target.has_value() || !CanCastOn(type, *target))
		{
			return entt::null;
		}
	}
	else if (!CanCastAt(type, seed.player, point))
	{
		return entt::null;
	}
	// Without a gesture to size it, a miracle that wants one takes its usual size
	float size = k_PlainHandMagnitude;
	if (const auto* radius = magic::GetMagicInfoAs<GMagicRadiusSpellInfo>(Info(), type))
	{
		size = radius->radiusForNormalCost;
	}
	auto cast = magic::SeedCastData(Info(), type, seed.seedType, seed.castMultiplier, size);
	cast.maxObjectsToCreate = seed.storedMaxObjects;
	const bool resuming = seed.storedChants >= 0.0f;
	if (resuming)
	{
		cast.chants = seed.storedChants;
	}
	const auto heldEntity = *_held;
	const auto spellEntity = target.has_value() ? CastOnObject(type, seed.player, *target, cast, HandInfo())
	                                            : CastAtPoint(type, seed.player, point, cast, HandInfo());
	if (spellEntity == entt::null)
	{
		return entt::null;
	}
	auto& spell = registry.Get<Spell>(spellEntity);
	auto& after = registry.Get<SpellSeed>(heldEntity);
	spell.castFromHand = true;
	spell.seed = heldEntity;
	if (resuming)
	{
		spell.age = after.storedAge;
	}
	after.spell = spellEntity;
	after.hasCast = true;
	after.storedChants = -1.0f;
	return spellEntity;
}

void MagicSystem::FailCast()
{
	const auto point = _hand.point.value_or(_hand.handPosition);
	if (Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().StartSpotVisual(SpotVisualType::SpellFailCast, point, std::nullopt, entt::null);
	}
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_SpellCastFailure), std::nullopt);
	}
	_lastHandResult = HandResult::CantCastThere;
}

bool MagicSystem::PressAction()
{
	if (!_hand.overWorld || !Locator::infoConstants::has_value())
	{
		return false;
	}
	auto& registry = EntityRegistry();
	if (!_held.has_value())
	{
		if (const auto orb = OrbAlong(_hand.rayOrigin, _hand.rayDirection))
		{
			return TapOrb(*orb);
		}
		return false;
	}
	const auto& seed = registry.Get<const SpellSeed>(*_held);
	if (!seed.ready)
	{
		_lastHandResult = HandResult::NotReady;
		return true;
	}
	switch (magic::CastStyleOf(magic::GetSpellSeedInfo(Info(), seed.seedType)))
	{
	case magic::CastStyle::OnPress:
		if (CastHeldSeed() != entt::null)
		{
			Locator::particleSystem::value().StartSpotVisual(SpotVisualType::SpellSucceedCast, *_hand.point, std::nullopt,
			                                                 entt::null);
			_lastHandResult = HandResult::Cast;
			LetGoOfSeed(true);
		}
		else
		{
			FailCast();
		}
		break;
	case magic::CastStyle::OnRelease:
		_readied = true;
		_lastHandResult = HandResult::Readied;
		break;
	case magic::CastStyle::Held:
		if (CastHeldSeed() != entt::null)
		{
			_holding = true;
			_lastHandResult = HandResult::CastHeld;
		}
		else
		{
			FailCast();
		}
		break;
	}
	return true;
}

void MagicSystem::ReleaseAction()
{
	if (_readied)
	{
		_readied = false;
		if (CastHeldSeed() != entt::null)
		{
			Locator::particleSystem::value().StartSpotVisual(SpotVisualType::SpellSucceedCast, *_hand.point, std::nullopt,
			                                                 entt::null);
			_lastHandResult = HandResult::Cast;
			LetGoOfSeed(true);
		}
		else
		{
			FailCast();
		}
	}
	if (_holding)
	{
		StopHeldMiracle();
	}
}

void MagicSystem::StopHeldMiracle()
{
	_holding = false;
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return;
	}
	auto& seed = registry.Get<SpellSeed>(*_held);
	bool enough = true;
	if (auto* spell = FindSpell(seed.spell))
	{
		// The seed keeps what the miracle has left, and how long it has run, for another go
		seed.storedChants = spell->chants.chants;
		seed.storedAge = spell->age;
		seed.storedMaxObjects = spell->maxObjectsToCreate;
		enough = !spell->closedDown && magic::spells::HasEnoughForRecast(Info(), *spell);
		spell->seed = entt::null;
		CloseDown(seed.spell);
	}
	seed.spell = entt::null;
	_lastHandResult = HandResult::Released;
	if (magic::GetSpellSeedInfo(Info(), seed.seedType).deleteSeedOnceCast != 0 || !enough)
	{
		LetGoOfSeed(true);
	}
}

void MagicSystem::DiscardHeldSeed()
{
	if (!_held.has_value())
	{
		return;
	}
	if (_holding)
	{
		StopHeldMiracle();
	}
	LetGoOfSeed(true);
	_lastHandResult = HandResult::Discarded;
}

// For the debug window

std::vector<MagicSystemInterface::SpellInfo> MagicSystem::GetSpells() const
{
	std::vector<SpellInfo> result;
	if (!Locator::infoConstants::has_value())
	{
		return result;
	}
	auto& self = const_cast<MagicSystem&>(*this);
	for (const auto entity : _spells)
	{
		const auto* spell = EntityRegistry().TryGet<const Spell>(entity);
		if (spell == nullptr)
		{
			continue;
		}
		result.push_back({
		    .entity = entity,
		    .magicType = spell->magicType,
		    .player = spell->caster.player,
		    .age = spell->age,
		    .duration = spell->duration,
		    .chants = spell->chants.chants,
		    .initialChants = spell->chants.initialChants,
		    .strength = magic::spells::StrengthOf(self, *spell),
		    .upkeep = magic::spells::CostToMaintain(Info(), *spell),
		    .closing = spell->closedDown,
		    .fromHand = spell->castFromHand,
		    .effect = spell->effect,
		    .position = spell->position,
		});
	}
	return result;
}

std::vector<MagicSystemInterface::DispenserInfo> MagicSystem::GetDispensers() const
{
	std::vector<DispenserInfo> result;
	EntityRegistry().Each<const SpellDispenser, const Transform>(
	    [&](entt::entity entity, const SpellDispenser& dispenser, const Transform& transform) {
		    result.push_back({
		        .entity = entity,
		        .magicType = dispenser.magicType,
		        .position = transform.position,
		        .hasOrb = dispenser.orb != entt::null,
		        .tick = dispenser.timer.tick,
		        .period = dispenser.timer.period,
		        .active = dispenser.timer.active,
		    });
	    });
	return result;
}
