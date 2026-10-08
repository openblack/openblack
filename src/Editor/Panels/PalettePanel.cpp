/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PalettePanel.h"

#include <algorithm>

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "Camera/Camera.h"
#include "Debug/CreatureSpawner.h"
#include "ECS/Registry.h"
#include "ECS/Systems/EditorSystemInterface.h"
#include "Editor/EditorContext.h"
#include "Editor/EditorEntities.h"
#include "Editor/EditorOutline.h"
#include "Editor/EditorStyle.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/DispenserRules.h"

namespace openblack::editor
{

namespace
{
/// How far apart a laid out row puts things of each kind
float SpacingOf(PlaceKind kind)
{
	switch (kind)
	{
	case PlaceKind::Creature:
		return 30.0f;
	case PlaceKind::Villager:
		return 5.0f;
	case PlaceKind::Building:
		return 45.0f;
	case PlaceKind::Tree:
		return 14.0f;
	case PlaceKind::Feature:
		return 30.0f;
	case PlaceKind::MobileObject:
		return 6.0f;
	case PlaceKind::Dispenser:
	case PlaceKind::MiracleBubble:
		return 12.0f;
	case PlaceKind::MobileStatic:
	default:
		return 16.0f;
	}
}

/// The most a layout puts down at once
constexpr size_t k_MaxLayout = 400;

template <typename Table>
std::vector<int32_t> AllOf(const Table& table)
{
	std::vector<int32_t> types(table.size());
	for (size_t i = 0; i < types.size(); ++i)
	{
		types.at(i) = static_cast<int32_t>(i);
	}
	return types;
}

/// The types of a table whose tribe is the one picked, or of no tribe for the entry after the last tribe
template <typename Table>
std::vector<int32_t> OfTribe(const Table& table, int tribe)
{
	std::vector<int32_t> types;
	for (size_t i = 0; i < table.size(); ++i)
	{
		const auto infoTribe = static_cast<int>(table.at(i).tribeType);
		const bool ofNoTribe = infoTribe < 0 || infoTribe >= static_cast<int>(Tribe::_COUNT);
		if (tribe >= static_cast<int>(Tribe::_COUNT) ? ofNoTribe : infoTribe == tribe)
		{
			types.push_back(static_cast<int32_t>(i));
		}
	}
	return types;
}

bool TribeCombo(int& tribe)
{
	constexpr auto k_Count = static_cast<int>(Tribe::_COUNT);
	const auto name = [](int index) {
		return index < k_Count ? TitleCase(k_TribeStrs.at(static_cast<size_t>(index))) : std::string("Others");
	};
	bool changed = false;
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 9.0f);
	if (ImGui::BeginCombo("Tribe", name(tribe).c_str()))
	{
		for (int i = 0; i <= k_Count; ++i)
		{
			if (ImGui::Selectable(name(i).c_str(), i == tribe))
			{
				tribe = i;
				changed = true;
			}
		}
		ImGui::EndCombo();
	}
	return changed;
}
} // namespace

void PalettePanel::Draw(EditorContext& context) noexcept
{
	if (!Locator::infoConstants::has_value())
	{
		ImGui::TextDisabled("The game's tables aren't loaded");
		return;
	}
	const auto& info = Locator::infoConstants::value();
	if (context.placement.item.has_value())
	{
		const auto& item = *context.placement.item;
		ImGui::TextColored(style::k_Good, "Placing %s: click the land to put it down, the wheel or [ ] turns it, Esc stops",
		                   std::string(context.names.NameOf(item.kind, item.type)).c_str());
	}
	else if (!context.placement.last.empty())
	{
		ImGui::TextColored(style::k_Muted, "%s", context.placement.last.c_str());
	}
	else
	{
		ImGui::TextColored(style::k_Muted, "Pick something to place it on the land");
	}

	if (!ImGui::BeginTabBar("Palette"))
	{
		return;
	}
	if (ImGui::BeginTabItem("Creatures"))
	{
		DrawCreatures(context);
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Villagers"))
	{
		TribeCombo(_tribe);
		ImGui::SameLine();
		DrawList(context, PlaceKind::Villager, OfTribe(info.villager, _tribe));
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Buildings"))
	{
		TribeCombo(_tribe);
		ImGui::SameLine();
		DrawList(context, PlaceKind::Building, OfTribe(info.abode, _tribe));
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Trees"))
	{
		DrawList(context, PlaceKind::Tree, AllOf(info.tree));
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Features"))
	{
		DrawList(context, PlaceKind::Feature, AllOf(info.feature));
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Mobile objects"))
	{
		DrawList(context, PlaceKind::MobileObject, AllOf(info.mobileObject));
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Mobile statics"))
	{
		DrawList(context, PlaceKind::MobileStatic, AllOf(info.mobileStatic));
		ImGui::EndTabItem();
	}
	if (ImGui::BeginTabItem("Miracles"))
	{
		DrawMiracles(context);
		ImGui::EndTabItem();
	}
	ImGui::EndTabBar();
}

void PalettePanel::DrawCreatures(EditorContext& context) noexcept
{
	const auto species = context.spawner.GetSpecies();
	const auto placing = context.placement.item.has_value() && context.placement.item->kind == PlaceKind::Creature;
	ImGui::PushStyleColor(ImGuiCol_Button, placing ? ImVec4(0.55f, 0.25f, 0.20f, 1.0f) : ImVec4(0.20f, 0.45f, 0.25f, 1.0f));
	if (ImGui::Button(placing ? "Stop placing creatures" : "Place creatures", ImVec2(ImGui::GetFontSize() * 12.0f, 0.0f)))
	{
		context.placement.item.reset();
		if (!placing)
		{
			context.placement.item = PlaceItem {.kind = PlaceKind::Creature, .type = static_cast<int32_t>(species)};
		}
	}
	ImGui::PopStyleColor();
	ImGui::SameLine();
	ImGui::TextColored(style::k_Muted, "As set up below; each click puts one down");
	ImGui::BeginChild("CreatureSettings",
	                  ImVec2(std::min(ImGui::GetContentRegionAvail().x, ImGui::GetFontSize() * 40.0f), 0.0f));
	context.spawner.DrawSpawnSettings();
	ImGui::EndChild();
	// The species can change while placing
	if (placing && context.placement.item.has_value())
	{
		context.placement.item->type = static_cast<int32_t>(context.spawner.GetSpecies());
	}
}

void PalettePanel::DrawMiracles(EditorContext& context) noexcept
{
	// A dispenser floats a new bubble a while after its last is taken; a bubble on its own gives its miracle once
	ImGui::RadioButton("Dispensers", &_miracleKind, 0);
	ImGui::SameLine();
	ImGui::RadioButton("Bubbles", &_miracleKind, 1);
	std::vector<int32_t> types;
	for (const auto type : magic::DispensableMiracles())
	{
		types.push_back(static_cast<int32_t>(type));
	}
	DrawList(context, _miracleKind == 0 ? PlaceKind::Dispenser : PlaceKind::MiracleBubble, types);
}

void PalettePanel::DrawList(EditorContext& context, PlaceKind kind, const std::vector<int32_t>& types) noexcept
{
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
	ImGui::InputTextWithHint("##Search", "Search", &_search);
	std::vector<int32_t> shown;
	for (const auto type : types)
	{
		if (MatchesSearch(context.names.NameOf(kind, type), _search))
		{
			shown.push_back(type);
		}
	}
	ImGui::SameLine();
	ImGui::BeginDisabled(shown.empty());
	if (ImGui::Button(fmt::format("Lay out all {}", shown.size()).c_str()))
	{
		LayOut(context, kind, shown);
	}
	ImGui::EndDisabled();
	ImGui::SetItemTooltip("Puts one of each shown in rows round the middle of the view");
	if (!_lastLayout.empty())
	{
		ImGui::SameLine();
		ImGui::TextColored(style::k_Muted, "%s", _lastLayout.c_str());
	}

	const auto cellWidth = ImGui::GetFontSize() * 11.0f;
	const auto columns = std::max(1, static_cast<int>(ImGui::GetContentRegionAvail().x / cellWidth));
	if (!ImGui::BeginTable("Items", columns, ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchSame))
	{
		return;
	}
	for (const auto type : shown)
	{
		ImGui::TableNextColumn();
		ImGui::PushID(type);
		const PlaceItem item {.kind = kind, .type = type};
		const bool picked = context.placement.item == item;
		if (ImGui::Selectable(std::string(context.names.NameOf(kind, type)).c_str(), picked))
		{
			if (picked)
			{
				context.placement.item.reset();
			}
			else
			{
				context.placement.item = item;
			}
		}
		ImGui::SetItemTooltip("Type %d", type);
		ImGui::PopID();
	}
	ImGui::EndTable();
}

void PalettePanel::LayOut(EditorContext& context, PlaceKind kind, const std::vector<int32_t>& types) noexcept
{
	if (!Locator::camera::has_value())
	{
		return;
	}
	const auto& camera = Locator::camera::value();
	const auto hit = camera.RaycastScreenCoordToLand({0.5f, 0.5f}, false);
	const auto centre = hit.has_value() ? hit->position : camera.GetFocus();
	const auto count = std::min(types.size(), k_MaxLayout);
	const auto points = GridLayout(count, SpacingOf(kind));
	size_t placed = 0;
	for (size_t i = 0; i < count; ++i)
	{
		const glm::vec2 point {centre.x + points.at(i).x, centre.z + points.at(i).y};
		const PlaceItem item {.kind = kind, .type = types.at(i)};
		const auto entity = kind == PlaceKind::Creature ? context.spawner.SpawnAt({point.x, LandHeight(point), point.y}, 0.0f)
		                                                : Place(item, {point.x, LandHeight(point), point.y}, 0.0f);
		placed += entity != entt::null ? 1 : 0;
	}
	_lastLayout = fmt::format("Laid out {} of {}", placed, types.size());
}

} // namespace openblack::editor
