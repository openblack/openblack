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

#include <3D/L3DMesh.h>
#include <3D/MeshPack.h>
#include <Game.h>
#include <Renderer.h>
#include <Graphics/ShaderManager.h>
#include <Graphics/Texture2D.h>
#include <MeshViewer.h>
#include <imgui/imgui.h>
#include <glm/gtc/matrix_transform.hpp>

namespace openblack
{
MeshViewer::MeshViewer()
{
	_frameBuffer  = std::make_unique<Graphics::FrameBuffer>(512, 512, GL_RGBA);
	_selectedMesh = Mesh::Dummy;
	_selectedSubMesh = 0;
	_cameraPosition  = glm::vec3(5.0f, 3.0f, 5.0f);
	_open            = false;
}

void MeshViewer::Open()
{
	_open = true;
}

void MeshViewer::DrawWindow()
{
	if (!_open)
		return;

	const ImGuiStyle& style   = ImGui::GetStyle();
	float fontSize            = ImGui::GetFontSize();
	auto const& meshPack      = Game::instance()->GetMeshPack();
	auto const& meshes        = meshPack.GetMeshes();
	const float meshListWidth = fontSize * 15.0f;

	ImGui::SetNextWindowSize(ImVec2(720.0f, 612.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MeshPack Viewer", &_open);

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

	auto const& mesh = meshes[static_cast<int>(_selectedMesh)];
	ImGui::BeginChild("viewer");

	ImGui::Text("%zu submeshes", mesh->GetSubMeshes().size());
	ImGui::InputInt("submesh", &_selectedSubMesh, 1, 1);
	if (_selectedSubMesh < 0)
		_selectedSubMesh = 0;
	if (_selectedSubMesh >= mesh->GetSubMeshes().size())
		_selectedSubMesh = mesh->GetSubMeshes().size() - 1;

	ImGui::DragFloat3("position", &_cameraPosition[0], 0.5f);

	ImGui::Image((void*)(intptr_t)_frameBuffer->GetTexture()->GetNativeHandle(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();

	ImGui::End();
}

void MeshViewer::DrawScene()
{
	if (!_open)
		return;

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glCullFace(GL_BACK);
	glFrontFace(GL_CW);

	auto const& meshPack        = Game::instance()->GetMeshPack();
	auto const& meshes          = meshPack.GetMeshes();
	ShaderProgram* objectShader = Game::instance()->GetRenderer().GetShaderManager().GetShader("SkinnedMesh");

	glm::mat4 perspective = glm::perspective(glm::radians(70.0f), 1.0f, 1.0f, 1024.0f);
	glm::mat4 view        = glm::lookAt(_cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	_frameBuffer->Bind();
	glViewport(0, 0, _frameBuffer->GetWidth(), _frameBuffer->GetHeight());
	glClearColor(39.0f / 255.0f, 70.0f / 255.0f, 89.0f / 255.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	objectShader->Bind();
	objectShader->SetUniformValue("u_viewProjection", perspective * view);
	objectShader->SetUniformValue("u_modelTransform", glm::mat4(1.0f));

	const auto& mesh = meshes[static_cast<int>(_selectedMesh)];
	if (_selectedSubMesh >= 0 && _selectedSubMesh < mesh->GetSubMeshes().size())
		mesh->Draw(*objectShader, _selectedSubMesh);

	_frameBuffer->Unbind();
}

} // namespace openblack
