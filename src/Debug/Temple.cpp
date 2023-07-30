/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Temple.h"

#include <unordered_map>

#include "3D/TempleInteriorInterface.h"
#include "Game.h"
#include "Locator.h"

using namespace openblack::debug::gui;

TempleInterior::TempleInterior()
    : Window("Citadel", ImVec2(250.0f, 165.0f))
{
}

const std::unordered_map<openblack::TempleRoom, std::string_view> k_RoomComponentMap = {
    {openblack::TempleRoom::Challenge, "Challenge Room"}, {openblack::TempleRoom::CreatureCave, "Creature Cave"},
    {openblack::TempleRoom::Credits, "Creature Cave"},    {openblack::TempleRoom::Main, "Main Room"},
    {openblack::TempleRoom::Multi, "Multi Room"},         {openblack::TempleRoom::Options, "Option Room"},
    {openblack::TempleRoom::SaveGame, "Save Game Room"}};

void TempleInterior::Draw([[maybe_unused]] openblack::Game& game)
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
		if (!active)
		{
			if (temple.GetCurrentRoom() == openblack::TempleRoom::Main)
			{
				for (const auto& [room, name] : k_RoomComponentMap)
				{
					if (room != openblack::TempleRoom::Main)
					{
						if (ImGui::Button(name.data()))
						{
							temple.ChangeRoom(room);
						}
					}
				}
			}
			else
			{
				auto roomView = k_RoomComponentMap.at(openblack::TempleRoom::Main);
				if (ImGui::Button(roomView.data()))
				{
					temple.ChangeRoom(openblack::TempleRoom::Main);
				}
			}
		}
	}
	else
	{
		ImGui::Text("No temple");
	}
}

void TempleInterior::Update([[maybe_unused]] openblack::Game& game, [[maybe_unused]] const openblack::Renderer& renderer) {}

void TempleInterior::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void TempleInterior::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
