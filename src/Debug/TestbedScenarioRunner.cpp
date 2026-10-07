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
#include <ctime>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ranges>
#include <system_error>
#include <type_traits>
#include <variant>

#include <MindFile.h>
#include <SDL_events.h>
#include <SDL_mouse.h>
#include <bgfx/bgfx.h>
#include <fmt/format.h>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/trigonometric.hpp>
#include <imgui.h>
#include <spdlog/spdlog.h>

#include "3D/DayNightClock.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "Common/FileDialog.h"
#include "Creature/CreatureDecisionTree.h"
#include "Creature/CreatureFight.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLearning.h"
#include "Creature/CreatureObjectActions.h"
#include "ECS/Archetypes/AbodeArchetype.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Archetypes/FeatureArchetype.h"
#include "ECS/Archetypes/MobileObjectArchetype.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Archetypes/TownArchetype.h"
#include "ECS/Archetypes/TreeArchetype.h"
#include "ECS/Archetypes/VillagerArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/Weather.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureCaveSystemInterface.h"
#include "ECS/Systems/CreatureFightSystemInterface.h"
#include "ECS/Systems/CreatureHandSystemInterface.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/CreatureModeSystemInterface.h"
#include "ECS/Systems/CreatureObjectActionSystemInterface.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "ECS/Systems/CreatureSkinSystemInterface.h"
#include "ECS/Systems/FootprintSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "FileSystem/FileSystemInterface.h"
#include "Game.h"
#include "InfoConstants.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Magic/SpellRules.h"
#include "TestbedDispenserGrid.h"
#include "Windowing/WindowingInterface.h"

#include "../Profiler.h"

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
/// The live results of a benchmark are summed up again every so many frames
constexpr uint32_t k_FramesPerSummary = 30;
/// What the store of a crowd's town starts with
constexpr uint32_t k_CrowdFood = 2000;
constexpr uint32_t k_CrowdWood = 2000;

/// The profiler's stages as benchmarks name them: the drawing of the reflection and of the main pass told apart, and
/// everything from drawing the scene on counted as rendering
std::vector<benchmark::StageInfo> BenchmarkStages()
{
	static const std::vector<std::string> k_Names = [] {
		std::vector<std::string> names;
		for (size_t i = 0; i < Profiler::k_StageNames.size(); ++i)
		{
			const auto stage = static_cast<Profiler::Stage>(i);
			std::string name(Profiler::k_StageNames.at(i));
			if (stage > Profiler::Stage::ReflectionPass && stage <= Profiler::Stage::ReflectionDrawParticles)
			{
				name = "Reflection " + name;
			}
			else if (stage > Profiler::Stage::MainPass && stage <= Profiler::Stage::MainPassDrawParticles)
			{
				name = "Main " + name;
			}
			names.push_back(std::move(name));
		}
		return names;
	}();
	std::vector<benchmark::StageInfo> stages;
	for (size_t i = 0; i < k_Names.size(); ++i)
	{
		stages.push_back({.name = k_Names.at(i), .render = static_cast<Profiler::Stage>(i) >= Profiler::Stage::SceneDraw});
	}
	return stages;
}

double Milliseconds(std::chrono::system_clock::duration duration)
{
	return std::chrono::duration<double, std::milli>(duration).count();
}

#ifdef NDEBUG
constexpr std::string_view k_Build = "optimised";
#else
constexpr std::string_view k_Build = "debug";
#endif

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

/// The debug windows leave the mouse alone while a scenario drives it, so that the real pointer resting on one of them
/// doesn't take the scenario's presses
void KeepDebugWindowsOffTheMouse(bool off)
{
	if (ImGui::GetCurrentContext() == nullptr)
	{
		return;
	}
	auto& io = ImGui::GetIO();
	io.ConfigFlags = off ? (io.ConfigFlags | ImGuiConfigFlags_NoMouse) : (io.ConfigFlags & ~ImGuiConfigFlags_NoMouse);
}
/// Whether a command sends its creature somewhere, or has it face, throw or point somewhere
bool HasPoint(Kind kind)
{
	return kind == Kind::WalkTo || kind == Kind::RunTo || kind == Kind::FleeFrom || kind == Kind::TurnToFace ||
	       kind == Kind::ThrowAt || kind == Kind::PointAt;
}

std::string_view Started(bool started)
{
	return started ? "started" : "can't";
}

/// The camera's fields of view across and up and down, in radians
glm::vec2 FieldsOfView(const Camera& camera)
{
	const auto horizontal = camera.GetHorizontalFieldOfView();
	const auto aspect = Locator::windowing::has_value() ? Locator::windowing::value().GetAspectRatio() : 1.0f;
	return {horizontal, 2.0f * std::atan(std::tan(horizontal * 0.5f) / std::max(aspect, 0.1f))};
}

/// Why the leash was refused, for the readout
std::string RefusedText(const ecs::systems::LeashSystemInterface& leashes)
{
	const auto refused = leashes.LastRefusal();
	return refused.has_value() ? fmt::format("refused: {}", creature_leash::Describe(refused->why)) : "can't";
}
} // namespace

void Runner::Start(const Scenario& scenario)
{
	Stop();
	// The camera is the player's again before the scenario frames it, and the cave closes
	if (Locator::creatureModeSystem::has_value())
	{
		Locator::creatureModeSystem::value().Leave();
	}
	if (Locator::creatureCaveSystem::has_value() && Locator::creatureCaveSystem::value().IsOpen())
	{
		Locator::creatureCaveSystem::value().Close();
	}
	_scenario = &scenario;
	_running = true;
	_seconds = 0.0f;
	_timeline = {};
	_creatures.clear();
	_objects.clear();
	_particles.clear();
	_miracles.clear();
	_started.clear();
	_log.clear();
	_shot.reset();
	_crowdCreatures.clear();
	_village = {};
	_crowdNext = 0;
	_crowdAbodes.clear();
	_crowdEntities.clear();
	_crowdProgress = {};
	_settledFrames = 0;
	_recorder.reset();
	_liveResults = {};
	_framesSinceSummary = 0;
	_saved = false;
	if (scenario.crowd.has_value())
	{
		const auto& crowd = *scenario.crowd;
		if (crowd.kind == Crowd::Kind::Creatures)
		{
			_crowdCreatures = LayOutCreatures(crowd.count, crowd.seed);
			_crowdProgress.total = _crowdCreatures.size();
		}
		else
		{
			_village = LayOutVillagers(crowd.count, crowd.seed);
			_crowdProgress.total = _village.towns.size() + _village.abodes.size() + _village.villagers.size();
		}
		_recorder = std::make_unique<benchmark::Recorder>(BenchmarkStages(), _benchmark.frames);
	}

	// A fresh testbed: the last one's creatures, objects, footprints, weather and scripts all go with it
	if (auto* game = Game::Instance(); game != nullptr)
	{
		game->LoadTestbed();
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
	PlaceDispensers(scenario);
	for (const auto& miracle : scenario.miracles)
	{
		_miracles.push_back({.nextAt = miracle.delaySeconds, .letGoAt = std::nullopt, .spell = entt::null});
	}
	for (size_t i = 0; i < scenario.particles.size(); ++i)
	{
		_particles.push_back({StartParticle(i), 0.0f});
	}
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
	// The mouse is the player's again
	_sweep.reset();
	if (Locator::gameActionSystem::has_value())
	{
		if (Locator::gameActionSystem::value().GetScriptedPointer().has_value())
		{
			Locator::gameActionSystem::value().SetScriptedPointer(std::nullopt);
			KeepDebugWindowsOffTheMouse(false);
		}
	}
	// Its particle effects die away
	if (Locator::particleSystem::has_value())
	{
		for (const auto& particle : _particles)
		{
			Locator::particleSystem::value().CloseDown(particle.effect);
		}
	}
	_particles.clear();
	// Its held miracles stop
	if (Locator::magicSystem::has_value())
	{
		for (const auto& miracle : _miracles)
		{
			if (miracle.letGoAt.has_value())
			{
				Locator::magicSystem::value().CloseDown(miracle.spell);
			}
		}
	}
	_miracles.clear();
	// The hand lets go of a creature a scenario held it to
	if (Locator::creatureHandSystem::has_value() && Locator::creatureHandSystem::value().IsHeldByCommand())
	{
		Locator::creatureHandSystem::value().Release();
	}
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
	if (Locator::creatureFightSystem::has_value())
	{
		Locator::creatureFightSystem::value().SetAngerStartsFights(true);
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
	if (Locator::creatureFightSystem::has_value())
	{
		Locator::creatureFightSystem::value().SetAngerStartsFights(environment.angerStartsFights);
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
		_objects.push_back(std::visit(
		    [&]<typename T>(T type) {
			    if constexpr (std::is_same_v<T, MobileObjectInfo>)
			    {
				    return ecs::archetypes::MobileObjectArchetype::Create(position, type, yaw, object.scale);
			    }
			    else if constexpr (std::is_same_v<T, TreeInfo>)
			    {
				    return ecs::archetypes::TreeArchetype::Create(0, position, type, true, yaw, object.scale, object.scale);
			    }
			    else if constexpr (std::is_same_v<T, VillagerInfo>)
			    {
				    constexpr uint32_t k_AdultAge = 30;
				    return ecs::archetypes::VillagerArchetype::Create(position, position, type, k_AdultAge);
			    }
			    else if constexpr (std::is_same_v<T, PotInfo>)
			    {
				    return ecs::archetypes::PotArchetype::Create(position, yaw, type, object.amount);
			    }
			    else
			    {
				    return ecs::archetypes::FeatureArchetype::Create(position, type, yaw, object.scale);
			    }
		    },
		    object.type));
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
		if (!setup.mindFile.empty())
		{
			LoadMindFile(entity, setup.mindFile);
		}
		if (Locator::leashSystem::has_value())
		{
			// Made for trying things out, it knows every leash, as creatures made by the original's debug tools do
			auto& leashes = Locator::leashSystem::value();
			for (const auto type : creature_leash::k_Types)
			{
				leashes.SetKnown(entity, type, true);
			}
			if (setup.leashable.has_value())
			{
				leashes.SetLeashable(entity, *setup.leashable);
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

std::optional<entt::entity> Runner::ObjectAt(size_t index) const
{
	if (index >= _objects.size() || !Locator::entitiesRegistry::has_value() ||
	    !Locator::entitiesRegistry::value().Valid(_objects[index]))
	{
		return std::nullopt;
	}
	return _objects[index];
}

std::string Runner::GiveObjectCommand(entt::entity creature, const Command& command)
{
	if (!Locator::creatureObjectActionSystem::has_value())
	{
		return "no hands";
	}
	auto& hands = Locator::creatureObjectActionSystem::value();
	const auto& land = Locator::terrainSystem::value();
	const auto point = MapPoint(_middle, command.point);
	const glm::vec3 onLand {point.x, land.GetHeightAt(point), point.y};
	const auto object = ObjectAt(command.object);
	switch (command.kind)
	{
	case Kind::PickUp:
		return object.has_value() ? std::string(Started(hands.PickUp(creature, *object))) : "it is gone";
	case Kind::PutDown:
		return std::string(Started(hands.PutDown(creature)));
	case Kind::Discard:
		return std::string(Started(hands.Discard(creature)));
	case Kind::Lob:
		return std::string(Started(hands.Lob(creature)));
	case Kind::EatHeld:
		return std::string(Started(hands.EatHeld(creature)));
	case Kind::Examine:
		return std::string(Started(hands.Keep(creature, creature_object_actions::k_FirstKeepAnimation + command.value)));
	case Kind::ThrowAt:
		// At about the height of a creature's middle
		return std::string(Started(hands.Throw(creature, onLand + glm::vec3(0.0f, CreatureHeight(1.0f) * 0.5f, 0.0f))));
	case Kind::KnockDown:
		return object.has_value() ? std::string(Started(hands.Destroy(creature, *object))) : "it is gone";
	case Kind::PointAt:
		return std::string(Started(hands.PointAt(creature, onLand)));
	default:
		return {};
	}
}

std::string Runner::GiveHandCommand(entt::entity creature, const Command& command)
{
	if (!Locator::creatureHandSystem::has_value())
	{
		return "no hand";
	}
	auto& hand = Locator::creatureHandSystem::value();
	switch (command.kind)
	{
	case Kind::HandStroke:
		return hand.Stroke(creature, static_cast<creature_feedback::BodyPart>(command.bodyPart)) ? "stroked" : "busy";
	case Kind::HandSlap:
		return hand.Slap(creature, command.slapHeight, command.gentle, command.sweepsRight) ? "slapped" : "busy";
	case Kind::HandLetGo:
		if (hand.GetCreature() == creature)
		{
			const auto sum = hand.GetFeedbackSum();
			hand.Release();
			return fmt::format("{:+.2f}", sum);
		}
		return "not held";
	default:
		return {};
	}
}

std::string Runner::GiveLeashCommand(entt::entity creature, const Command& command)
{
	if (!Locator::leashSystem::has_value())
	{
		return "no leashes";
	}
	auto& leashes = Locator::leashSystem::value();
	switch (command.kind)
	{
	case Kind::PutOnLeash:
		// It must know the learning leash before any, and the leash itself
		leashes.SetKnown(creature, LeashType::Rope, true);
		leashes.SetKnown(creature, command.leash, true);
		if (leashes.IsLeashed(creature))
		{
			return leashes.ChangeType(creature, command.leash) ? "changed" : "can't";
		}
		return leashes.PutOn(creature, command.leash) ? "on" : RefusedText(leashes);
	case Kind::MakeLeashable:
		return leashes.SetLeashable(creature, true) ? "leashable" : RefusedText(leashes);
	case Kind::HandTapLeash:
		// As the player's Action button tapping it does
		return leashes.TapCreature(command.player, creature) ? "on" : RefusedText(leashes);
	case Kind::LeashShake:
	{
		// A quick shake from side to side across the middle of the screen, sixty frames a second
		constexpr int k_Samples = 16;
		constexpr float k_Frame = 1.0f / 60.0f;
		constexpr float k_Swing = 0.12f;
		bool shaken = false;
		for (int i = 0; i < k_Samples && !shaken; ++i)
		{
			const auto side = (i / 2) % 2 == 0 ? -1.0f : 1.0f;
			shaken = leashes.TrackHand(command.player, {0.5f + (side * k_Swing), 0.5f}, k_Frame, true);
		}
		return shaken ? "off" : "no leash to shake off";
	}
	case Kind::LeashKey:
	{
		// As the player pressing the key does, through the same call the controls make
		constexpr std::array k_Keys {creature_leash::LeashKey::Leash, creature_leash::LeashKey::PreviousLeash,
		                             creature_leash::LeashKey::NextLeash};
		const auto before = leashes.TypeOf(creature);
		if (!leashes.PressKey(command.player, k_Keys.at(command.value)))
		{
			return RefusedText(leashes);
		}
		const auto after = leashes.TypeOf(creature);
		return after == LeashType::None ? "off" : before == after ? "same" : creature_leash::Name(after);
	}
	case Kind::TieLeash:
		if (const auto object = ObjectAt(command.object))
		{
			return leashes.TieTo(creature, *object) ? "tied" : "can't";
		}
		return "it is gone";
	case Kind::UntieLeash:
		leashes.UntieToHand(creature);
		return {};
	case Kind::TakeOffLeash:
		leashes.TakeOff(creature);
		return {};
	case Kind::ConfineToHome:
		leashes.ConfineToHome(creature, command.radius);
		return {};
	default:
		return {};
	}
}

std::string Runner::GiveFightCommand(entt::entity creature, const Command& command)
{
	if (!Locator::creatureFightSystem::has_value())
	{
		return "no fights";
	}
	auto& fights = Locator::creatureFightSystem::value();
	switch (command.kind)
	{
	case Kind::StartFight:
		if (const auto opponent = CreatureAt(command.value))
		{
			constexpr std::array<std::string_view, 4> k_Results {"started", "no opponent", "busy", "too weak"};
			return std::string(k_Results.at(static_cast<size_t>(fights.StartFight(creature, *opponent))));
		}
		return "it is gone";
	case Kind::FightBlow:
	{
		constexpr std::array k_Bands {creature_fight::Band::High, creature_fight::Band::Mid, creature_fight::Band::Low};
		// As a click held for the charge, then let go
		const bool queued = fights.QueueMove(creature, creature_fight::AttackMove(k_Bands.at(command.value)), true);
		fights.ReleaseCharge(creature, command.chargeMs);
		return queued ? fmt::format("{:.0f} ms", command.chargeMs) : "not fighting";
	}
	case Kind::FightBlock:
		return fights.QueueMove(creature, creature_fight::BlockMove(), true) ? "" : "not fighting";
	case Kind::FightStep:
		return fights.QueueMove(creature, creature_fight::StepMove(static_cast<creature_fight::Step>(command.value)), true)
		           ? ""
		           : "not fighting";
	case Kind::FightSpecial:
		return fights.QueueMove(creature, {.kind = creature_fight::Move::Kind::Special}, true) ? "" : "not fighting";
	case Kind::FightAuto:
		fights.SetAutoFighting(creature, command.value != 0);
		return fights.IsFighting(creature) ? "" : "not fighting";
	case Kind::KnockOut:
		fights.KnockOut(creature);
		return {};
	case Kind::BringRound:
		fights.Resurrect(creature);
		return {};
	case Kind::TieLeashToCreature:
		if (const auto other = CreatureAt(command.value); other.has_value() && Locator::leashSystem::has_value())
		{
			return Locator::leashSystem::value().TieTo(creature, *other) ? "tied" : "can't";
		}
		return "it is gone";
	default:
		return {};
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
	if (IsPointerCommand(command.kind))
	{
		const auto result = GivePointerCommand(command);
		const auto line = fmt::format("{:.2f}s: {}: {}", _seconds, Name(command.kind), result);
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Testbed {}", line);
		Log(line);
		return;
	}
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
	    needs != nullptr && needs->rest == CreatureNeeds::Rest::Unconscious && command.kind != Kind::BringRound)
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
	case Kind::ShowFeeling:
		if (const auto face = minds.ShowFeeling(*entity, static_cast<creature_face::Cue>(command.value)))
		{
			result = creature_face::Name(face->face);
		}
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
	case Kind::PickUp:
	case Kind::PutDown:
	case Kind::Discard:
	case Kind::Lob:
	case Kind::EatHeld:
	case Kind::Examine:
	case Kind::ThrowAt:
	case Kind::KnockDown:
	case Kind::PointAt:
		result = GiveObjectCommand(*entity, command);
		break;
	case Kind::HandStroke:
	case Kind::HandSlap:
	case Kind::HandLetGo:
		result = GiveHandCommand(*entity, command);
		break;
	case Kind::PutOnLeash:
	case Kind::TieLeash:
	case Kind::UntieLeash:
	case Kind::TakeOffLeash:
	case Kind::ConfineToHome:
	case Kind::MakeLeashable:
	case Kind::HandTapLeash:
	case Kind::LeashKey:
	case Kind::LeashShake:
		result = GiveLeashCommand(*entity, command);
		break;
	case Kind::StartFight:
	case Kind::FightBlow:
	case Kind::FightBlock:
	case Kind::FightStep:
	case Kind::FightSpecial:
	case Kind::FightAuto:
	case Kind::KnockOut:
	case Kind::BringRound:
	case Kind::TieLeashToCreature:
		result = GiveFightCommand(*entity, command);
		break;
	case Kind::CreatureKey:
	case Kind::DoubleClick:
	case Kind::CameraKeys:
	case Kind::ClearCameraView:
	case Kind::LeaveCreatureMode:
	case Kind::OpenCreatureCave:
	case Kind::ApplyTattoo:
	case Kind::RemoveTattoo:
		result = GiveCreatureModeCommand(*entity, command);
		break;
	case Kind::SetHour:
		break;
	case Kind::SetDesire:
	case Kind::SetPhase:
	case Kind::RewardIf:
		result = TeachMind(*entity, command);
		break;
	case Kind::SeeSkill:
		minds.SeeSkill(Locator::entitiesRegistry::value().Get<Transform>(*entity).position, command.value);
		break;
	case Kind::SeeMiracle:
		minds.SeeMiracle(Locator::entitiesRegistry::value().Get<Transform>(*entity).position, command.value);
		break;
	case Kind::PlayerDid:
	{
		const auto& land = Locator::terrainSystem::value();
		minds.PlayerDid(command.value, glm::vec3(point.x, land.GetHeightAt(point), point.y), std::nullopt);
		break;
	}
	}
	Log(fmt::format("{:.1f}s: {} {}{}{}", _seconds, who, Name(command.kind), result.empty() ? "" : ": ", result));
}

std::string Runner::GiveCreatureModeCommand(entt::entity creature, const Command& command)
{
	if (!Locator::creatureModeSystem::has_value() || !Locator::creatureCaveSystem::has_value())
	{
		return "no creature mode";
	}
	auto& mode = Locator::creatureModeSystem::value();
	auto& cave = Locator::creatureCaveSystem::value();
	const auto following = [&mode] {
		const auto followed = mode.GetCreature();
		return followed.has_value() ? fmt::format("following entity {}", static_cast<uint32_t>(*followed))
		                            : std::string("the camera is the player's");
	};
	switch (command.kind)
	{
	case Kind::CreatureKey:
		mode.PressCreatureKey();
		return following();
	case Kind::DoubleClick:
	{
		// Two presses of the left button on the creature, a fifth of a second apart
		const auto now = static_cast<uint32_t>(_seconds * 1000.0f) + 1;
		mode.Press({.milliseconds = now, .creature = creature});
		mode.Press({.milliseconds = now + 200, .creature = creature});
		return following();
	}
	case Kind::CameraKeys:
	{
		constexpr std::array<glm::ivec2, 4> k_Directions {{{-1, 0}, {1, 0}, {0, -1}, {0, 1}}};
		const auto direction = k_Directions.at(std::min<size_t>(command.value, k_Directions.size() - 1));
		mode.HoldKeys(direction.x, direction.y, !command.ctrl, command.ctrl, command.amount);
		return mode.IsActive() ? "" : "not in Creature Mode";
	}
	case Kind::ClearCameraView:
		mode.ClearView();
		if (const auto view = mode.GetView(); view.has_value())
		{
			return fmt::format("heading {:.2f}, pitch {:.2f}", view->yaw, view->pitch);
		}
		return "not in Creature Mode";
	case Kind::LeaveCreatureMode:
		mode.Leave();
		return following();
	case Kind::OpenCreatureCave:
		cave.Open();
		cave.GetScreen().page = static_cast<creature_cave::Page>(command.value);
		cave.GetScreen().pageRequested = true;
		return cave.GetCreature().has_value() ? "" : "the player has no creature";
	case Kind::ApplyTattoo:
		return cave.ApplyTattoo(static_cast<uint8_t>(command.bodyPart), static_cast<uint8_t>(command.value),
		                        glm::u8vec3(180, 30, 30))
		           ? ""
		           : "can't";
	case Kind::RemoveTattoo:
		return cave.RemoveTattoo(static_cast<uint8_t>(command.bodyPart)) ? "" : "nothing there";
	default:
		return {};
	}
}

std::string Runner::TeachMind(entt::entity entity, const Command& command)
{
	auto* mind = Locator::entitiesRegistry::value().TryGet<CreatureMindState>(entity);
	if (mind == nullptr)
	{
		return "no mind";
	}
	switch (command.kind)
	{
	case Kind::SetDesire:
		if (mind->desires.has_value() && command.value < creature_desires::k_DesireCount)
		{
			auto& state = mind->desires->desires.at(command.value);
			state.activated = true;
			state.suppressedTurns = 0;
			state.value = command.amount * std::max(state.max, 0.0f);
			return fmt::format("{} {:.2f}", creature_desires::Name(static_cast<creature_desires::Desire>(command.value)),
			                   state.value);
		}
		return "no desires yet";
	case Kind::SetPhase:
		mind->developmentPhase = static_cast<uint32_t>(command.value);
		return fmt::format("stage {}", command.value);
	case Kind::RewardIf:
		// From now on each thing it does to something is judged as soon as it is done
		mind->trainer = static_cast<uint32_t>(command.value);
		return fmt::format("trained: stroked for a {}, slapped for anything else",
		                   creature_tree::BeliefName(static_cast<uint32_t>(command.value)));
	default:
		return {};
	}
}

void Runner::LoadMindFile(entt::entity entity, std::string_view name)
{
	constexpr std::string_view k_Chosen = "chosen:";
	constexpr std::string_view k_Game = "game:";
	std::filesystem::path gameFolder;
	if (Locator::filesystem::has_value())
	{
		gameFolder = Locator::filesystem::value().GetPath<filesystem::Path::CreatureMind>(true);
	}
	std::filesystem::path path;
	if (name.starts_with(k_Chosen))
	{
		// The mind file last opened with the spawner's file dialog, else the game's own mind of that name
		std::error_code error;
		const auto chosen = file_dialog::RememberedPath("creature-mind-file");
		if (chosen.has_value() && std::filesystem::is_regular_file(*chosen, error))
		{
			path = *chosen;
		}
		else if (!gameFolder.empty())
		{
			path = gameFolder / name.substr(k_Chosen.size());
		}
	}
	else if (name.starts_with(k_Game) && !gameFolder.empty())
	{
		path = gameFolder / name.substr(k_Game.size());
	}
	auto data = std::make_shared<creaturemind::MindFileData>();
	const auto result = path.empty() ? creaturemind::MindResult::ErrCantOpen : creaturemind::ReadFile(path, *data);
	if (result == creaturemind::MindResult::Success && Locator::creatureMindSystem::has_value())
	{
		Locator::creatureMindSystem::value().LoadMind(entity, data);
	}
	Log(fmt::format("mind file {}: {}", path.empty() ? std::string(name) : path.filename().string(),
	                creaturemind::ResultToStr(result)));
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
	// Creature Mode takes the camera from the scenario's shot for good
	if (Locator::creatureModeSystem::has_value() && Locator::creatureModeSystem::value().IsActive())
	{
		_shot.reset();
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
		for (const auto& particle : _scenario->particles)
		{
			points.push_back(MapPoint(_middle, particle.offset));
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
	const auto gone = [](entt::entity entity) { return !Locator::entitiesRegistry::value().Valid(entity); };
	if ((!_creatures.empty() && std::ranges::none_of(std::views::iota(size_t {0}, _creatures.size()),
	                                                 [this](size_t i) { return CreatureAt(i).has_value(); })) ||
	    (!_crowdEntities.empty() && gone(_crowdEntities.front())))
	{
		_running = false;
		_shot.reset();
		Log("Its creatures are gone");
		return;
	}
	_seconds += seconds;
	Measure();
	SpawnCrowd();
	UpdateParticles(seconds);
	UpdateMiracles();
	ApplyStates();
	UpdatePointer(seconds);
	const auto due = Advance(_timeline, _scenario->commands, _scenario->repeatFrom, seconds,
	                         [this](size_t creature) { return IsFree(creature); });
	for (const auto index : due)
	{
		Give(_scenario->commands[index]);
	}
}

void Runner::PlaceDispensers(const Scenario& scenario)
{
	if (!KeepsDispenserGrid(scenario))
	{
		testbed_dispensers::RemoveGrid();
	}
	if (!Locator::magicSystem::has_value() || !Locator::terrainSystem::has_value())
	{
		return;
	}
	auto& magic = Locator::magicSystem::value();
	for (const auto& dispenser : scenario.dispensers)
	{
		const auto point = MapPoint(_middle, dispenser.offset);
		const float ground = Locator::terrainSystem::value().GetHeightAt(point);
		if (dispenser.bubbleHeight.has_value())
		{
			magic.CreateOneOffSeedFor({point.x, ground + *dispenser.bubbleHeight, point.y}, dispenser.type);
		}
		else
		{
			magic.CreateDispenser({point.x, ground, point.y}, dispenser.type, 0.0f);
		}
	}
}

void Runner::UpdateMiracles()
{
	if (!Locator::magicSystem::has_value())
	{
		return;
	}
	for (size_t i = 0; i < _miracles.size(); ++i)
	{
		auto& miracle = _miracles.at(i);
		const auto& setup = _scenario->miracles.at(i);
		if (miracle.letGoAt.has_value() && _seconds >= *miracle.letGoAt)
		{
			Locator::magicSystem::value().CloseDown(miracle.spell);
			miracle.letGoAt.reset();
		}
		if (!miracle.nextAt.has_value() || _seconds < *miracle.nextAt)
		{
			continue;
		}
		miracle.spell = CastMiracle(i);
		Log(fmt::format("Cast {}{}", static_cast<int>(setup.type), miracle.spell == entt::null ? ", which failed" : ""));
		if (setup.holdSeconds.has_value() && miracle.spell != entt::null)
		{
			miracle.letGoAt = _seconds + *setup.holdSeconds;
		}
		miracle.nextAt = setup.repeatSeconds.has_value() ? std::optional(*miracle.nextAt + *setup.repeatSeconds) : std::nullopt;
	}
}

entt::entity Runner::CastMiracle(size_t index)
{
	if (!Locator::terrainSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return entt::null;
	}
	const auto& setup = _scenario->miracles.at(index);
	const auto& info = Locator::infoConstants::value();
	const auto& land = Locator::terrainSystem::value();
	auto& magic = Locator::magicSystem::value();
	// What a seed of it in the hand would cast: a sizing miracle at its usual size
	const auto seed = magic::FindFirstSpellSeedForMagicType(info, setup.type).value_or(SpellSeedType::None);
	float size = 1.0f;
	if (const auto* radius = magic::GetMagicInfoAs<GMagicRadiusSpellInfo>(info, setup.type))
	{
		size = radius->radiusForNormalCost;
	}
	const auto cast = magic::SeedCastData(info, setup.type, seed, 1.0f, size);
	std::optional<entt::entity> target;
	auto point2 = MapPoint(_middle, setup.point);
	glm::vec3 point {point2.x, land.GetHeightAt(point2), point2.y};
	if (setup.target == MiracleCast::Target::Creature)
	{
		target = CreatureAt(setup.creature);
		if (!target.has_value())
		{
			return entt::null;
		}
		point = Locator::entitiesRegistry::value().Get<Transform>(*target).position;
	}
	const auto hand2 = MapPoint(_middle, setup.handOffset);
	const glm::vec3 hand {hand2.x, land.GetHeightAt(hand2) + setup.handHeight, hand2.y};
	const auto toward = point - hand;
	const particles::ProcessInfo process {
	    .handPosition = hand,
	    .cameraForward = glm::length(toward) > 0.0f ? glm::normalize(toward) : glm::vec3(0.0f, 0.0f, 1.0f),
	    .direction = setup.throwVelocity,
	};
	return target.has_value() ? magic.CastOnObject(setup.type, PlayerNames::PLAYER_ONE, *target, cast, process)
	                          : magic.CastAtPoint(setup.type, PlayerNames::PLAYER_ONE, point, cast, process);
}

uint32_t Runner::StartParticle(size_t index) const
{
	if (!Locator::particleSystem::has_value() || !Locator::terrainSystem::has_value())
	{
		return ecs::systems::ParticleSystemInterface::k_NoEffect;
	}
	const auto& particle = _scenario->particles.at(index);
	const auto point = MapPoint(_middle, particle.offset);
	const glm::vec3 position {point.x, Locator::terrainSystem::value().GetHeightAt(point) + particle.height, point.y};
	auto& particles = Locator::particleSystem::value();
	const auto effect = particle.file.empty() ? particles.Start(particle.type, position, particle.magnitude)
	                                          : particles.Start(particle.file, position, particle.magnitude);
	particles.SetPlayer(effect, particle.player);
	particles.SetDrawPath(effect, particle.path);
	if (particle.targetsCreatures)
	{
		for (const auto creature : _creatures)
		{
			particles.AddTarget(effect, creature);
		}
	}
	return effect;
}

void Runner::UpdateParticles(float seconds)
{
	if (!Locator::particleSystem::has_value())
	{
		return;
	}
	auto& particles = Locator::particleSystem::value();
	for (size_t i = 0; i < _particles.size(); ++i)
	{
		auto& running = _particles.at(i);
		const auto restart = _scenario->particles.at(i).restartSeconds;
		running.seconds += seconds;
		// An effect that has ended, or whose time is up, starts again
		if (!particles.IsRunning(running.effect) || (restart > 0.0f && running.seconds >= restart))
		{
			particles.CloseDown(running.effect);
			running = {StartParticle(i), 0.0f};
		}
	}
}

namespace
{
glm::vec2 WindowSize()
{
	return Locator::windowing::has_value() ? glm::vec2(Locator::windowing::value().GetSize()) : glm::vec2(1.0f);
}

/// Sends a move of the pointer to where it is, by so much, with the buttons held, as the mouse does
void PushMotion(const input::GameActionInterface::ScriptedPointer& pointer, glm::ivec2 moved)
{
	SDL_Event event {};
	event.type = SDL_MOUSEMOTION;
	event.motion.windowID = Locator::windowing::has_value() ? Locator::windowing::value().GetID() : 0;
	event.motion.state = pointer.buttons;
	event.motion.x = pointer.position.x;
	event.motion.y = pointer.position.y;
	event.motion.xrel = moved.x;
	event.motion.yrel = moved.y;
	SDL_PushEvent(&event);
}
} // namespace

std::string Runner::GivePointerCommand(const Command& command)
{
	if (!Locator::gameActionSystem::has_value())
	{
		return "no controls";
	}
	auto& actions = Locator::gameActionSystem::value();
	const auto size = WindowSize();
	auto pointer = actions.GetScriptedPointer().value_or(input::GameActionInterface::ScriptedPointer {
	    .position = glm::ivec2(actions.GetMousePosition()),
	});
	KeepDebugWindowsOffTheMouse(true);
	switch (command.kind)
	{
	case Kind::PointerTo:
	{
		const auto to = glm::ivec2(glm::round(command.point * size));
		const auto moved = to - pointer.position;
		pointer.position = to;
		actions.SetScriptedPointer(pointer);
		PushMotion(pointer, moved);
		break;
	}
	case Kind::PointerPress:
	case Kind::PointerRelease:
	{
		const bool press = command.kind == Kind::PointerPress;
		const auto button = static_cast<uint8_t>(command.value);
		pointer.buttons = press ? (pointer.buttons | SDL_BUTTON(button)) : (pointer.buttons & ~SDL_BUTTON(button));
		actions.SetScriptedPointer(pointer);
		SDL_Event event {};
		event.type = press ? SDL_MOUSEBUTTONDOWN : SDL_MOUSEBUTTONUP;
		event.button.windowID = Locator::windowing::has_value() ? Locator::windowing::value().GetID() : 0;
		event.button.button = button;
		event.button.state = press ? SDL_PRESSED : SDL_RELEASED;
		event.button.clicks = 1;
		event.button.x = pointer.position.x;
		event.button.y = pointer.position.y;
		SDL_PushEvent(&event);
		_handWatchSeconds = 0.5f;
		break;
	}
	case Kind::PointerSweep:
		actions.SetScriptedPointer(pointer);
		_sweep = PointerSweep {.pixelsPerSecond = command.point * size / command.amount, .secondsLeft = command.amount};
		break;
	case Kind::WheelTurn:
	{
		actions.SetScriptedPointer(pointer);
		SDL_Event event {};
		event.type = SDL_MOUSEWHEEL;
		event.wheel.windowID = Locator::windowing::has_value() ? Locator::windowing::value().GetID() : 0;
		event.wheel.y = static_cast<int32_t>(command.value) * (command.ctrl ? -1 : 1);
		event.wheel.preciseY = static_cast<float>(event.wheel.y);
		SDL_PushEvent(&event);
		break;
	}
	default:
		break;
	}
	return HandOnScreen();
}

void Runner::UpdatePointer(float seconds)
{
	if (!Locator::gameActionSystem::has_value())
	{
		return;
	}
	auto& actions = Locator::gameActionSystem::value();
	// Once the commands are done and the buttons let go, the mouse is the player's again
	if (const auto pointer = actions.GetScriptedPointer();
	    pointer.has_value() && pointer->buttons == 0 && !_sweep.has_value() && _timeline.done && _handWatchSeconds <= 0.0f)
	{
		actions.SetScriptedPointer(std::nullopt);
		KeepDebugWindowsOffTheMouse(false);
	}
	if (_handWatchSeconds > 0.0f || _sweep.has_value())
	{
		_handWatchSeconds -= seconds;
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Testbed {:.3f}s: {}", _seconds, HandOnScreen());
	}
	if (_sweep.has_value())
	{
		auto pointer = actions.GetScriptedPointer().value_or(input::GameActionInterface::ScriptedPointer {});
		const auto step = std::min(seconds, _sweep->secondsLeft);
		const auto exact = _sweep->pixelsPerSecond * step + _sweep->remainder;
		const auto moved = glm::ivec2(exact);
		_sweep->remainder = exact - glm::vec2(moved);
		// While the camera turns with the mouse its pointer is held, and only the movement comes through
		if (!actions.IsCursorFrozen())
		{
			pointer.position = glm::clamp(pointer.position + moved, glm::ivec2(0), glm::ivec2(WindowSize()) - 1);
		}
		actions.SetScriptedPointer(pointer);
		PushMotion(pointer, moved);
		_sweep->secondsLeft -= step;
		if (_sweep->secondsLeft <= 0.0f)
		{
			_sweep.reset();
			const auto line = fmt::format("{:.2f}s: moved: {}", _seconds, HandOnScreen());
			SPDLOG_LOGGER_INFO(spdlog::get("game"), "Testbed {}", line);
			Log(line);
		}
	}
}

std::string Runner::HandOnScreen() const
{
	if (!Locator::gameActionSystem::has_value() || !Locator::handSystem::has_value() || !Locator::camera::has_value())
	{
		return {};
	}
	const auto cursor = Locator::gameActionSystem::value().GetMousePosition();
	const auto hand = Locator::handSystem::value().GetPlayerHands()[0];
	const auto& transform = Locator::entitiesRegistry::value().Get<Transform>(hand);
	const auto& position = transform.position;
	const auto size = WindowSize();
	glm::vec3 screen {0.0f};
	Locator::camera::value().ProjectWorldToScreen(position, {0.0f, 0.0f, size.x, size.y}, screen);
	return fmt::format("cursor ({}, {}), hand ({:.0f}, {:.0f}) at ({:.1f}, {:.1f}, {:.1f}), {:.1f} from the camera, scale "
	                   "{:.3f}{}",
	                   cursor.x, cursor.y, screen.x, screen.y, position.x, position.y, position.z,
	                   glm::distance(position, Locator::camera::value().GetOrigin()), transform.scale.y,
	                   Locator::gameActionSystem::value().IsCursorFrozen() ? ", held" : "");
}

void Runner::Log(std::string line)
{
	_log.push_back(std::move(line));
	while (_log.size() > k_LogLines)
	{
		_log.pop_front();
	}
}

std::optional<CrowdProgress> Runner::GetCrowdProgress() const
{
	if (_scenario == nullptr || !_scenario->crowd.has_value())
	{
		return std::nullopt;
	}
	return _crowdProgress;
}

std::span<const benchmark::StageInfo> Runner::GetStages() const
{
	if (_recorder == nullptr)
	{
		return {};
	}
	return _recorder->Stages();
}

uint32_t Runner::GetWarmUpLeft() const
{
	return _settledFrames >= _benchmark.warmUpFrames ? 0 : _benchmark.warmUpFrames - _settledFrames;
}

void Runner::SpawnCrowd()
{
	if (_scenario == nullptr || !_scenario->crowd.has_value() || _crowdProgress.Done() ||
	    !Locator::terrainSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return;
	}
	const auto start = std::chrono::steady_clock::now();
	const auto batch = std::min(_scenario->crowd->perFrame, _crowdProgress.total - _crowdNext);
	for (size_t i = 0; i < batch; ++i)
	{
		SpawnCrowdMember(_crowdNext++);
	}
	_crowdProgress.spawned = _crowdNext;
	_crowdProgress.spawnMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	++_crowdProgress.spawnFrames;
	if (_crowdProgress.Done())
	{
		Log(fmt::format("Spawned {} in {:.0f} ms over {} frames", _crowdProgress.total, _crowdProgress.spawnMs,
		                _crowdProgress.spawnFrames));
	}
}

void Runner::SpawnCrowdMember(size_t index)
{
	const auto& land = Locator::terrainSystem::value();
	const auto onLand = [&land, this](glm::vec2 offset) {
		const auto point = MapPoint(_middle, offset);
		return glm::vec3(point.x, land.GetHeightAt(point), point.y);
	};
	if (!_crowdCreatures.empty())
	{
		const auto& member = _crowdCreatures.at(index);
		_crowdEntities.push_back(CreatureArchetype::Create(onLand(member.offset), member.owner, member.species, 0,
		                                                   glm::radians(member.facingDegrees), k_DefaultSize,
		                                                   CreatureArchetype::StartBody(member.species)));
		return;
	}
	// The towns first, then their homes and stores, then the villagers who live in them
	const auto towns = _village.towns.size();
	const auto abodes = _village.abodes.size();
	if (index < towns)
	{
		const auto& town = _village.towns.at(index);
		_crowdEntities.push_back(
		    ecs::archetypes::TownArchetype::Create(static_cast<int>(index), onLand(town.offset), town.owner, town.tribe));
		return;
	}
	if (index < towns + abodes)
	{
		const auto& abode = _village.abodes.at(index - towns);
		_crowdAbodes.push_back(ecs::archetypes::AbodeArchetype::Create(static_cast<uint32_t>(abode.town), onLand(abode.offset),
		                                                               abode.type, glm::radians(abode.yawDegrees), 1.0f,
		                                                               k_CrowdFood, k_CrowdWood));
		return;
	}
	const auto& member = _village.villagers.at(index - towns - abodes);
	const auto& home = _village.abodes.at(member.abode);
	const auto entity =
	    ecs::archetypes::VillagerArchetype::Create(onLand(home.offset), onLand(member.offset), member.type, member.age);
	_crowdEntities.push_back(entity);
	// It lives in the home laid out for it, rather than the first in its town with room
	auto& registry = Locator::entitiesRegistry::value();
	const auto abode = _crowdAbodes.at(member.abode);
	const auto& townIds = registry.Context().towns;
	const auto town = townIds.find(static_cast<uint32_t>(home.town));
	if (abode != entt::null && registry.Valid(abode) && town != townIds.end())
	{
		auto& villager = registry.Get<ecs::components::Villager>(entity);
		villager.abode = abode;
		villager.town = town->second;
		registry.Get<ecs::components::Abode>(abode).inhabitants.insert(entity);
	}
}

void Runner::Measure()
{
	if (_recorder == nullptr || !_crowdProgress.Done() || !Locator::profiler::has_value())
	{
		return;
	}
	if (_settledFrames < _benchmark.warmUpFrames)
	{
		++_settledFrames;
		return;
	}
	// The last frame, whole: it ended as this one started
	const auto& profiler = Locator::profiler::value();
	const auto& entry = profiler.GetEntries().at(profiler.GetEntryIndex(-1));
	const auto frame = entry.frameEnd - entry.frameStart;
	const auto& draw = entry.stages.at(static_cast<size_t>(Profiler::Stage::SceneDraw));
	const bool drawn = draw.finalized && draw.start >= entry.frameStart && draw.start <= entry.frameEnd;
	const auto update = drawn ? draw.start - entry.frameStart : frame;
	std::array<float, static_cast<size_t>(Profiler::Stage::_count)> stages {};
	for (size_t i = 0; i < stages.size(); ++i)
	{
		stages.at(i) = static_cast<float>(Milliseconds(entry.stages.at(i).total));
	}
	// What the renderer last drew, the frame before
	const auto* renderStats = bgfx::getStats();
	_recorder->Add(static_cast<float>(Milliseconds(frame)), static_cast<float>(Milliseconds(update)),
	               static_cast<float>(Milliseconds(frame - update)), stages,
	               renderStats != nullptr ? static_cast<float>(renderStats->numDraw) : 0.0f);
	if (++_framesSinceSummary >= k_FramesPerSummary || _recorder->Count() == _recorder->Capacity())
	{
		_framesSinceSummary = 0;
		_liveResults = _recorder->Summarise();
	}
	if (_benchmark.autoSave.has_value() && !_saved && _recorder->Count() >= _recorder->Capacity())
	{
		_saved = true;
		_liveResults = _recorder->Summarise();
		const auto written = SaveResults(_benchmark.autoSave);
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Benchmark {}: frame mean {:.2f} ms p95 {:.2f} max {:.2f}; results {}",
		                   _scenario->id, _liveResults.frame.average, _liveResults.frame.p95, _liveResults.frame.max,
		                   written.has_value() ? written->generic_string() : "not written");
		if (auto* game = Game::Instance(); game != nullptr)
		{
			game->RequestQuit();
		}
	}
}

std::vector<std::pair<std::string, size_t>> Runner::EntityCounts() const
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return {};
	}
	auto& registry = Locator::entitiesRegistry::value();
	return {
	    {"placed", registry.Size<Transform>()},
	    {"creatures", registry.Size<Creature>()},
	    {"villagers", registry.Size<ecs::components::Villager>()},
	    {"abodes", registry.Size<ecs::components::Abode>()},
	    {"towns", registry.Size<ecs::components::Town>()},
	};
}

std::optional<std::filesystem::path> Runner::SaveResults(std::optional<std::filesystem::path> base)
{
	if (_scenario == nullptr || _recorder == nullptr || _recorder->Count() == 0)
	{
		return std::nullopt;
	}
	if (!base.has_value())
	{
		const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm local {};
#ifdef _WIN32
		localtime_s(&local, &now);
#else
		localtime_r(&now, &local);
#endif
		base = std::filesystem::path("benchmarks") / fmt::format("{}_{}_{:04}{:02}{:02}_{:02}{:02}{:02}", _scenario->id,
		                                                         k_Build, local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
		                                                         local.tm_hour, local.tm_min, local.tm_sec);
	}
	benchmark::RunInfo run {
	    .scenarioId = std::string(_scenario->id),
	    .scenarioName = std::string(_scenario->name),
	    .build = std::string(k_Build),
	    .crowd = _crowdProgress.total,
	    .spawnMs = _crowdProgress.spawnMs,
	    .spawnFrames = _crowdProgress.spawnFrames,
	    .warmUpFrames = _benchmark.warmUpFrames,
	    .entityCounts = EntityCounts(),
	};
	if (Locator::windowing::has_value())
	{
		const auto size = Locator::windowing::value().GetSize();
		run.width = static_cast<uint32_t>(size.x);
		run.height = static_cast<uint32_t>(size.y);
	}
	std::error_code error;
	if (base->has_parent_path())
	{
		std::filesystem::create_directories(base->parent_path(), error);
	}
	auto jsonPath = *base;
	jsonPath += ".json";
	auto csvPath = *base;
	csvPath += ".csv";
	std::ofstream json(jsonPath, std::ios::binary);
	std::ofstream csv(csvPath, std::ios::binary);
	if (!json || !csv)
	{
		Log(fmt::format("Couldn't write {}", jsonPath.generic_string()));
		return std::nullopt;
	}
	const auto results = _recorder->Summarise();
	json << benchmark::ToJson(run, results, _recorder->Stages());
	csv << benchmark::ToCsv(run, results, _recorder->Stages());
	Log(fmt::format("Saved {}", jsonPath.generic_string()));
	return jsonPath;
}
