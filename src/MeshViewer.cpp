/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <imgui.h>
#include <glm/gtc/matrix_transform.hpp>

#include <3D/L3DMesh.h>
#include <3D/MeshPack.h>
#include <Game.h>
#include <Gui.h>
#include <Renderer.h>
#include <Graphics/IndexBuffer.h>
#include <Graphics/ShaderManager.h>
#include <Graphics/Texture2D.h>
#include <Graphics/VertexBuffer.h>
#include <MeshViewer.h>
#include <imgui_bitfield.h>

using namespace openblack;

MeshViewer::MeshViewer()
	: _open(false)
	, _selectedMesh(MeshId::Dummy)
	, _selectedSubMesh(0)
	, _meshFlagFilter(-1)
	, _cameraPosition(5.0f, 3.0f, 5.0f)
	, _viewBoundingBox(false)
	, _boundingBox(graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f)))
	, _frameBuffer(std::make_unique<graphics::FrameBuffer>("MeshViewer", 512, 512, graphics::Format::RGBA8, graphics::Format::Depth24Stencil8))
{
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

	_filter.Draw();
	ImGui::InputScalar("Mesh flag filter", ImGuiDataType_U32, &_meshFlagFilter, nullptr, nullptr, "%08X", ImGuiInputTextFlags_CharsHexadecimal);
	uint32_t hoverIndex;
	static char bitfieldTitle[0x400];
	{
		int32_t offset = 0;
		int32_t newLines = 1;
		for (uint8_t i = 0; i < 32; ++i)
		{
			if (_meshFlagFilter & (1u << i))
			{
				auto writen = std::sprintf(bitfieldTitle + offset, "%s%s%s", offset ? "|" : "", offset > newLines*100 ? "\n" : "", L3DMeshFlagNames[i].data());
				while (offset > newLines*100)
					newLines++;
				offset += writen;
			}
		}
	}
	ImGuiBitField::BitField("Mesh flag bit-field filter", &_meshFlagFilter, &hoverIndex);
	if (ImGui::IsItemHovered() && hoverIndex < L3DMeshFlagNames.size())
		ImGui::SetTooltip("%s", L3DMeshFlagNames[hoverIndex].data());
	ImGui::Text("%s", bitfieldTitle);

	ImGui::BeginChild("meshes", ImVec2(fontSize * 15.0f, 0), true);
	for (size_t i = 0; i < meshes.size(); i++)
	{
		if (_filter.PassFilter(MeshNames[i].data()) && meshes[i]->GetFlags() & _meshFlagFilter)
		{
			const auto meshEnum = static_cast<MeshId>(i);
			const auto &enumName = std::string(MeshNames[i]);

			if (ImGui::Selectable(enumName.c_str(), static_cast<MeshId>(meshEnum)==_selectedMesh))
				_selectedMesh = meshEnum;
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	auto const& mesh = meshes[static_cast<int>(_selectedMesh)];
	ImGui::BeginChild("viewer");

	ImGui::Text("%zu submeshes", mesh->GetSubMeshes().size());
	ImGui::SliderInt("submesh", &_selectedSubMesh, 0, mesh->GetSubMeshes().size() - 1);
	ImGui::DragFloat3("position", &_cameraPosition[0], 0.5f);
	ImGui::Checkbox("View bounding box", &_viewBoundingBox);

	{
		int32_t offset = 0;
		int32_t newLines = 1;
		for (uint8_t i = 0; i < 32; ++i)
		{
			if (mesh->GetFlags() & (1u << i))
			{
				auto writen = std::sprintf(bitfieldTitle + offset, "%s%s%s", offset ? "|" : "", offset > newLines * 100 ? "\n" : "", L3DMeshFlagNames[i].data());
				while (offset > newLines * 100)
					newLines++;
				offset += writen;
			}
		}
	}
	char meshFlagStr[0x20];
	std::sprintf(meshFlagStr, "Mesh flag=0x%X", mesh->GetFlags());
	if (ImGui::TreeNodeEx(meshFlagStr))
	{
		ImGui::Text("%s", bitfieldTitle);
		ImGui::TreePop();
	}


	auto const& submesh = mesh->GetSubMeshes()[_selectedSubMesh];

	auto flags = submesh->GetFlags();
	ImGui::Text("SubMesh LOD=%u Status=%u%s%s",
	            flags.lod,
	            flags.status,
	            flags.isPhysics ? " [Physics]" : "",
	            flags.isWindow ? " [Window]" : "");

	auto const& graphicsMesh = submesh->GetMesh();
	ImGui::Text("Vertices %u, Indices %u", graphicsMesh.GetVertexBuffer().GetCount(), graphicsMesh.GetIndexBuffer().GetCount());

	ImGui::Image(_frameBuffer->GetColorAttachment().GetNativeHandle(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();

	ImGui::End();
}

void MeshViewer::DrawScene()
{
	if (!_open)
		return;

	auto const& meshPack        = Game::instance()->GetMeshPack();
	auto const& meshes          = meshPack.GetMeshes();
	auto& shaderManager         = Game::instance()->GetRenderer().GetShaderManager();
	auto objectShader = shaderManager.GetShader("Object");
	auto debugShader  = shaderManager.GetShader("DebugLine");

	// TODO(bwrsandman): use camera class
	glm::mat4 perspective = glm::perspective(glm::radians(70.0f), 1.0f, 1.0f, 1024.0f);
	glm::mat4 view        = glm::lookAt(_cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(_viewId), &view, &perspective);

	_frameBuffer->Bind(_viewId);
	// TODO(bwrsandman): The setting of viewport and clearing should probably be done in framebuffer bind
	static const uint32_t clearColor = 0x274659ff;
	bgfx::setViewClear(static_cast<bgfx::ViewId>(_viewId),
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		clearColor,
		1.0f,
		0);
	uint16_t width, height;
	_frameBuffer->GetSize(width, height);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(_viewId), 0, 0, width, height);

	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW  // TODO(bwrsandman): Some meshes wind one way and some others (i.e. rocks, gate)
		| BGFX_STATE_MSAA
	;

	const auto& mesh = meshes[static_cast<int>(_selectedMesh)];
	if (_selectedSubMesh >= 0 && static_cast<uint32_t>(_selectedSubMesh) < mesh->GetSubMeshes().size())
	{
		mesh->Draw(_viewId, glm::mat4(1.0f), *objectShader, _selectedSubMesh, state);
		if (_viewBoundingBox)
		{
			auto box = mesh->GetSubMeshes()[_selectedSubMesh]->GetBoundingBox();
			_boundingBox->SetPose(box.center(), box.size());
			_boundingBox->Draw(_viewId, *debugShader);
		}
	}
}
