/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Temple.h"

#include "Game.h"
#include "Locator.h"

using namespace openblack::debug::gui;

TempleInterior::TempleInterior()
    : Window("Citadel", ImVec2(250.0f, 165.0f))
{
}

void TempleInterior::Draw(openblack::Game& game)
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
	//
	//	ImGui::Separator();
	//
	//	if (temple.Active())
	//	{
	//		auto roomCount = temple.GetRoomCount();
	//
	//		if (temple.GetSelectedRoom() == TempleRoom::Main)
	//		{
	//			for (auto i = 0; i < roomCount; i++)
	//			{
	//				auto room = TempleRoom(i);
	//				auto label = "Enter " + TempleInterior::GetRoomName(room);
	//
	//				if (ImGui::Button(label.c_str()))
	//				{
	//					temple.MoveToRoom(room);
	//				}
	//			}
	//		}
	//		else
	//		{
	//			if (ImGui::Button("Go back"))
	//			{
	//				temple.MoveToRoom(TempleRoom::Main);
	//			}
	//		}
	//	}
	//
	//	ImGui::Separator();
	//	ImGui::Text("Information");
	//	auto templePos = temple.GetPosition();
	//	ImGui::Text("Citadel Position: (%.1f,%.1f, %.1f)", templePos.x, templePos.y, templePos.z);
	//	if (temple.GetSelectedRoom() != TempleRoom::Unknown)
	//	{
	//		ImGui::Text("Active Room: ");
	//		ImGui::SameLine();
	//		ImGui::TextColored(ImVec4(.0f, 1.0f, .0f, 1.0), "%s",
	// TempleInterior::GetRoomName(temple.GetSelectedRoom()).c_str());
	//	}
	//	else
	//	{
	//		ImGui::TextColored(ImVec4(1.0f, .0f, .0f, 1.0), "No active room.");
	//	}
	//
	//	ImGui::Separator();
	//	ImGui::Text("Settings");
	//
	//	bool freeMove = temple.IsFreeCameraEnabled();
	//	bool teleport = temple.IsTeleportEnabled();
	//	ImGui::Checkbox("Enable free camera movement", &freeMove);
	//	ImGui::Checkbox("Instant room teleport", &teleport);
	//	temple.SetFreeCamera(freeMove);
	//	temple.SetTeleport(teleport);
}

void TempleInterior::Update([[maybe_unused]] openblack::Game& game, [[maybe_unused]] const openblack::Renderer& renderer) {}

void TempleInterior::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void TempleInterior::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
