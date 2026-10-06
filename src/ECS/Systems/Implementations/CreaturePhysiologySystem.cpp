/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreaturePhysiologySystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <numbers>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "3D/CreatureBody.h"
#include "3D/DayNightClock.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "Creature/CreaturePhysiology.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Archetypes/MobileObjectArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
namespace physiology = openblack::creature_physiology;

namespace
{
constexpr float k_TurnsPerSecond = 1.0f / std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
/// Without a stage of growing up to go by, a creature's body works as a grown up's does
constexpr uint32_t k_GrownUpPhase = 13;
/// The time scale can't run more than this many turns of the body in a game turn
constexpr float k_MaxTimeScale = 3600.0f;

/// The poo lump is the size of the creature by this, and drops this far behind it for its size
constexpr float k_PooScale = 1.5f;
constexpr float k_PooBehind = 3.0f;
/// Sick comes out as this many drops from the creature's mouth, about this high and this far ahead for its size, flung
/// forwards and up, a little to either side, and lies on the land a while before it is gone
constexpr int k_PukeDrops = 12;
constexpr float k_MouthHeight = 9.0f;
constexpr float k_MouthAhead = 3.0f;
constexpr float k_PukeSpeed = 5.0f;
constexpr float k_PukeSpread = 2.0f;
constexpr float k_PukeDropScale = 0.5f;
constexpr float k_PukeSeconds = 4.0f;
constexpr float k_PukeFadeSeconds = 1.5f;
constexpr float k_Gravity = 9.81f;
/// The sick drops are a frame of the land effects' sprite sheet, in greenish colours
constexpr entt::hashed_string k_PukeSpriteSheet = entt::hashed_string("raw/S_SpriteSheet3a");
constexpr float k_PukeSheetCell = 1.0f / 8.0f;

const GCreatureInfo* SpeciesInfo(CreatureType species)
{
	if (!Locator::infoConstants::has_value())
	{
		return nullptr;
	}
	const auto& creatures = Locator::infoConstants::value().creature;
	const auto row = creature::InfoRow(species);
	return row < creatures.size() ? &creatures.at(row) : nullptr;
}

/// What the species' body is like, from its row of the game's creature table
physiology::Species SpeciesOf(CreatureType species)
{
	const auto* info = SpeciesInfo(species);
	if (info == nullptr)
	{
		return {};
	}
	return {
	    .startEnergy = info->startEnergy,
	    .startWarmth = info->startWarmth,
	    .comfortTemperature = info->comfortTemperature,
	    .secondsPerAgeTick = info->secondsPerAgeTick,
	    .growUpMinutes = info->growUpMinutes,
	    .lowEnergyThreshold = info->lowEnergyThreshold,
	    .exhaustionRate = info->exhaustionRate,
	    .secondsToDehydrate = info->secondsToDehydrate,
	    .strengthDecay = info->strengthDecay,
	    .carryStrengthMinutes = info->carryStrengthMinutes,
	    .energyDrain = info->energyDrain,
	    .fatBurn = info->fatBurn,
	    .overeatFatFactor = info->overeatFatFactor,
	    .sleepHeal = info->sleepHeal,
	    .sleepRecover = info->sleepRecover,
	    .sleepLength = info->sleepLength,
	    .foodToEnergy = info->foodToEnergy,
	    .pooPerEnergy = info->pooPerEnergy,
	};
}

uint32_t PhaseOf(ecs::Registry& registry, entt::entity entity)
{
	const auto* mind = registry.TryGet<const CreatureMindState>(entity);
	return mind != nullptr ? mind->developmentPhase : k_GrownUpPhase;
}

bool IsNight()
{
	return Locator::skySystem::has_value() && Locator::skySystem::value().GetClock().IsVisualNight();
}

float TemperatureAt(const glm::vec3& position)
{
	if (!Locator::weatherSystem::has_value())
	{
		return 0.0f;
	}
	return static_cast<float>(Locator::weatherSystem::value().GetWeather(position).temperature);
}

float HeightAt(glm::vec2 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
}

bool IsMoving(const CreatureLocomotion* locomotion)
{
	using Motion = CreatureLocomotion::Motion;
	return locomotion != nullptr && (locomotion->motion == Motion::Walking || locomotion->motion == Motion::Stepping ||
	                                 locomotion->motion == Motion::Turning);
}

/// The creature's body as its needs see it, and back
physiology::Shape ShapeOf(const Creature& creature)
{
	return {.fatness = creature.fatness, .strength = creature.strength, .size = creature.size};
}

/// Takes on a changed shape; returns whether its drawn size changed
bool TakeShape(Creature& creature, Transform* transform, const physiology::Shape& shape)
{
	creature.fatness = shape.fatness;
	creature.strength = shape.strength;
	if (shape.size == creature.size)
	{
		return false;
	}
	creature.size = shape.size;
	if (transform != nullptr)
	{
		transform->scale = glm::vec3(ecs::archetypes::CreatureArchetype::DrawnScale(creature.species, creature.size));
	}
	return true;
}

void StartNeeds(CreatureNeeds& needs, const physiology::Species& species)
{
	if (!needs.started)
	{
		needs.needs = physiology::Start(species);
		needs.started = true;
	}
}

/// Where the creature faces on the land, for its rotation; its mesh looks back along +z
glm::vec3 AheadOf(const Transform& transform)
{
	auto ahead = -(transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	ahead.y = 0.0f;
	return glm::length(ahead) > 0.0f ? glm::normalize(ahead) : glm::vec3(0.0f, 0.0f, -1.0f);
}
} // namespace

void CreaturePhysiologySystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	_owedTurns += _timeScale;
	const auto turns = static_cast<uint32_t>(std::floor(_owedTurns));
	_owedTurns -= static_cast<float>(turns);
	const bool night = IsNight();
	bool resized = false;

	registry.Each<Creature, CreatureNeeds>([&](entt::entity entity, Creature& creature, CreatureNeeds& needs) {
		const auto species = SpeciesOf(creature.species);
		StartNeeds(needs, species);
		auto* transform = registry.TryGet<Transform>(entity);
		needs.moving = IsMoving(registry.TryGet<const CreatureLocomotion>(entity));
		const auto phase = PhaseOf(registry, entity);
		if (needs.rest == CreatureNeeds::Rest::Awake)
		{
			needs.restTurns = 0;
			needs.rested = false;
		}

		const physiology::Turn turn {
		    .moving = needs.moving,
		    .asleep = needs.rest == CreatureNeeds::Rest::Asleep,
		    .resting = needs.rest == CreatureNeeds::Rest::Resting,
		    .phase = phase,
		    .carriedWeight = needs.carriedWeight,
		    .temperature = transform != nullptr ? TemperatureAt(transform->position) : 0.0f,
		    .turnsPerSecond = k_TurnsPerSecond,
		};
		auto shape = ShapeOf(creature);
		for (uint32_t i = 0; i < turns; ++i)
		{
			physiology::TickTurn(needs.needs, shape, species, turn);
			// Asleep or resting, it heals and rests, and may be ready to wake
			if (turn.asleep || turn.resting)
			{
				++needs.restTurns;
				needs.rested = physiology::SleepTurn(needs.needs, species, shape.size, needs.restTurns, night) || needs.rested;
			}
		}
		resized = TakeShape(creature, transform, shape) || resized;

		needs.faint.reset();
		if (_fainting && needs.rest != CreatureNeeds::Rest::Unconscious)
		{
			needs.faint = physiology::ShouldFaint(needs.needs, phase, creature.owner != PlayerNames::NEUTRAL);
		}
	});
	if (resized)
	{
		registry.SetDirty();
	}
}

void CreaturePhysiologySystem::Update(float seconds)
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> gone;
	registry.Each<CreaturePukeDrop, Sprite, Transform>(
	    [&](entt::entity entity, CreaturePukeDrop& drop, Sprite& sprite, Transform& transform) {
		    drop.seconds += seconds;
		    if (drop.seconds >= k_PukeSeconds)
		    {
			    gone.push_back(entity);
			    return;
		    }
		    const auto ground = HeightAt(glm::vec2(transform.position.x, transform.position.z));
		    if (transform.position.y > ground)
		    {
			    drop.velocity.y -= k_Gravity * seconds;
			    transform.position += drop.velocity * seconds;
			    transform.position.y = std::max(transform.position.y, ground);
		    }
		    // It lies where it landed, fading at the end
		    const auto left = k_PukeSeconds - drop.seconds;
		    sprite.tint.a = std::clamp(left / k_PukeFadeSeconds, 0.0f, 1.0f);
	    });
	for (const auto entity : gone)
	{
		registry.Destroy(entity);
	}
	if (!gone.empty() || registry.Size<CreaturePukeDrop>() > 0)
	{
		registry.SetDirty();
	}
}

void CreaturePhysiologySystem::Eat(entt::entity creature, float foodValue)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* self = registry.TryGet<Creature>(creature);
	auto* needs = registry.TryGet<CreatureNeeds>(creature);
	if (self == nullptr || needs == nullptr)
	{
		return;
	}
	const auto species = SpeciesOf(self->species);
	StartNeeds(*needs, species);
	auto shape = ShapeOf(*self);
	physiology::Eat(needs->needs, shape, species, foodValue);
	if (TakeShape(*self, registry.TryGet<Transform>(creature), shape))
	{
		registry.SetDirty();
	}
}

void CreaturePhysiologySystem::Drink(entt::entity creature)
{
	if (auto* needs = Locator::entitiesRegistry::value().TryGet<CreatureNeeds>(creature))
	{
		physiology::Drink(needs->needs);
	}
}

void CreaturePhysiologySystem::Poo(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* needs = registry.TryGet<CreatureNeeds>(creature);
	const auto* self = registry.TryGet<const Creature>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (needs == nullptr || self == nullptr || transform == nullptr)
	{
		return;
	}
	physiology::Poo(needs->needs);
	// The lump drops behind it, turned any way. The game throws it back along the ground; here it lands there.
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	const auto behind = transform->position - (AheadOf(*transform) * (k_PooBehind * self->size));
	const auto point = glm::vec3(behind.x, HeightAt(glm::vec2(behind.x, behind.z)), behind.z);
	const auto yaw = std::uniform_real_distribution<float>(0.0f, 2.0f * std::numbers::pi_v<float>)(_random);
	ecs::archetypes::MobileObjectArchetype::Create(point, MobileObjectInfo::LumpOfPoo, yaw, k_PooScale * self->size);
}

void CreaturePhysiologySystem::Puke(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* self = registry.TryGet<const Creature>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (self == nullptr || transform == nullptr || !Locator::resources::has_value())
	{
		return;
	}
	auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(k_PukeSpriteSheet.value()))
	{
		return;
	}
	const auto texture = textures.Handle(k_PukeSpriteSheet.value());
	const auto ahead = AheadOf(*transform);
	const glm::vec3 side {ahead.z, 0.0f, -ahead.x};
	const auto mouth =
	    transform->position + (ahead * (k_MouthAhead * self->size)) + glm::vec3(0.0f, k_MouthHeight * self->size, 0.0f);
	std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
	std::uniform_real_distribution<float> green(0.4f, 0.9f);
	for (int i = 0; i < k_PukeDrops; ++i)
	{
		const auto entity = registry.Create();
		const auto velocity = (ahead * (k_PukeSpeed * self->size)) + (side * (unit(_random) * k_PukeSpread * self->size)) +
		                      glm::vec3(0.0f, (1.0f + (0.5f * unit(_random))) * k_PukeSpeed * self->size * 0.5f, 0.0f);
		const auto g = green(_random);
		registry.Assign<CreaturePukeDrop>(entity, velocity, 0.0f);
		registry.Assign<Sprite>(entity, texture->GetNativeHandle(), glm::vec2(0.0f), glm::vec2(k_PukeSheetCell),
		                        glm::vec4(g * 0.6f, g, g * 0.2f, 1.0f), false);
		registry.Assign<Transform>(entity, mouth, glm::mat3(1.0f), glm::vec3(k_PukeDropScale * self->size));
	}
}

void CreaturePhysiologySystem::WakeFromFaint(entt::entity creature)
{
	if (auto* needs = Locator::entitiesRegistry::value().TryGet<CreatureNeeds>(creature))
	{
		physiology::WakeFromFaint(needs->needs);
		needs->faint.reset();
	}
}

void CreaturePhysiologySystem::FinishAction(entt::entity creature, std::string_view action)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* self = registry.TryGet<Creature>(creature);
	auto* needs = registry.TryGet<CreatureNeeds>(creature);
	if (self == nullptr || needs == nullptr || !Locator::infoConstants::has_value())
	{
		return;
	}
	const auto& actions = Locator::infoConstants::value().creatureAction;
	const auto found = std::ranges::find_if(actions, [action](const auto& row) {
		return std::string_view(row.name.data(), strnlen(row.name.data(), row.name.size())) == action;
	});
	if (found == actions.end())
	{
		return;
	}
	auto shape = ShapeOf(*self);
	physiology::ApplyActionCost(
	    needs->needs, shape,
	    {.strengthGain = found->strengthGain, .energyCost = found->energyCost, .exhaustionCost = found->exhaustionCost},
	    PhaseOf(registry, creature));
	if (TakeShape(*self, registry.TryGet<Transform>(creature), shape))
	{
		registry.SetDirty();
	}
}

void CreaturePhysiologySystem::ModifyStrength(entt::entity creature, float amount)
{
	if (auto* self = Locator::entitiesRegistry::value().TryGet<Creature>(creature))
	{
		auto shape = ShapeOf(*self);
		physiology::ModifyStrength(shape, amount);
		self->strength = shape.strength;
	}
}

void CreaturePhysiologySystem::SetTimeScale(float scale)
{
	_timeScale = std::clamp(scale, 0.0f, k_MaxTimeScale);
}

float CreaturePhysiologySystem::GetTimeScale() const
{
	return _timeScale;
}

void CreaturePhysiologySystem::SetFaintingEnabled(bool enabled)
{
	_fainting = enabled;
}

bool CreaturePhysiologySystem::IsFaintingEnabled() const
{
	return _fainting;
}
