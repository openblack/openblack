/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureSpawner.h"

#include <algorithm>
#include <array>
#include <string_view>
#include <vector>

#include <SDL_events.h>
#include <glm/trigonometric.hpp>

#include "3D/CreatureBody.h"
#include "Camera/Camera.h"
#include "Creature/CreatureMorph.h"
#include "ECS/Archetypes/CreatureArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using namespace openblack::debug::gui;
using openblack::ecs::archetypes::CreatureArchetype;
using openblack::ecs::components::Creature;
using openblack::ecs::components::CreatureMorph;
using openblack::ecs::components::Transform;

namespace
{
// Indexed by species, from the cow
constexpr std::array<std::string_view, static_cast<size_t>(CreatureType::_COUNT) - 1> k_SpeciesNames {
    "Cow",        "Tiger", "Leopard", "Wolf", "Lion",     "Horse", "Tortoise", "Zebra",     "Brown Bear",
    "Polar Bear", "Sheep", "Chimp",   "Ogre", "Mandrill", "Rhino", "Gorilla",  "Giant Ape",
};

constexpr std::array<std::string_view, static_cast<size_t>(PlayerNames::_COUNT)> k_OwnerNames {
    "Player One", "Player Two", "Player Three", "Player Four", "Player Five", "Player Six", "Player Seven", "Neutral",
};

const ImVec4 k_PlacingColour {0.85f, 0.30f, 0.25f, 1.0f};
const ImVec4 k_StartColour {0.25f, 0.60f, 0.30f, 1.0f};

std::string_view SpeciesName(CreatureType species)
{
	const auto index = static_cast<size_t>(species);
	return index >= 1 && index <= k_SpeciesNames.size() ? k_SpeciesNames.at(index - 1) : "Unknown";
}

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

bool IsRightButton(const SDL_Event& event)
{
	return (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP) && event.button.button == SDL_BUTTON_RIGHT;
}
} // namespace

CreatureSpawner::CreatureSpawner() noexcept
    : Window("Creature Spawner", ImVec2(420.0f, 520.0f))
{
}

void CreatureSpawner::Close() noexcept
{
	_placing = false;
	_placeAt.reset();
	Window::Close();
}

void CreatureSpawner::Draw() noexcept
{
	DrawSettings();
	ImGui::Separator();
	DrawPlacing();
	ImGui::Separator();
	DrawCreatures();
}

void CreatureSpawner::UseSpeciesDefaults() noexcept
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	const auto body = CreatureArchetype::StartBody(_species);
	_alignment = body.alignment;
	_fatness = body.fatness;
	_strength = body.strength;
	_scale = CreatureArchetype::StartScale(_species);
	_defaultsFor = _species;
}

void CreatureSpawner::DrawSettings() noexcept
{
	if (_defaultsFor != _species)
	{
		UseSpeciesDefaults();
	}

	ImGui::SeparatorText("Creature");

	if (ImGui::BeginCombo("Species", SpeciesName(_species).data()))
	{
		for (size_t i = 0; i < k_SpeciesNames.size(); ++i)
		{
			const auto species = static_cast<CreatureType>(i + 1);
			if (ImGui::Selectable(k_SpeciesNames.at(i).data(), species == _species))
			{
				_species = species;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::BeginCombo("Owner", k_OwnerNames.at(static_cast<size_t>(_owner)).data()))
	{
		for (size_t i = 0; i < k_OwnerNames.size(); ++i)
		{
			const auto owner = static_cast<PlayerNames>(i);
			if (ImGui::Selectable(k_OwnerNames.at(i).data(), owner == _owner))
			{
				_owner = owner;
			}
		}
		ImGui::EndCombo();
	}

	ImGui::SeparatorText("Body");
	ImGui::SliderFloat("Alignment", &_alignment, -1.0f, 1.0f,
	                   _alignment < 0.0f ? "Evil %.2f" : (_alignment > 0.0f ? "Good %.2f" : "Neutral %.2f"));
	ImGui::SliderFloat("Fatness", &_fatness, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Strength", &_strength, 0.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Size", &_scale, creature_morph::k_MinScale, creature_morph::k_MaxScale, "%.2f",
	                   ImGuiSliderFlags_Logarithmic);
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Creatures grow by themselves up to size %.1f", static_cast<double>(creature_morph::k_MaxGrownScale));
	}
	if (ImGui::Button("Species defaults"))
	{
		UseSpeciesDefaults();
	}
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("As a new creature of the species is: neutral, with its size, fatness and strength");
	}

	const auto* info = SpeciesInfo(_species);
	const auto morph = creature_morph::FromAttributes(
	    _alignment, _fatness, _strength, info != nullptr ? info->strength : creature_morph::k_UnknownSpeciesStrength);
	ImGui::Text("Evil-good %+.2f, thin-fat %+.2f, weak-strong %+.2f", static_cast<double>(morph.evilGood),
	            static_cast<double>(morph.thinFat), static_cast<double>(morph.weakStrong));
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("How far the body is pulled from its base mesh towards each axis' mesh, -1 to 1");
	}

	DrawSelected();

	ImGui::SeparatorText("Facing");
	ImGui::Checkbox("Random", &_randomFacing);
	if (!_randomFacing)
	{
		ImGui::SliderFloat("Degrees", &_facingDegrees, 0.0f, 360.0f, "%.0f");
	}
}

void CreatureSpawner::DrawSelected() noexcept
{
	if (!_selected.has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	auto* creature = registry.TryGet<Creature>(*_selected);
	auto* transform = registry.TryGet<Transform>(*_selected);
	if (creature == nullptr || transform == nullptr)
	{
		_selected.reset();
		return;
	}

	ImGui::SeparatorText("Selected creature");
	ImGui::Text("%s at %.0f, %.0f", SpeciesName(creature->species).data(), static_cast<double>(transform->position.x),
	            static_cast<double>(transform->position.z));
	ImGui::SliderFloat("Its alignment", &creature->alignment, -1.0f, 1.0f, "%.2f");
	ImGui::SliderFloat("Its fatness", &creature->fatness, 0.0f, 1.0f, "%.2f");
	if (ImGui::IsItemHovered())
	{
		ImGui::SetTooltip("Its body follows its fatness by at most %.2f a game turn",
		                  static_cast<double>(creature_morph::k_MaxFatnessStep));
	}
	ImGui::SliderFloat("Its strength", &creature->strength, 0.0f, 1.0f, "%.2f");
	if (ImGui::SliderFloat("Its size", &creature->size, creature_morph::k_MinScale, creature_morph::k_MaxScale, "%.2f",
	                       ImGuiSliderFlags_Logarithmic))
	{
		creature->size = creature_morph::ClampScale(creature->size);
		transform->scale = glm::vec3(CreatureArchetype::DrawnScale(creature->species, creature->size));
		registry.SetDirty();
	}
	if (auto* morph = registry.TryGet<CreatureMorph>(*_selected); morph != nullptr)
	{
		ImGui::Text("Drawn: evil-good %+.2f, thin-fat %+.2f, weak-strong %+.2f", static_cast<double>(morph->drawn.evilGood),
		            static_cast<double>(morph->drawn.thinFat), static_cast<double>(morph->drawn.weakStrong));
		ImGui::Text("Fatness shown %.2f", static_cast<double>(morph->shownFatness));
		ImGui::SameLine();
		if (ImGui::SmallButton("Show now"))
		{
			morph->shownFatness = creature->fatness;
		}
	}
	if (ImGui::Button("Deselect"))
	{
		_selected.reset();
	}
}

void CreatureSpawner::DrawPlacing() noexcept
{
	const auto colour = _placing ? k_PlacingColour : k_StartColour;
	ImGui::PushStyleColor(ImGuiCol_Button, colour);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(colour.x * 1.15f, colour.y * 1.15f, colour.z * 1.15f, 1.0f));
	if (ImGui::Button(_placing ? "Stop Placing Creatures" : "Start Placing Creatures", ImVec2(-1.0f, 0.0f)))
	{
		_placing = !_placing;
		_placeAt.reset();
	}
	ImGui::PopStyleColor(2);
	if (_placing)
	{
		ImGui::TextWrapped("Right click on the land to place a creature there");
	}
}

void CreatureSpawner::DrawCreatures() noexcept
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();

	std::vector<entt::entity> creatures;
	registry.Each<const Creature>([&creatures](entt::entity entity, const Creature&) { creatures.push_back(entity); });

	ImGui::SeparatorText("On the land");
	ImGui::Text("%zu creature%s", creatures.size(), creatures.size() == 1 ? "" : "s");
	if (_selected.has_value() && std::ranges::find(creatures, *_selected) == creatures.end())
	{
		_selected.reset();
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(creatures.empty());
	if (ImGui::Button("Remove all"))
	{
		registry.Destroy(creatures.begin(), creatures.end());
		creatures.clear();
		_selected.reset();
	}
	ImGui::EndDisabled();

	std::optional<entt::entity> remove;
	if (ImGui::BeginTable("Creatures", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
	                      ImVec2(0.0f, 0.0f)))
	{
		ImGui::TableSetupColumn("Species");
		ImGui::TableSetupColumn("Owner");
		ImGui::TableSetupColumn("Where");
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
		ImGui::TableHeadersRow();
		for (const auto entity : creatures)
		{
			const auto& creature = registry.Get<Creature>(entity);
			const auto& transform = registry.Get<Transform>(entity);
			ImGui::PushID(static_cast<int>(entt::to_integral(entity)));
			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(SpeciesName(creature.species).data());
			ImGui::TableNextColumn();
			ImGui::TextUnformatted(k_OwnerNames.at(static_cast<size_t>(creature.owner)).data());
			ImGui::TableNextColumn();
			ImGui::Text("%.0f, %.0f", static_cast<double>(transform.position.x), static_cast<double>(transform.position.z));
			ImGui::TableNextColumn();
			const bool selected = _selected == entity;
			if (ImGui::SmallButton(selected ? "Selected" : "Select"))
			{
				_selected = entity;
			}
			ImGui::TableNextColumn();
			if (ImGui::SmallButton("Remove"))
			{
				remove = entity;
			}
			ImGui::PopID();
		}
		ImGui::EndTable();
	}
	if (remove.has_value())
	{
		if (_selected == *remove)
		{
			_selected.reset();
		}
		registry.Destroy(*remove);
	}
}

void CreatureSpawner::Update() noexcept
{
	if (_placeAt.has_value())
	{
		Spawn(*_placeAt);
		_placeAt.reset();
	}
}

void CreatureSpawner::Spawn(glm::vec2 screenCoord) noexcept
{
	if (!Locator::terrainSystem::has_value() || !Locator::dynamicsSystem::has_value())
	{
		return;
	}
	const auto hit = Locator::camera::value().RaycastScreenCoordToLand(screenCoord, false);
	if (!hit.has_value())
	{
		return;
	}
	const auto degrees = _randomFacing ? std::uniform_real_distribution(0.0f, 360.0f)(_random) : _facingDegrees;
	CreatureArchetype::Create(hit->position, _owner, _species, 0, glm::radians(degrees), _scale,
	                          {.alignment = _alignment, .fatness = _fatness, .strength = _strength});
}

bool CreatureSpawner::TakesEvent(const SDL_Event& event) const noexcept
{
	// While placing, a right click on the land is the window's rather than the hand's
	return _placing && IsRightButton(event) && !ImGui::GetIO().WantCaptureMouse;
}

void CreatureSpawner::ProcessEventOpen(const SDL_Event& event) noexcept
{
	if (!TakesEvent(event) || event.type != SDL_MOUSEBUTTONDOWN || !Locator::windowing::has_value())
	{
		return;
	}
	const auto size = static_cast<glm::vec2>(Locator::windowing::value().GetSize());
	_placeAt = glm::vec2(static_cast<float>(event.button.x), static_cast<float>(event.button.y)) / size;
}

void CreatureSpawner::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
