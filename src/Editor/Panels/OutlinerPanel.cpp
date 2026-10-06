/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "OutlinerPanel.h"

#include <algorithm>

#include <fmt/format.h>
#include <imgui.h>
#include <imgui_stdlib.h>

#include "ECS/Components/Hand.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/EditorSystemInterface.h"
#include "Editor/EditorContext.h"
#include "Editor/EditorEntities.h"
#include "Editor/EditorStyle.h"

namespace openblack::editor
{

namespace
{
/// Frames between readings of the registry, about half a second
constexpr int k_RefreshFrames = 30;
} // namespace

void OutlinerPanel::Rebuild(EditorContext& context) noexcept
{
	_entries.clear();
	const auto& registry = context.registry;
	registry.Each<const ecs::components::Transform>([&](entt::entity entity, const ecs::components::Transform&) {
		// The player's hands are the game's, not things on the land
		if (registry.AllOf<ecs::components::Hand>(entity))
		{
			return;
		}
		const auto kind = KindOf(registry, entity);
		_entries.push_back({.entity = entity, .kind = kind, .label = LabelOf(registry, context.names, entity)});
	});
	_groupedFor = "\x01"; // Groups again
	_framesToRefresh = k_RefreshFrames;
}

void OutlinerPanel::Draw(EditorContext& context) noexcept
{
	if (--_framesToRefresh <= 0)
	{
		Rebuild(context);
	}
	if (_groupedFor != _search)
	{
		_groups = Group(_entries, _search);
		_groupedFor = _search;
	}

	ImGui::SetNextItemWidth(-ImGui::GetFrameHeight() - ImGui::GetStyle().ItemSpacing.x);
	ImGui::InputTextWithHint("##Search", "Search", &_search);
	ImGui::SameLine();
	if (ImGui::Button("R", ImVec2(ImGui::GetFrameHeight(), 0.0f)))
	{
		Rebuild(context);
	}
	ImGui::SetItemTooltip("Read the list again now");
	ImGui::TextColored(style::k_Muted, "%zu things", _entries.size());

	auto& selection = context.system.GetSelection();
	const bool scrollToSelection = selection.GetPicks() != _scrolledFor;
	_scrolledFor = selection.GetPicks();

	ImGui::BeginChild("Tree");
	for (const auto& group : _groups)
	{
		const auto title =
		    group.entries.size() == group.total
		        ? fmt::format("{} ({})###{}", Name(group.kind), group.total, Name(group.kind))
		        : fmt::format("{} ({} of {})###{}", Name(group.kind), group.entries.size(), group.total, Name(group.kind));
		const auto selected = selection.Get();
		const bool holdsSelection = selected.has_value() && std::ranges::any_of(group.entries, [&](const auto* entry) {
			                            return entry->entity == *selected;
		                            });
		if (holdsSelection && scrollToSelection)
		{
			ImGui::SetNextItemOpen(true);
		}
		if (!ImGui::TreeNodeEx(title.c_str(), _search.empty() ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_DefaultOpen))
		{
			continue;
		}
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(group.entries.size()));
		if (holdsSelection && scrollToSelection)
		{
			const auto found =
			    std::ranges::find_if(group.entries, [&](const auto* entry) { return entry->entity == *selected; });
			clipper.IncludeItemByIndex(static_cast<int>(found - group.entries.begin()));
		}
		while (clipper.Step())
		{
			for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row)
			{
				const auto& entry = *group.entries.at(static_cast<size_t>(row));
				ImGui::PushID(static_cast<int>(entt::to_integral(entry.entity)));
				const bool isSelected = selection.IsSelected(entry.entity);
				if (ImGui::Selectable(RowLabel(entry).c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
				{
					if (context.registry.Valid(entry.entity))
					{
						selection.Select(entry.entity);
						_scrolledFor = selection.GetPicks();
						if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						{
							context.system.FrameSelection();
						}
					}
				}
				if (isSelected && scrollToSelection)
				{
					ImGui::SetScrollHereY(0.4f);
				}
				if (ImGui::BeginPopupContextItem())
				{
					if (context.registry.Valid(entry.entity))
					{
						selection.Select(entry.entity);
						_scrolledFor = selection.GetPicks();
						if (ImGui::MenuItem("Bring into view"))
						{
							context.system.FrameSelection();
						}
						if (ImGui::MenuItem("Orbit"))
						{
							context.system.SetCameraMode(ecs::systems::EditorSystemInterface::CameraMode::Orbit);
						}
						if (ImGui::MenuItem("Follow"))
						{
							context.system.SetCameraMode(ecs::systems::EditorSystemInterface::CameraMode::Follow);
						}
						ImGui::Separator();
						if (ImGui::MenuItem("Duplicate", "Ctrl+D"))
						{
							if (const auto copy = Duplicate(entry.entity, {8.0f, 0.0f}))
							{
								selection.Select(*copy);
							}
							Invalidate();
						}
						if (ImGui::MenuItem("Delete", "Del"))
						{
							selection.Clear();
							Remove(entry.entity);
							Invalidate();
						}
					}
					ImGui::EndPopup();
				}
				ImGui::PopID();
			}
		}
		ImGui::TreePop();
	}
	ImGui::EndChild();
}

} // namespace openblack::editor
