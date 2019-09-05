/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <MeshViewer.h>
#include <Game.h>
#include <3D/MeshPack.h>
#include <Graphics/Texture2D.h>

namespace OpenBlack
{
void MeshViewer::Open()
{
	_open = true;
}

void MeshViewer::DrawWindow()
{
	if (!_open)
		return;

	auto const& meshPack = Game::instance()->GetMeshPack();

	ImGui::SetNextWindowSize(ImVec2(720.0f, 612.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MeshPack Viewer", &_open);

	ImGui::Text("%d meshes, %d textures", meshPack.GetMeshes().size(), meshPack.GetTextures().size());

	if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Textures"))
		{
			int i = 0;
			for (const auto& tex : meshPack.GetTextures())
			{
				ImGui::Image((ImTextureID)tex->GetNativeHandle(), ImVec2(128, 128));
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("ID: %d", i);

				if (++i % 4 != 0)
					ImGui::SameLine();
			}

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}
} // namespace OpenBlack
