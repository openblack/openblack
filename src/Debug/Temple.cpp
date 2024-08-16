/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Temple.h"

#include <unordered_map>

#include "3D/TempleInteriorInterface.h"
#include "Locator.h"

using namespace openblack::debug::gui;

TempleInterior::TempleInterior() noexcept
    : Window("Citadel", ImVec2(250.0f, 165.0f))
{
}

void TempleInterior::Draw() noexcept
{
	if (Locator::temple::has_value())
	{
		auto& temple = Locator::temple::value();
		const auto active = !temple.Active();

		if (ImGui::Button(active ? "Enter temple" : "Exit temple"))
		{
			if (active)
			{
				temple.Activate();
			}
			else
			{
				temple.Deactivate();
			}
		}
	}
	else
	{
		ImGui::Text("No temple");
	}
}

void TempleInterior::Update() noexcept {}

void TempleInterior::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void TempleInterior::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
