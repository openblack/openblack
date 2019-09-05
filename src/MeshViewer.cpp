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
#include <imgui/imgui.h>

namespace OpenBlack
{
MeshViewer::MeshViewer()
{
	_frameBuffer = std::make_unique<Graphics::FrameBuffer>(512, 512, GL_RGBA);
	_selectedMesh = Mesh::Dummy;
}

void MeshViewer::Open()
{
	_open = true;
}

void MeshViewer::DrawWindow()
{
	if (!_open)
		return;

	const ImGuiStyle& style = ImGui::GetStyle();
	float fontSize = ImGui::GetFontSize();
	auto const& meshPack = Game::instance()->GetMeshPack();

	ImGui::SetNextWindowSize(ImVec2(720.0f, 612.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MeshPack Viewer", &_open);

	auto const& meshes = meshPack.GetMeshes();
	ImGui::BeginChild("meshes", ImVec2(fontSize * 15.0f, 0), true);
	for (int i = 0; i < meshes.size(); i++)
	{
		Mesh meshEnum = static_cast<Mesh>(i);
		auto enumName = std::string(magic_enum::enum_name(meshEnum));

		if (ImGui::Selectable(enumName.c_str(), meshEnum == _selectedMesh))
			_selectedMesh = meshEnum;
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("viewer");

	ImGui::Text("Selected Mesh: %d", static_cast<int>(_selectedMesh));
	ImGui::Image((void*)(intptr_t)_frameBuffer->GetTexture()->GetNativeHandle(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();

	/*
	int i = 0;
	for (const auto& tex : meshPack.GetTextures())
	{
		ImGui::Image((ImTextureID)tex->GetNativeHandle(), ImVec2(128, 128));
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("ID: %d", i);

		if (++i % 4 != 0)
			ImGui::SameLine();
	}
	*/

	ImGui::End();
}

void MeshViewer::DrawScene()
{
	_frameBuffer->Bind();
	glViewport(0, 0, _frameBuffer->GetWidth(), _frameBuffer->GetHeight());
	glClearColor(39.0f / 255.0f, 70.0f / 255.0f, 89.0f / 255.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	_frameBuffer->Unbind();
}

} // namespace OpenBlack
