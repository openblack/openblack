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

#include <fmt/format.h>

#include "3D/TempleInteriorInterface.h"
#include "ECS/Components/Temple.h"
#include "Locator.h"

using namespace openblack;
using namespace debug::gui;

TempleInterior::TempleInterior() noexcept
    : Window("Citadel", ImVec2(250.0f, 150.0f))
{
}

const std::array<std::tuple<TempleRoom, std::string_view>, 7> k_Lookup {
    std::make_tuple(TempleRoom::Challenge, "Challenge Room"),   //
    std::make_tuple(TempleRoom::CreatureCave, "Creature Cave"), //
    std::make_tuple(TempleRoom::Credits, "Credits Room"),       //
    std::make_tuple(TempleRoom::Main, "Hall"),                  //
    std::make_tuple(TempleRoom::Multi, "Multiplayer Room"),     //
    std::make_tuple(TempleRoom::Options, "Options Room"),       //
    std::make_tuple(TempleRoom::SaveGame, "Save Game Room"),    //
};

void TempleInterior::Draw() noexcept
{
	if (Locator::temple::has_value())
	{
		auto& temple = Locator::temple::value();
		const auto inactive = !temple.Active();

		if (ImGui::Button(inactive ? "Enter temple" : "Exit temple"))
		{
			if (inactive)
			{
				temple.Activate();
			}
			else
			{
				temple.Deactivate();
			}
		}

		for (auto [room, name] : k_Lookup)
		{
			if (ImGui::Button(fmt::format("{}", name).c_str()))
			{
				if (inactive)
				{
					temple.Activate(static_cast<TempleRoom>(room));
				}
				else
				{
					//					temple.templeGoTo();
				}
			}
		}
	}
	else
	{
		ImGui::Text("There is no temple loaded");
	}
}

void TempleInterior::Update() noexcept {}

void TempleInterior::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void TempleInterior::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
