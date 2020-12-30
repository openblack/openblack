/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "GUILandIsland.h"

#include <3D/LandIsland.h>
#include <Gui.h>

#include <imgui.h>

namespace openblack
{

void GUILandIsland::Draw(LandIsland& island)
{
	ImGui::SetNextWindowSize(ImVec2(720.0f, 612.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("Land Island");

	ImGui::BeginGroupPanel("Bump Map");
	ImGui::Image(island._textureBumpMap->GetNativeHandle(), ImVec2(128, 128));
	ImGui::EndGroupPanel();
	ImGui::SameLine();
	ImGui::BeginGroupPanel("Noise Map");
	ImGui::Image(island._textureNoiseMap->GetNativeHandle(), ImVec2(128, 128));
	ImGui::EndGroupPanel();
	ImGui::BeginGroupPanel("Country Map");
	ImGui::Image(island._textureCountryMap->GetNativeHandle(), ImVec2(256, 256));
	ImGui::EndGroupPanel();
	ImGui::SameLine();
	ImGui::BeginGroupPanel("Country Test");
	ImGui::Image(island._textureCountryTest->GetNativeHandle(), ImVec2(256, 256));
	ImGui::EndGroupPanel();


	ImGui::BeginGroupPanel("\xEF\xA2\xAD Statistics");
	ImGui::Text("Block Count: %zu", island.GetBlocks().size());
	ImGui::Text("Country Count: %zu", island.GetCountries().size());
	ImGui::EndGroupPanel();

	// Stuff to display:
	// - Heightmap
	// - Countrymap
	// - Lightmap

	// - Bumpmap
	// - Noisemap

	// - Country style
	//   - Can display overall country textures as altitude = 2D Y
	//   - Material # blends with Material #
	//   - Blending Factor
	//   - Coref
	//   - Resulting texture
	//   - Altitude Slider (along Y axis)

	ImGui::End();
}

}
