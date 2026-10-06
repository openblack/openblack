/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <optional>

#include "Creature/CreatureFootprints.h"
#include "CreatureSpawner.h"
#include "ECS/Systems/FootprintSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;

void CreatureSpawner::DrawFootprintSettings() noexcept
{
	if (!Locator::footprintSystem::has_value())
	{
		return;
	}
	auto& footprints = Locator::footprintSystem::value();
	bool shown = footprints.IsShown();
	if (ImGui::Checkbox("Show footprints", &shown))
	{
		footprints.SetShown(shown);
	}
	ImGui::SameLine();
	bool aprilFools = footprints.GetAprilFoolsOverride().value_or(false);
	if (ImGui::Checkbox("1 April", &aprilFools))
	{
		footprints.SetAprilFoolsOverride(aprilFools ? std::optional(true) : std::nullopt);
	}
	ImGui::SetItemTooltip("Every creature leaves smiley faces, as on the first of April; unticked, it goes by the date");
	ImGui::SameLine();
	ImGui::Text("%zu of %zu prints, %zu dropped%s", footprints.GetPrints().size(), creature_footprints::k_Capacity,
	            footprints.GetDroppedCount(), footprints.IsAprilFools() ? ", smileys" : "");
}
