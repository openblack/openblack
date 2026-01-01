/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LandIsland.h"

#include <LNDFile.h>

#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "Debug/ImGuiUtils.h"
#include "EngineConfig.h"
#include "Graphics/FrameBuffer.h"
#include "Locator.h"

using namespace openblack::debug::gui;

LandIsland::LandIsland() noexcept
    : Window("Land Island", ImVec2(600.0f, 600.0f))
{
}

void LandIsland::Draw() noexcept
{
	auto& config = Locator::config::value();

	const auto& landIsland = Locator::terrainSystem::value();

	ImGui::SliderFloat("Bump", &config.bumpMapStrength, 0.0f, 1.0f, "%.3f");
	ImGui::SliderFloat("Small Bump", &config.smallBumpMapStrength, 0.0f, 1.0f, "%.3f");

	ImGui::Separator();

	ImGui::Text("Block Count: %zu", landIsland.GetBlocks().size());
	ImGui::Text("Country Count: %zu", landIsland.GetCountries().size());

	ImGui::Separator();

	if (ImGui::TreeNodeEx("Height Map", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto indexExtent = landIsland.GetIndexExtent();
		const auto extentSize = indexExtent.maximum - indexExtent.minimum;
		const auto dim = static_cast<uint16_t>(LandIslandInterface::k_CellCount) * extentSize;
		const auto& texture = landIsland.GetHeightMap();
		ImGui::Text("Resolution: %ux%u", dim.x, dim.y);
		const float scaling = 512.0f / static_cast<float>(dim.x);
		ImGui::Image(texture.GetNativeHandle(), ImVec2(dim.x * scaling, dim.y * scaling));
		ImGui::TreePop();
	}

	if (ImGui::TreeNodeEx("Footprints", ImGuiTreeNodeFlags_DefaultOpen))
	{
		const auto& frameBuffer = landIsland.GetFootprintFramebuffer();
		uint16_t width;
		uint16_t height;
		frameBuffer.GetSize(width, height);
		ImGui::Text("Resolution: %ux%u", width, height);
		const float scaling = 512.0f / static_cast<float>(width);
		ImGui::Image(frameBuffer.GetColorAttachment().GetNativeHandle(), ImVec2(width * scaling, height * scaling));
		ImGui::TreePop();
	}

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

void LandIsland::Update() noexcept {}

void LandIsland::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) noexcept {}

void LandIsland::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) noexcept {}
