/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TestbedScenarios.h"

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "Creature/CreatureIdleMind.h"
#include "CreatureSpawner.h"
#include "DebugGuiInterface.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "Game.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;
using namespace openblack::testbed_scenarios;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureLocomotion;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::CreatureNeeds;

namespace
{
// Indexed by species, from the cow
constexpr std::array<std::string_view, static_cast<size_t>(CreatureType::_COUNT) - 1> k_SpeciesNames {
    "Cow",        "Tiger", "Leopard", "Wolf", "Lion",     "Horse", "Tortoise", "Zebra",     "Brown Bear",
    "Polar Bear", "Sheep", "Chimp",   "Ogre", "Mandrill", "Rhino", "Gorilla",  "Giant Ape",
};

/// The game's speeds to pick from: how many times longer a turn takes, and what to call it
struct Speed
{
	float multiplier;
	std::string_view name;
};
constexpr std::array<Speed, 4> k_Speeds {{{2.0f, "x0.5"}, {1.0f, "x1"}, {0.5f, "x2"}, {0.25f, "x4"}}};
/// The speeds the creatures' body time can run at
constexpr std::array<float, 5> k_BodyTimes {1.0f, 10.0f, 100.0f, 1000.0f, 3600.0f};

const ImVec4 k_RunColour {0.25f, 0.60f, 0.30f, 1.0f};
const ImVec4 k_StopColour {0.85f, 0.30f, 0.25f, 1.0f};
/// The costliest profiler stages a benchmark shows as it runs
constexpr size_t k_TopStages = 12;
/// A frame this long or less makes the target of 100 frames a second
constexpr float k_TargetFrameMs = 10.0f;

std::string_view SpeciesName(CreatureType species)
{
	const auto index = static_cast<size_t>(species);
	return index >= 1 && index <= k_SpeciesNames.size() ? k_SpeciesNames.at(index - 1) : "Unknown";
}

std::string_view MotionName(CreatureLocomotion::Motion motion)
{
	constexpr std::array<std::string_view, 6> k_Names {"standing", "planning",     "confused",
	                                                   "turning",  "stepping off", "walking"};
	return k_Names.at(static_cast<size_t>(motion));
}

/// The scenarios the list shows, of the facet or all of them
std::vector<size_t> Listed(std::optional<Facet> facet)
{
	std::vector<size_t> listed;
	const auto all = All();
	for (size_t i = 0; i < all.size(); ++i)
	{
		if (!facet.has_value() || all[i].facet == *facet)
		{
			listed.push_back(i);
		}
	}
	return listed;
}

std::string Label(const Scenario& scenario)
{
	return fmt::format("{}: {}", Name(scenario.facet), scenario.name);
}

/// The strongest of a creature's activated desires
std::optional<creature_desires::Desire> Strongest(const creature_desires::Desires& desires)
{
	std::optional<creature_desires::Desire> strongest;
	float value = 0.0f;
	for (size_t i = 0; i < creature_desires::k_DesireCount; ++i)
	{
		const auto& state = desires.desires.at(i);
		if (state.activated && state.value > value)
		{
			value = state.value;
			strongest = static_cast<creature_desires::Desire>(i);
		}
	}
	return strongest;
}
} // namespace

TestbedScenarios::TestbedScenarios(CreatureSpawner& spawner) noexcept
    : Window(std::string(k_TestbedScenariosWindow), ImVec2(640.0f, 620.0f))
    , _spawner(spawner)
{
}

void TestbedScenarios::UpdateAlways() noexcept
{
	// Scenarios run in the game's time: not while it is paused, and faster as the game is
	auto* game = Game::Instance();
	float seconds = ImGui::GetIO().DeltaTime;
	if (game != nullptr)
	{
		seconds = game->IsPaused() ? 0.0f : seconds / std::max(game->GetGameSpeed(), 0.01f);
	}
	RunRequested();
	_runner.Update(seconds);
}

void TestbedScenarios::RunRequested() noexcept
{
	auto* game = Game::Instance();
	if (game == nullptr)
	{
		return;
	}
	const auto request = game->TakeScenarioRequest();
	if (!request.has_value())
	{
		return;
	}
	const auto* scenario = Find(request->id);
	if (scenario == nullptr)
	{
		std::string ids;
		for (const auto& each : All())
		{
			ids += fmt::format(" {}", each.id);
		}
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "No testbed scenario {}; there are:{}", request->id, ids);
		game->RequestQuit();
		return;
	}
	const auto all = All();
	_picked = static_cast<size_t>(scenario - all.data());
	_facet = scenario->facet;
	_runner.SetBenchmarkSettings({
	    .warmUpFrames = request->warmUpFrames,
	    .frames = request->frames,
	    // Without a crowd there is nothing to measure, and the game carries on with the scenario
	    .autoSave = scenario->crowd.has_value()
	                    ? std::optional(request->results.value_or(std::filesystem::path("benchmarks") / scenario->id))
	                    : std::nullopt,
	});
	_focus = 0;
	_runner.Start(*scenario);
}

void TestbedScenarios::Draw() noexcept
{
	DrawPicker();
	DrawControls();
	DrawBenchmark();
	DrawTime();
	DrawCamera();
	DrawCreatures();
}

void TestbedScenarios::DrawPicker() noexcept
{
	const auto all = All();
	if (ImGui::BeginCombo("Facet", _facet.has_value() ? Name(*_facet).data() : "Every facet"))
	{
		if (ImGui::Selectable("Every facet", !_facet.has_value()))
		{
			_facet.reset();
		}
		for (size_t i = 0; i < k_FacetCount; ++i)
		{
			const auto facet = static_cast<Facet>(i);
			if (ImGui::Selectable(Name(facet).data(), _facet == facet))
			{
				_facet = facet;
				const auto listed = Listed(_facet);
				if (!listed.empty() && std::ranges::find(listed, _picked) == listed.end())
				{
					_picked = listed.front();
				}
			}
		}
		ImGui::EndCombo();
	}
	_picked = std::min(_picked, all.size() - 1);
	if (ImGui::BeginCombo("Scenario", Label(all[_picked]).c_str(), ImGuiComboFlags_HeightLarge))
	{
		for (const auto index : Listed(_facet))
		{
			if (ImGui::Selectable(Label(all[index]).c_str(), index == _picked))
			{
				_picked = index;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", all[index].description.data());
			}
		}
		ImGui::EndCombo();
	}
	const auto& scenario = all[_picked];
	ImGui::TextWrapped("%s", scenario.description.data());
	ImGui::TextWrapped("Look for: %s", scenario.expected.data());
	const auto& environment = scenario.environment;
	ImGui::TextDisabled("%.1f o'clock%s, %s, body time x%.0f, %zu creature%s, %zu object%s, %zu command%s",
	                    static_cast<double>(environment.hour), environment.clockRuns ? "" : " (clock stopped)",
	                    Name(environment.weather).data(), static_cast<double>(environment.bodyTimeScale),
	                    scenario.creatures.size(), scenario.creatures.size() == 1 ? "" : "s", scenario.objects.size(),
	                    scenario.objects.size() == 1 ? "" : "s", scenario.commands.size(),
	                    scenario.commands.size() == 1 ? "" : "s");
}

void TestbedScenarios::DrawControls() noexcept
{
	const auto& picked = All()[_picked];
	const auto* current = _runner.GetScenario();
	const bool running = _runner.IsRunning();

	ImGui::PushStyleColor(ImGuiCol_Button, k_RunColour);
	if (ImGui::Button("Run", ImVec2(90.0f, 0.0f)))
	{
		_focus = 0;
		_runner.Start(picked);
	}
	ImGui::PopStyleColor();
	ImGui::SetItemTooltip("Loads the testbed afresh and sets this scenario up on it");
	ImGui::SameLine();
	ImGui::BeginDisabled(current == nullptr);
	if (ImGui::Button("Restart", ImVec2(90.0f, 0.0f)) && current != nullptr)
	{
		_runner.Start(*current);
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Runs the last scenario again from the start, on a fresh testbed");
	ImGui::SameLine();
	ImGui::BeginDisabled(!running);
	ImGui::PushStyleColor(ImGuiCol_Button, k_StopColour);
	if (ImGui::Button("Stop", ImVec2(90.0f, 0.0f)))
	{
		_runner.Stop();
	}
	ImGui::PopStyleColor();
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Gives no more commands and puts back the body time, fainting, footprints and clock; the "
	                      "creatures stay");
	ImGui::SameLine();
	if (ImGui::Button("Empty testbed") && Game::Instance() != nullptr)
	{
		_runner.Stop();
		Game::Instance()->LoadTestbed();
	}
	ImGui::SetItemTooltip("Loads the testbed afresh, with nothing on it");

	if (current != nullptr)
	{
		const auto& timeline = _runner.GetTimeline();
		ImGui::Text("%s %s, %.1f s in, %s", running ? "Running" : "Stopped", current->name.data(),
		            static_cast<double>(_runner.GetSeconds()),
		            current->commands.empty() ? "no commands"
		            : timeline.done
		                ? "every command given"
		                : fmt::format("next command {} of {}", timeline.next + 1, current->commands.size()).c_str());
		if (!_runner.GetLog().empty() && ImGui::TreeNode("Log"))
		{
			for (const auto& line : _runner.GetLog())
			{
				ImGui::TextUnformatted(line.c_str());
			}
			ImGui::TreePop();
		}
	}
}

void TestbedScenarios::DrawBenchmark() noexcept
{
	const auto progress = _runner.GetCrowdProgress();
	if (!progress.has_value())
	{
		return;
	}
	ImGui::SeparatorText("Benchmark");
	std::string counts;
	for (const auto& [name, count] : _runner.EntityCounts())
	{
		counts += fmt::format("{}{} {}", counts.empty() ? "" : ", ", count, name);
	}
	ImGui::TextUnformatted(counts.c_str());

	if (!progress->Done())
	{
		const auto fraction = static_cast<float>(progress->spawned) / static_cast<float>(std::max<size_t>(progress->total, 1));
		ImGui::ProgressBar(fraction, ImVec2(-1.0f, 0.0f),
		                   fmt::format("Spawning {} of {}", progress->spawned, progress->total).c_str());
		return;
	}
	ImGui::Text("Spawned %zu in %.0f ms over %u frames", progress->total, progress->spawnMs, progress->spawnFrames);
	ImGui::SetItemTooltip("The time spent creating the crowd's entities, apart from the rest of those frames");
	if (const auto warmUp = _runner.GetWarmUpLeft(); warmUp > 0)
	{
		ImGui::Text("Settling: %u frames before measuring", warmUp);
		return;
	}

	const auto& settings = _runner.GetBenchmarkSettings();
	const auto& results = _runner.GetLiveResults();
	ImGui::Text("Measured the last %zu of up to %u frames", _runner.GetMeasuredFrames(), settings.frames);
	const auto frame = results.frame;
	const auto colour = frame.average <= k_TargetFrameMs ? ImVec4(0.4f, 0.85f, 0.4f, 1.0f) : ImVec4(0.95f, 0.45f, 0.35f, 1.0f);
	ImGui::TextColored(colour, "Frame: mean %.2f ms (%.0f FPS), p95 %.2f, max %.2f", static_cast<double>(frame.average),
	                   frame.average > 0.0f ? 1000.0 / static_cast<double>(frame.average) : 0.0, static_cast<double>(frame.p95),
	                   static_cast<double>(frame.max));
	ImGui::Text("Update: mean %.2f ms, p95 %.2f   Render: mean %.2f ms, p95 %.2f", static_cast<double>(results.update.average),
	            static_cast<double>(results.update.p95), static_cast<double>(results.render.average),
	            static_cast<double>(results.render.p95));
	ImGui::Text("Draw calls: mean %.0f, max %.0f", static_cast<double>(results.draws.average),
	            static_cast<double>(results.draws.max));

	const auto stages = _runner.GetStages();
	if (!results.stages.empty() &&
	    ImGui::BeginTable("Benchmark stages", 6,
	                      ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
	{
		ImGui::TableSetupColumn("Stage");
		ImGui::TableSetupColumn("");
		ImGui::TableSetupColumn("Mean");
		ImGui::TableSetupColumn("p95");
		ImGui::TableSetupColumn("Max");
		ImGui::TableSetupColumn("When run");
		ImGui::TableHeadersRow();
		for (size_t i = 0; i < std::min(k_TopStages, results.stages.size()); ++i)
		{
			const auto& stage = results.stages[i];
			const auto& info = stages[stage.stage];
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(info.name.data(), info.name.data() + info.name.size());
			ImGui::TableNextColumn();
			ImGui::TextDisabled("%s", info.render ? "render" : "update");
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", static_cast<double>(stage.perFrame.average));
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", static_cast<double>(stage.perFrame.p95));
			ImGui::TableNextColumn();
			ImGui::Text("%.2f", static_cast<double>(stage.perFrame.max));
			ImGui::TableNextColumn();
			ImGui::Text("%.2f in %zu", static_cast<double>(stage.meanWhenRun), stage.framesRun);
		}
		ImGui::EndTable();
	}

	ImGui::BeginDisabled(_runner.GetMeasuredFrames() == 0);
	if (ImGui::Button("Save results"))
	{
		const auto path = _runner.SaveResults();
		_savedTo = path.has_value() ? path->generic_string() : "couldn't write them";
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Writes the frames measured so far to benchmarks/, as JSON and CSV, to compare with other runs");
	if (!_savedTo.empty())
	{
		ImGui::SameLine();
		ImGui::TextDisabled("%s", _savedTo.c_str());
	}
}

void TestbedScenarios::DrawTime() noexcept
{
	ImGui::SeparatorText("Time");
	if (auto* game = Game::Instance())
	{
		ImGui::TextUnformatted("Game");
		for (const auto& speed : k_Speeds)
		{
			ImGui::SameLine();
			if (ImGui::RadioButton(speed.name.data(), game->GetGameSpeed() == speed.multiplier))
			{
				game->SetGameSpeed(speed.multiplier);
			}
		}
	}
	if (Locator::creaturePhysiologySystem::has_value())
	{
		auto& physiology = Locator::creaturePhysiologySystem::value();
		ImGui::SameLine(0.0f, 24.0f);
		ImGui::TextUnformatted("Body");
		for (const auto scale : k_BodyTimes)
		{
			ImGui::SameLine();
			if (ImGui::RadioButton(fmt::format("x{:.0f}##body", scale).c_str(), physiology.GetTimeScale() == scale))
			{
				physiology.SetTimeScale(scale);
			}
		}
		ImGui::SetItemTooltip("Game turns of every creature's body that pass each game turn, to watch them grow and tire");
	}
}

void TestbedScenarios::DrawCamera() noexcept
{
	ImGui::SeparatorText("Camera");
	const auto creatures = _runner.GetCreatures();
	ImGui::BeginDisabled(creatures.empty());
	const auto shot = _runner.GetShot();
	if (ImGui::Button(shot == Shot::Follow ? "Following" : "Follow"))
	{
		_runner.Frame(Shot::Follow, _focus);
	}
	ImGui::SetItemTooltip("Behind and above the creature picked below, keeping up with it");
	ImGui::SameLine();
	if (ImGui::Button(shot == Shot::Head ? "Close up on the head" : "Head and eyes"))
	{
		_runner.Frame(Shot::Head, _focus, 1.4f);
	}
	ImGui::SetItemTooltip("In front of the picked creature's face, keeping up with it");
	ImGui::SameLine();
	if (ImGui::Button("Overview"))
	{
		_runner.Frame(Shot::Overview, _focus);
	}
	ImGui::SetItemTooltip("Above and to the south of all of the scenario, once");
	ImGui::SameLine();
	if (ImGui::Button("Testbed view"))
	{
		_runner.Frame(Shot::Testbed, _focus);
	}
	ImGui::EndDisabled();
	ImGui::SameLine();
	ImGui::BeginDisabled(!shot.has_value());
	if (ImGui::Button("Let go"))
	{
		_runner.ReleaseCamera();
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("The camera is yours again");
}

void TestbedScenarios::DrawCreatures() noexcept
{
	const auto* scenario = _runner.GetScenario();
	const auto creatures = _runner.GetCreatures();
	if (scenario == nullptr || creatures.empty() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	ImGui::SeparatorText("Its creatures");
	auto& registry = Locator::entitiesRegistry::value();
	if (!ImGui::BeginTable("Scenario creatures", 8,
	                       ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY |
	                           ImGuiTableFlags_SizingFixedFit))
	{
		return;
	}
	ImGui::TableSetupScrollFreeze(2, 1);
	ImGui::TableSetupColumn("");
	ImGui::TableSetupColumn("Creature");
	ImGui::TableSetupColumn("Doing");
	ImGui::TableSetupColumn("Speed");
	ImGui::TableSetupColumn("Needs");
	ImGui::TableSetupColumn("Size");
	ImGui::TableSetupColumn("Strongest desire");
	ImGui::TableSetupColumn("");
	ImGui::TableHeadersRow();
	for (size_t i = 0; i < creatures.size(); ++i)
	{
		const auto entity = creatures[i];
		ImGui::PushID(static_cast<int>(i));
		ImGui::TableNextRow();
		ImGui::TableNextColumn();
		auto focus = static_cast<int>(_focus);
		if (ImGui::RadioButton("##focus", &focus, static_cast<int>(i)))
		{
			_focus = i;
			if (const auto shot = _runner.GetShot(); shot == Shot::Follow || shot == Shot::Head)
			{
				_runner.Frame(*shot, _focus, *shot == Shot::Head ? 1.4f : 1.0f);
			}
		}
		const auto* creature = registry.Valid(entity) ? registry.TryGet<const Creature>(entity) : nullptr;
		const auto& setup = scenario->creatures[i];
		ImGui::TableNextColumn();
		const auto name = setup.label.empty() ? std::string(SpeciesName(setup.species))
		                                      : fmt::format("{}, {}", SpeciesName(setup.species), setup.label);
		ImGui::TextUnformatted(name.c_str());
		if (creature == nullptr)
		{
			ImGui::TableNextColumn();
			ImGui::TextDisabled("gone");
			ImGui::PopID();
			continue;
		}
		const auto* mind = registry.TryGet<const CreatureMindState>(entity);
		const auto* needs = registry.TryGet<const CreatureNeeds>(entity);
		const auto* locomotion = registry.TryGet<const CreatureLocomotion>(entity);
		ImGui::TableNextColumn();
		ImGui::Text("%s%s", mind != nullptr ? creature_mind::Name(mind->idle.activity).data() : "-",
		            mind != nullptr && mind->paused ? " (paused)" : "");
		if (locomotion != nullptr)
		{
			ImGui::SameLine();
			ImGui::TextDisabled("%s", MotionName(locomotion->motion).data());
		}
		ImGui::TableNextColumn();
		ImGui::Text("%.1f", locomotion != nullptr ? static_cast<double>(locomotion->speed) : 0.0);
		ImGui::TableNextColumn();
		if (needs != nullptr)
		{
			const auto& body = needs->needs;
			ImGui::Text("E %.2f T %.2f X %.2f P %.2f W %+.2f", static_cast<double>(body.energy),
			            static_cast<double>(body.dehydration), static_cast<double>(body.exhaustion),
			            static_cast<double>(body.poo), static_cast<double>(body.warmth));
			ImGui::SetItemTooltip("Energy, thirst, exhaustion, poo and warmth");
		}
		ImGui::TableNextColumn();
		ImGui::Text("%.2f", static_cast<double>(creature->size));
		ImGui::TableNextColumn();
		if (mind != nullptr && mind->desires.has_value())
		{
			if (const auto strongest = Strongest(*mind->desires))
			{
				ImGui::Text("%s %.2f", creature_desires::Name(*strongest).data(),
				            static_cast<double>((*mind->desires)[*strongest].value));
			}
		}
		ImGui::TableNextColumn();
		if (ImGui::SmallButton("Select"))
		{
			_spawner.Select(entity);
		}
		ImGui::SetItemTooltip("Picks it in the creature spawner, for its body, mind, movement and sounds");
		ImGui::PopID();
	}
	ImGui::EndTable();
}
