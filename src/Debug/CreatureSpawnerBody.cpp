/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <glm/geometric.hpp>

#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreaturePhysiology.h"
#include "CreatureSpawner.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureMindSystemInterface.h"
#include "ECS/Systems/CreaturePhysiologySystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureMindState;
using openblack::ecs::components::CreatureMorph;
using openblack::ecs::components::CreatureNeeds;
using openblack::ecs::components::Transform;

namespace
{
/// The speeds the body's time can run at, to watch a creature grow and get hungry
constexpr std::array<float, 5> k_TimeScales {1.0f, 10.0f, 100.0f, 1000.0f, 3600.0f};
/// Food is dropped this far in front of the creature for its size
constexpr float k_FoodAhead = 6.0f;
/// The food dropped for a creature to eat: a pot of food, worth as much as the food in it, here a meal for a grown up
/// creature
constexpr auto k_FoodPot = PotInfo::FoodPot;
constexpr int32_t k_FoodAmount = 800;

constexpr float k_BarWidth = 160.0f;

std::string_view RestName(CreatureNeeds::Rest rest)
{
	constexpr std::array<std::string_view, 4> k_Names {"awake", "asleep", "resting", "out cold"};
	return k_Names.at(static_cast<size_t>(rest));
}

/// A bar of a value from low to high, with a slider beside it to set it
void Bar(const char* label, float& value, float low, float high, const char* tooltip)
{
	const auto fraction = high > low ? (value - low) / (high - low) : 0.0f;
	ImGui::ProgressBar(std::clamp(fraction, 0.0f, 1.0f), ImVec2(k_BarWidth, 0.0f), fmt::format("{:.3f}", value).c_str());
	ImGui::SameLine();
	ImGui::PushID(label);
	ImGui::SetNextItemWidth(90.0f);
	ImGui::SliderFloat("##set", &value, low, high, "set");
	ImGui::PopID();
	ImGui::SameLine();
	ImGui::TextUnformatted(label);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("%s", tooltip);
	}
}
} // namespace

void CreatureSpawner::DrawBody(entt::entity entity) noexcept
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* creature = registry.TryGet<Creature>(entity);
	auto* needs = registry.TryGet<CreatureNeeds>(entity);
	auto* transform = registry.TryGet<Transform>(entity);
	if (creature == nullptr || needs == nullptr || transform == nullptr)
	{
		return;
	}

	ImGui::SeparatorText("Body and needs");
	if (Locator::creaturePhysiologySystem::has_value())
	{
		auto& physiology = Locator::creaturePhysiologySystem::value();
		const auto scale = physiology.GetTimeScale();
		ImGui::TextUnformatted("Body time");
		for (const auto option : k_TimeScales)
		{
			ImGui::SameLine();
			if (ImGui::RadioButton(fmt::format("x{:.0f}", option).c_str(), scale == option))
			{
				physiology.SetTimeScale(option);
			}
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Game turns of every creature's body that pass each game turn, to watch them grow and tire");
		}
		bool fainting = physiology.IsFaintingEnabled();
		if (ImGui::Checkbox("Creatures faint", &fainting))
		{
			physiology.SetFaintingEnabled(fainting);
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Grown up creatures owned by a player faint when exhausted, starved or out of life");
		}
	}

	auto& body = needs->needs;
	ImGui::Text("Age %u (game hours), %u turns, %u meals, %s for %u turns%s", body.age, body.turns, body.meals,
	            RestName(needs->rest).data(), needs->restTurns, needs->rested ? ", rested" : "");
	ImGui::Text("%s%s", needs->moving ? "On the move" : "Standing still",
	            needs->moving ? ": tiring, not growing" : ": growing, not tiring");
	Bar("Energy", body.energy, 0.0f, std::max(1.0f, creature->size),
	    "Hunger is 1 less energy; a big meal fills it up to its size");
	Bar("Exhaustion", body.exhaustion, 0.0f, 1.0f, "Slowed to its slow speed from 0.8, faints at 1");
	Bar("Thirst", body.dehydration, 0.0f, 1.0f, "Fills in about 83 minutes; drinking clears it");
	Bar("Poo", body.poo, 0.0f, 1.0f, "Builds up from meals");
	Bar("Life", body.life, 0.0f, 1.0f, "Heals while asleep");
	Bar("Warmth", body.warmth, -1.0f, 1.0f, "Cold below 0, hot above, from the temperature against its species' comfort");
	Bar("Itchiness", body.itchiness, 0.0f, 1.0f, "Only the itchy spell makes a creature itch");
	Bar("Fatness", creature->fatness, 0.0f, 1.0f, "Burnt while hungry, put on by overeating");
	if (const auto* morph = registry.TryGet<const CreatureMorph>(entity))
	{
		ImGui::SameLine();
		ImGui::Text("(shown %.2f)", static_cast<double>(morph->shownFatness));
	}
	Bar("Strength", creature->strength, 0.0f, 1.0f, "Gained from hard work");
	const auto oldSize = creature->size;
	Bar("Size", creature->size, creature_morph::k_MinScale, creature_physiology::k_MaxGrownSize,
	    "Grows while standing still, fast while young");
	if (creature->size != oldSize)
	{
		creature->size = creature_morph::ClampScale(creature->size);
		transform->scale = glm::vec3(ecs::archetypes::CreatureArchetype::DrawnScale(creature->species, creature->size));
		registry.SetDirty();
	}
	if (ImGui::SmallButton("Fill up"))
	{
		body.energy = 1.0f;
		body.exhaustion = 0.0f;
		body.dehydration = 0.0f;
		body.poo = 0.0f;
		body.life = 1.0f;
		body.warmth = 0.0f;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Starve"))
	{
		body.energy = 0.05f;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Tire out"))
	{
		body.exhaustion = 0.85f;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Parch"))
	{
		body.dehydration = 0.9f;
	}
	ImGui::SameLine();
	if (ImGui::SmallButton("Bloat"))
	{
		body.poo = 0.9f;
	}

	if (!Locator::creatureMindSystem::has_value())
	{
		return;
	}
	auto& minds = Locator::creatureMindSystem::value();
	ImGui::TextUnformatted("See to it now");
	const auto report = [this](std::string_view what, bool done) {
		_lastNeed = fmt::format("{}: {}", what, done ? "started" : "nothing to do it with");
	};
	if (ImGui::Button("Sleep"))
	{
		report("Sleep", minds.Sleep(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Wake"))
	{
		minds.Wake(entity);
	}
	ImGui::SameLine();
	if (ImGui::Button("Eat"))
	{
		report("Eat the nearest food", minds.Eat(entity, std::nullopt));
	}
	ImGui::SameLine();
	if (ImGui::Button("Drink"))
	{
		report("Drink at the nearest water", minds.Drink(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Poo"))
	{
		report("Poo", minds.Poo(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Puke"))
	{
		report("Puke", minds.Puke(entity));
	}
	ImGui::SameLine();
	if (ImGui::Button("Faint"))
	{
		report("Faint", minds.Faint(entity));
	}
	if (ImGui::Button("Drop food in front") && Locator::infoConstants::has_value())
	{
		auto ahead = -(transform->rotation * glm::vec3(0.0f, 0.0f, 1.0f));
		ahead.y = 0.0f;
		ahead = glm::length(ahead) > 0.0f ? glm::normalize(ahead) : glm::vec3(0.0f, 0.0f, -1.0f);
		const auto at = transform->position + (ahead * (k_FoodAhead * std::max(creature->size, 1.0f)));
		ecs::archetypes::PotArchetype::Create(at, 0.0f, k_FoodPot, k_FoodAmount);
		_lastNeed = fmt::format("Dropped a pot of {} food", k_FoodAmount);
	}
	if (!_lastNeed.empty())
	{
		ImGui::TextUnformatted(_lastNeed.c_str());
	}
}
