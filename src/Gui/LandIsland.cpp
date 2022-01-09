/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LandIsland.h"

#include <LNDFile.h>

#include <3D/LandIsland.h>
#include <Game.h>

openblack::gui::LandIsland::LandIsland()
    : DebugWindow("Land Island", ImVec2(250.0f, 165.0f))
{
}

void openblack::gui::LandIsland::Draw(openblack::Game& game)
{
	auto& config = game.GetConfig();

	const auto& landIsland = game.GetLandIsland();

	ImGui::SliderFloat("Bump", &config.bumpMapStrength, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Small Bump", &config.smallBumpMapStrength, 0.0f, 1.0f, "%.3f");

	ImGui::Separator();

	ImGui::Text("Block Count: %zu", landIsland.GetBlocks().size());
	ImGui::Text("Country Count: %zu", landIsland.GetCountries().size());

	ImGui::Separator();

	if (ImGui::Button("Dump Textures"))
	{
		landIsland.DumpTextures();
	}

	if (ImGui::Button("Dump Heightmap"))
	{
		landIsland.DumpMaps();
	}
}

void openblack::gui::LandIsland::Update([[maybe_unused]] openblack::Game& game,
                                        [[maybe_unused]] const openblack::Renderer& renderer)
{
}

void openblack::gui::LandIsland::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void openblack::gui::LandIsland::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
