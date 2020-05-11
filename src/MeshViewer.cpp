/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshViewer.h"

#include "3D/L3DMesh.h"
#include "3D/MeshPack.h"
#include "Game.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "Gui.h"
#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_bitfield.h>

using namespace openblack;

MeshViewer::MeshViewer()
    : _cameraPosition(5.0f, 3.0f, 5.0f)
    , _boundingBox(graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f)))
    , _frameBuffer(std::make_unique<graphics::FrameBuffer>("MeshViewer", 512, 512, graphics::Format::RGBA8,
                                                           graphics::Format::Depth24Stencil8))
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

	const ImGuiStyle& style = ImGui::GetStyle();
	float fontSize = ImGui::GetFontSize();
	auto const& meshPack = Game::instance()->GetMeshPack();
	auto const& meshes = meshPack.GetMeshes();
	const float meshListWidth = fontSize * 15.0f;

	ImGui::SetNextWindowSize(ImVec2(720.0f, 612.0f), ImGuiCond_FirstUseEver);
	ImGui::Begin("MeshPack Viewer", &_open);

	_filter.Draw();
	ImGui::InputScalar("Mesh flag filter", ImGuiDataType_U32, &_meshFlagFilter, nullptr, nullptr, "%08X",
	                   ImGuiInputTextFlags_CharsHexadecimal);
	uint32_t hoverIndex;
	static char bitfieldTitle[0x400];
	{
		int32_t offset = 0;
		int32_t newLines = 1;
		for (uint8_t i = 0; i < 32; ++i)
		{
			if (_meshFlagFilter & (1u << i))
			{
				auto writen = std::sprintf(bitfieldTitle + offset, "%s%s%s", offset ? "|" : "",
				                           offset > newLines * 100 ? "\n" : "", L3DMeshFlagNames[i].data());
				while (offset > newLines * 100) newLines++;
				offset += writen;
			}
		}
		if (offset == 0)
		{
			bitfieldTitle[0] = '\0';
		}
	}
	ImGuiBitField::BitField("Mesh flag bit-field filter", &_meshFlagFilter, &hoverIndex);
	if (ImGui::IsItemHovered() && hoverIndex < L3DMeshFlagNames.size())
		ImGui::SetTooltip("%s", L3DMeshFlagNames[hoverIndex].data());
	if (bitfieldTitle[0])
		ImGui::Text("%s", bitfieldTitle);

	ImGui::BeginChild("meshes", ImVec2(fontSize * 15.0f, 0));
	auto meshSize = ImGui::GetItemRectSize();
	ImGui::BeginChild("meshesSelect", ImVec2(meshSize.x - 5, meshSize.y - ImGui::GetTextLineHeight() - 5), true);
	uint32_t displayedMeshes = 0;
	for (size_t i = 0; i < meshes.size(); i++)
	{
		if (_filter.PassFilter(MeshNames[i].data()) && meshes[i]->GetFlags() & _meshFlagFilter)
		{
			const auto meshEnum = static_cast<MeshId>(i);
			const auto& enumName = std::string(MeshNames[i]);
			displayedMeshes++;

			if (ImGui::Selectable(enumName.c_str(), static_cast<MeshId>(meshEnum) == _selectedMesh))
				_selectedMesh = meshEnum;
		}
	}
	ImGui::EndChild();
	ImGui::Text("%u meshes", displayedMeshes);
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
				auto writen = std::sprintf(bitfieldTitle + offset, "%s%s%s", offset ? "|" : "",
				                           offset > newLines * 100 ? "\n" : "", L3DMeshFlagNames[i].data());
				while (offset > newLines * 100) newLines++;
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
	ImGui::Text("SubMesh LOD=%u Status=%u%s%s", flags.lod, flags.status, flags.isPhysics ? " [Physics]" : "",
	            flags.isWindow ? " [Window]" : "");

	auto const& graphicsMesh = submesh->GetMesh();
	ImGui::Text("Vertices %u, Indices %u", graphicsMesh.GetVertexBuffer().GetCount(), graphicsMesh.GetIndexBuffer().GetCount());

	ImGui::Image(_frameBuffer->GetColorAttachment().GetNativeHandle(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();

	ImGui::End();
}

void MeshViewer::DrawScene(const Renderer& renderer)
{
	if (!_open)
		return;

	auto const& meshPack = Game::instance()->GetMeshPack();
	auto const& meshes = meshPack.GetMeshes();
	auto& shaderManager = Game::instance()->GetRenderer().GetShaderManager();
	auto objectShader = shaderManager.GetShader("Object");
	auto debugShader = shaderManager.GetShader("DebugLine");

	// TODO(bwrsandman): use camera class
	glm::mat4 perspective = glm::perspective(glm::radians(70.0f), 1.0f, 1.0f, 1024.0f);
	glm::mat4 view = glm::lookAt(_cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(_viewId), &view, &perspective);

	_frameBuffer->Bind(_viewId);
	// TODO(bwrsandman): The setting of viewport and clearing should probably be
	// done in framebuffer bind
	static const uint32_t clearColor = 0x274659ff;
	bgfx::setViewClear(static_cast<bgfx::ViewId>(_viewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
	uint16_t width, height;
	_frameBuffer->GetSize(width, height);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(_viewId), 0, 0, width, height);

	// clang-format off
	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW // TODO(bwrsandman): Some meshes wind one way and
		                      //                   some others (i.e. rocks, gate)
		| BGFX_STATE_MSAA;
	// clang-format on

	const auto& mesh = meshes[static_cast<int>(_selectedMesh)];
	if (_selectedSubMesh >= 0 && static_cast<uint32_t>(_selectedSubMesh) < mesh->GetSubMeshes().size())
	{
		const auto identity = glm::mat4(1.0f);
		Renderer::L3DMeshSubmitDesc desc = {};
		desc.viewId = _viewId;
		desc.program = objectShader;
		desc.state = state;
		desc.modelMatrices = &identity;
		desc.matrixCount = 1;
		if (mesh->IsBoned())
		{
			desc.modelMatrices = mesh->GetBoneMatrices().data();
			desc.matrixCount = mesh->GetBoneMatrices().size();
		}
		renderer.DrawMesh(*mesh, meshPack, desc, _selectedSubMesh);
		if (_viewBoundingBox)
		{
			auto box = mesh->GetSubMeshes()[_selectedSubMesh]->GetBoundingBox();
			auto model = glm::translate(box.center()) * glm::scale(box.size());
			bgfx::setTransform(glm::value_ptr(model));
			_boundingBox->GetMesh().GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES, 0);
			bgfx::submit(static_cast<bgfx::ViewId>(_viewId), debugShader->GetRawHandle());
		}
	}
}
