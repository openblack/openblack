/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TestbedScenarioRunner.h"

#include <cmath>

#include <algorithm>
#include <ranges>
#include <type_traits>
#include <variant>

#include <fmt/format.h>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/trigonometric.hpp>

#include "3D/DayNightClock.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "Creature/CreatureLayers.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Archetypes/FeatureArchetype.h"
#include "ECS/Archetypes/MobileObjectArchetype.h"
#include "ECS/Archetypes/TreeArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Weather.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "ECS/Systems/CreatureSkinSystemInterface.h"
#include "ECS/Systems/FootprintSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "Game.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using namespace openblack::testbed_scenarios;
using openblack::ecs::archetypes::CreatureArchetype;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureAnimation;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::CreatureNeeds;
using openblack::ecs::components::Transform;

namespace
{
using Kind = Command::Kind;
using Pace = ecs::systems::CreatureLocomotionSystemInterface::Pace;

/// The lines of the log of commands kept
constexpr size_t k_LogLines = 8;
/// A creature follows another this far behind, for a creature of size 1
constexpr float k_FollowDistance = 25.0f;
/// Forced weather lasts as long as anyone watches
constexpr float k_WeatherSeconds = 3600.0f;
/// The size of a creature whose scenario doesn't give one
constexpr float k_DefaultSize = 1.0f;
/// The least the overview takes in either way of the middle of what it frames
constexpr float k_MinOverviewHalfSize = 25.0f;

/// The weather of each kind laid over the island, as the weather window's presets have it
ecs::components::WeatherInfo WeatherOf(Weather weather)
{
	switch (weather)
	{
	case Weather::Rain:
		return {.temperature = 12, .rain = 80, .overcast = 90, .windZ = 15};
	case Weather::Thunderstorm:
		return {.temperature = 20, .rain = 100, .overcast = 100, .windZ = 30};
	case Weather::Snow:
		return {.temperature = -5, .snow = 70, .overcast = 80, .windZ = 5};
	case Weather::Blizzard:
		return {.temperature = -15, .snow = 100, .overcast = 100, .windZ = 60};
	case Weather::Clear:
	default:
		// The testbed has no climate of its own, which would leave it at freezing; a clear day is mild
		return {.temperature = 18};
	}
}

std::string_view MoveResultName(ecs::systems::CreatureLocomotionSystemInterface::MoveResult result)
{
	using MoveResult = ecs::systems::CreatureLocomotionSystemInterface::MoveResult;
	switch (result)
	{
	case MoveResult::InvalidDestination:
		return "nowhere to stand there";
	case MoveResult::Busy:
		return "it can't walk";
	case MoveResult::Started:
	default:
		return "on its way";
	}
}

/// Where a creature faces on the land; its mesh looks back along +z
glm::vec2 AheadOf(const Transform& transform)
{
	const auto ahead = -(transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f));
	return {ahead.x, ahead.z};
}

/// Whether a command sends its creature somewhere, or has it face somewhere
bool HasPoint(Kind kind)
{
	return kind == Kind::WalkTo || kind == Kind::RunTo || kind == Kind::FleeFrom || kind == Kind::TurnToFace;
}

/// The camera's fields of view across and up and down, in radians
glm::vec2 FieldsOfView(const Camera& camera)
{
	const auto horizontal = camera.GetHorizontalFieldOfView();
	const auto aspect = Locator::windowing::has_value() ? Locator::windowing::value().GetAspectRatio() : 1.0f;
	return {horizontal, 2.0f * std::atan(std::tan(horizontal * 0.5f) / std::max(aspect, 0.1f))};
}
} // namespace

void Runner::Start(const Scenario& scenario)
{
	Stop();
	_scenario = &scenario;
	_running = true;
	_seconds = 0.0f;
	_timeline = {};
	_creatures.clear();
	_started.clear();
	_log.clear();
	_shot.reset();

	// A fresh testbed: the last one's creatures, objects, footprints, weather and scripts all go with it
	if (auto* game = Game::Instance(); game != nullptr)
	{
		game->LoadTestbed(scenario.environment.land == Land::Pool);
	}
	if (!Locator::terrainSystem::has_value())
	{
		_running = false;
		return;
	}
	const auto& land = Locator::terrainSystem::value();
	_middle = (land.GetExtent().minimum + land.GetExtent().maximum) * 0.5f;

	SetUpEnvironment(scenario.environment);
	PlaceObjects(scenario, _middle);
	PlaceCreatures(scenario, _middle);
	Frame(scenario.framing.shot, scenario.framing.creature, scenario.framing.distance);
	Log(fmt::format("Started {}", scenario.name));
}

void Runner::Stop()
{
	if (!_running)
	{
		return;
	}
	_running = false;
	if (Locator::creaturePhysiologySystem::has_value())
	{
		auto& physiology = Locator::creaturePhysiologySystem::value();
		physiology.SetTimeScale(1.0f);
		physiology.SetFaintingEnabled(true);
	}
	if (Locator::footprintSystem::has_value())
	{
		Locator::footprintSystem::value().SetAprilFoolsOverride(std::nullopt);
	}
	if (Locator::skySystem::has_value())
	{
		Locator::skySystem::value().GetClock().SetRunning(true);
	}
	Log("Stopped");
}

void Runner::SetUpEnvironment(const Environment& environment)
{
	if (Locator::skySystem::has_value())
	{
		auto& sky = Locator::skySystem::value();
		sky.SetTime(environment.hour);
		sky.GetClock().SetRunning(environment.clockRuns);
	}
	if (Locator::weatherSystem::has_value())
	{
		// Only the scenario's weather: the climates breed no storms of their own
		auto& weather = Locator::weatherSystem::value();
		weather.ClearStorms();
		weather.SetStormCreationEnabled(false);
		const auto thunder = environment.weather == Weather::Thunderstorm;
		weather.ForceStorm({.effect = WeatherOf(environment.weather),
		                    .seconds = k_WeatherSeconds,
		                    .fadeSeconds = 0.5f,
		                    .thunderWait = thunder ? glm::vec2(3.0f, 15.0f) : glm::vec2(0.0f),
		                    .boltWait = thunder ? glm::vec2(2.0f, 8.0f) : glm::vec2(0.0f)});
	}
	if (Locator::creaturePhysiologySystem::has_value())
	{
		auto& physiology = Locator::creaturePhysiologySystem::value();
		physiology.SetTimeScale(environment.bodyTimeScale);
		physiology.SetFaintingEnabled(environment.fainting);
	}
	if (Locator::footprintSystem::has_value())
	{
		Locator::footprintSystem::value().SetAprilFoolsOverride(environment.aprilFools);
	}
}

void Runner::PlaceObjects(const Scenario& scenario, glm::vec2 middle)
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	const auto& land = Locator::terrainSystem::value();
	for (const auto& object : scenario.objects)
	{
		const auto point = MapPoint(middle, object.offset);
		const glm::vec3 position {point.x, land.GetHeightAt(point), point.y};
		const auto yaw = glm::radians(object.yawDegrees);
		std::visit(
		    [&]<typename T>(T type) {
			    if constexpr (std::is_same_v<T, MobileObjectInfo>)
			    {
				    ecs::archetypes::MobileObjectArchetype::Create(position, type, yaw, object.scale);
			    }
			    else if constexpr (std::is_same_v<T, TreeInfo>)
			    {
				    ecs::archetypes::TreeArchetype::Create(0, position, type, true, yaw, object.scale, object.scale);
			    }
			    else
			    {
				    ecs::archetypes::FeatureArchetype::Create(position, type, yaw, object.scale);
			    }
		    },
		    object.type);
	}
}

void Runner::PlaceCreatures(const Scenario& scenario, glm::vec2 middle)
{
	const auto& land = Locator::terrainSystem::value();
	auto& registry = Locator::entitiesRegistry::value();
	for (const auto& setup : scenario.creatures)
	{
		const auto point = MapPoint(middle, setup.offset);
		const glm::vec3 position {point.x, land.GetHeightAt(point), point.y};
		auto body = CreatureArchetype::StartBody(setup.species);
		body.alignment = setup.alignment.value_or(body.alignment);
		body.fatness = setup.fatness.value_or(body.fatness);
		body.strength = setup.strength.value_or(body.strength);
		const auto size = setup.size.value_or(k_DefaultSize);
		const auto entity =
		    CreatureArchetype::Create(position, setup.owner, setup.species, 0, glm::radians(setup.facingDegrees), size, body);
		if (auto* mind = registry.TryGet<CreatureMindState>(entity))
		{
			mind->paused = setup.pauseMind;
			if (setup.phase.has_value())
			{
				mind->developmentPhase = *setup.phase;
			}
		}
		if (Locator::creatureSkinSystem::has_value())
		{
			auto& skins = Locator::creatureSkinSystem::value();
			for (size_t slot = 0; slot < setup.tattoos.size(); ++slot)
			{
				skins.SetTattoo(entity, slot, setup.tattoos[slot]);
			}
			for (const auto& wound : setup.wounds)
			{
				skins.AddWound(entity, wound);
			}
			for (const auto& drop : setup.blood)
			{
				skins.AddBlood(entity, drop);
			}
		}
		_creatures.push_back(entity);
		_started.push_back(false);
	}
}

std::optional<entt::entity> Runner::CreatureAt(size_t index) const
{
	if (index >= _creatures.size() || !Locator::entitiesRegistry::has_value())
	{
		return std::nullopt;
	}
	const auto entity = _creatures[index];
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(entity) || !registry.AllOf<Creature>(entity))
	{
		return std::nullopt;
	}
	return entity;
}

void Runner::ApplyStates()
{
	auto& registry = Locator::entitiesRegistry::value();
	for (size_t i = 0; i < _creatures.size(); ++i)
	{
		const auto entity = CreatureAt(i);
		if (!entity.has_value())
		{
			continue;
		}
		const auto& setup = _scenario->creatures[i];
		auto* needs = registry.TryGet<CreatureNeeds>(*entity);
		auto* mind = registry.TryGet<CreatureMindState>(*entity);
		const auto* creature = registry.TryGet<const Creature>(*entity);
		// The body and the mind start from the species' tables on their first turn, over anything set before
		if (needs == nullptr || mind == nullptr || creature == nullptr || !needs->started || !mind->desires.has_value())
		{
			continue;
		}
		if (_started[i] && !setup.hold)
		{
			continue;
		}
		auto overrides = setup.needs;
		if (_started[i])
		{
			// Its age is only set as it starts, so it still grows older
			overrides.age.reset();
		}
		Apply(overrides, needs->needs, creature->size);
		Apply(setup.desires, *mind->desires);
		if (!_started[i] && !setup.desires.empty())
		{
			// Its first thought was had with the species' desires; it thinks again with the scenario's
			mind->idle = {};
		}
		_started[i] = true;
	}
}

bool Runner::IsFree(size_t creature) const
{
	const auto entity = CreatureAt(creature);
	if (!entity.has_value())
	{
		// A creature that is gone holds nothing up
		return true;
	}
	if (Locator::creatureLocomotionSystem::has_value() && Locator::creatureLocomotionSystem::value().IsMoving(*entity))
	{
		return false;
	}
	const auto* animation = Locator::entitiesRegistry::value().TryGet<const CreatureAnimation>(*entity);
	return animation == nullptr || !creature_layers::IsPlaying(animation->body);
}

void Runner::Give(const Command& command)
{
	if (command.kind == Kind::SetHour)
	{
		if (Locator::skySystem::has_value())
		{
			Locator::skySystem::value().SetTime(command.hour);
		}
		Log(fmt::format("{:.1f}s: the hour is {:.1f}", _seconds, command.hour));
		return;
	}
	const auto entity = CreatureAt(command.creature);
	if (!entity.has_value() || !Locator::creatureLocomotionSystem::has_value() || !Locator::creatureMindSystem::has_value())
	{
		return;
	}
	const auto& setup = _scenario->creatures[command.creature];
	const auto who = setup.label.empty() ? fmt::format("creature {}", command.creature) : std::string(setup.label);
	// Lying out cold, it does nothing it is told until it comes round
	if (const auto* needs = Locator::entitiesRegistry::value().TryGet<const CreatureNeeds>(*entity);
	    needs != nullptr && needs->rest == CreatureNeeds::Rest::Unconscious)
	{
		Log(fmt::format("{:.1f}s: {} {}: out cold", _seconds, who, Name(command.kind)));
		return;
	}
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	auto& minds = Locator::creatureMindSystem::value();
	const auto point = MapPoint(_middle, command.point);
	std::string result;
	switch (command.kind)
	{
	case Kind::WalkTo:
	case Kind::RunTo:
		result =
		    MoveResultName(locomotion.MoveTo(*entity, point, command.kind == Kind::RunTo ? Pace::Run : Pace::Walk, 0.0f, 1.0f));
		break;
	case Kind::Follow:
		if (const auto leader = CreatureAt(command.value))
		{
			const auto size = Locator::entitiesRegistry::value().Get<Creature>(*entity).size;
			result = MoveResultName(locomotion.Follow(*entity, *leader, k_FollowDistance * std::max(size, 0.5f), Pace::Walk));
		}
		break;
	case Kind::FleeFrom:
		result = MoveResultName(locomotion.FleeFrom(*entity, point));
		break;
	case Kind::TurnToFace:
		result = locomotion.TurnToFace(*entity, point) ? "turning" : "can't";
		break;
	case Kind::FaceCamera:
		result = locomotion.TurnToFace(*entity, glm::xz(Locator::camera::value().GetOrigin())) ? "turning" : "can't";
		break;
	case Kind::Stop:
		locomotion.Stop(*entity);
		break;
	case Kind::PlayAction:
		result = minds.PlayAction(*entity, command.value) ? "playing" : "busy";
		break;
	case Kind::PlayGesture:
		result = minds.PlayGesture(*entity, command.value) ? "playing" : "busy";
		break;
	case Kind::PullFace:
		minds.PullFace(*entity, command.value);
		break;
	case Kind::SitDown:
		result = minds.SitDown(*entity) ? "sitting" : "busy";
		break;
	case Kind::StandUp:
		minds.StandUp(*entity);
		break;
	case Kind::Sleep:
		result = minds.Sleep(*entity) ? "started" : "can't";
		break;
	case Kind::Wake:
		minds.Wake(*entity);
		break;
	case Kind::Eat:
		result = minds.Eat(*entity, std::nullopt) ? "started" : "nothing to eat";
		break;
	case Kind::Drink:
		result = minds.Drink(*entity) ? "started" : "no water near";
		break;
	case Kind::Poo:
		result = minds.Poo(*entity) ? "started" : "can't";
		break;
	case Kind::Puke:
		result = minds.Puke(*entity) ? "started" : "can't";
		break;
	case Kind::Faint:
		result = minds.Faint(*entity) ? "started" : "can't";
		break;
	case Kind::Stroke:
	case Kind::Slap:
		// As a whole session of the hand on the creature would, a full reward or punishment as the hand lets go
		minds.ReceiveFeedback(*entity, command.kind == Kind::Stroke ? 1.0f : -1.0f);
		break;
	case Kind::SetHour:
		break;
	}
	Log(fmt::format("{:.1f}s: {} {}{}{}", _seconds, who, Name(command.kind), result.empty() ? "" : ": ", result));
}

void Runner::Frame(Shot shot, size_t creature, float distance)
{
	_shot = shot;
	_shotCreature = creature;
	_shotDistance = distance;
	UpdateCamera();
}

void Runner::UpdateCamera()
{
	if (!_shot.has_value() || _scenario == nullptr || !Locator::terrainSystem::has_value())
	{
		return;
	}
	auto& camera = Locator::camera::value();
	auto& registry = Locator::entitiesRegistry::value();
	const auto& land = Locator::terrainSystem::value();
	std::optional<CameraPlacement> placement;
	switch (*_shot)
	{
	case Shot::Testbed:
		// As the testbed leaves it: 120 units south of the middle and 60 up, looking at the middle
		placement = CameraPlacement {
		    .origin = {_middle.x, land.GetHeightAt(_middle) + 60.0f, _middle.y - 120.0f},
		    .focus = {_middle.x, land.GetHeightAt(_middle), _middle.y},
		};
		break;
	case Shot::Overview:
	{
		std::vector<glm::vec2> points;
		for (size_t i = 0; i < _creatures.size(); ++i)
		{
			if (const auto entity = CreatureAt(i))
			{
				points.push_back(glm::xz(registry.Get<Transform>(*entity).position));
			}
		}
		for (const auto& object : _scenario->objects)
		{
			points.push_back(MapPoint(_middle, object.offset));
		}
		for (const auto& extra : _scenario->framing.include)
		{
			points.push_back(MapPoint(_middle, extra));
		}
		// And everywhere the creatures are sent
		for (const auto& command : _scenario->commands)
		{
			if (HasPoint(command.kind))
			{
				points.push_back(MapPoint(_middle, command.point));
			}
		}
		const auto bounds = BoundsOf(points, glm::vec2(k_MinOverviewHalfSize));
		placement = Overview({bounds.centre.x, land.GetHeightAt(bounds.centre), bounds.centre.y}, bounds.halfSize,
		                     FieldsOfView(camera), _shotDistance);
		break;
	}
	case Shot::Follow:
	case Shot::Head:
		if (const auto entity = CreatureAt(_shotCreature))
		{
			const auto& transform = registry.Get<Transform>(*entity);
			const auto height = CreatureHeight(registry.Get<Creature>(*entity).size);
			placement = *_shot == Shot::Follow ? Follow(transform.position, height, _shotDistance)
			                                   : Head(transform.position, AheadOf(transform), height, _shotDistance);
		}
		break;
	}
	if (placement.has_value())
	{
		camera.SetOrigin(placement->origin).SetFocus(placement->focus);
	}
	// A shot of everything is taken once; following and close ups keep up with the creature
	if (*_shot == Shot::Overview || *_shot == Shot::Testbed)
	{
		_shot.reset();
	}
}

void Runner::Update(float seconds)
{
	if (_scenario == nullptr)
	{
		return;
	}
	UpdateCamera();
	if (!_running)
	{
		return;
	}
	// Another land was loaded over the testbed
	if (!_creatures.empty() && std::ranges::none_of(std::views::iota(size_t {0}, _creatures.size()),
	                                                [this](size_t i) { return CreatureAt(i).has_value(); }))
	{
		_running = false;
		_shot.reset();
		Log("Its creatures are gone");
		return;
	}
	_seconds += seconds;
	ApplyStates();
	const auto due = Advance(_timeline, _scenario->commands, _scenario->repeatFrom, seconds,
	                         [this](size_t creature) { return IsFree(creature); });
	for (const auto index : due)
	{
		Give(_scenario->commands[index]);
	}
}

void Runner::Log(std::string line)
{
	_log.push_back(std::move(line));
	while (_log.size() > k_LogLines)
	{
		_log.pop_front();
	}
}
