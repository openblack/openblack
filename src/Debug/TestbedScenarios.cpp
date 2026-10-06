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
	_runner.Update(seconds);
}

void TestbedScenarios::Draw() noexcept
{
	DrawPicker();
	DrawControls();
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
	ImGui::TextDisabled("%s testbed, %.1f o'clock%s, %s, body time x%.0f, %zu creature%s, %zu object%s, %zu command%s",
	                    environment.land == Land::Pool ? "Pool" : "Plain", static_cast<double>(environment.hour),
	                    environment.clockRuns ? "" : " (clock stopped)", Name(environment.weather).data(),
	                    static_cast<double>(environment.bodyTimeScale), scenario.creatures.size(),
	                    scenario.creatures.size() == 1 ? "" : "s", scenario.objects.size(),
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
		Game::Instance()->LoadTestbed(picked.environment.land == Land::Pool);
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
