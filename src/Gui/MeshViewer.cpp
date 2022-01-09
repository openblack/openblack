/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshViewer.h"

#include "3D/AnimationPack.h"
#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/MeshPack.h"
#include "Game.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "Gui/Gui.h"
#include "Renderer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_bitfield.h>

using namespace openblack;
using namespace openblack::gui;

MeshViewer::MeshViewer()
    : DebugWindow("MeshPack Viewer", ImVec2(950.0f, 780.0f))
    , _selectedMesh(MeshId::Dummy)
    , _selectedSubMesh(0)
    , _selectedAnimation(std::nullopt)
    , _selectedFrame(0)
    , _meshFlagFilter(0xFFFFFFFF)
    , _matchBones(true)
    , _cameraPosition(5.0f, 3.0f, 5.0f)
    , _viewBoundingBox(false)
    , _boundingBox(graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f)))
    , _frameBuffer(std::make_unique<graphics::FrameBuffer>("MeshViewer", static_cast<uint16_t>(512), static_cast<uint16_t>(512),
                                                           graphics::Format::RGBA8, graphics::Format::Depth24Stencil8))
{
}

void MeshViewer::Draw(Game& game)
{

	float fontSize = ImGui::GetFontSize();
	auto const& meshPack = game.GetMeshPack();
	auto const& animationPack = game.GetAnimationPack();
	auto const& meshes = meshPack.GetMeshes();
	auto const& animations = animationPack.GetAnimations();

	_filter.Draw();
	ImGui::InputScalar("Mesh flag filter", ImGuiDataType_U32, &_meshFlagFilter, nullptr, nullptr, "%08X",
	                   ImGuiInputTextFlags_CharsHexadecimal);
	uint32_t hoverIndex;
	ImGuiBitField::BitField("Mesh flag bit-field filter", &_meshFlagFilter, &hoverIndex);
	if (ImGui::IsItemHovered() && hoverIndex < L3DMeshFlagNames.size())
		ImGui::SetTooltip("%s", L3DMeshFlagNames[hoverIndex].data());

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
			{
				_selectedMesh = meshEnum;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", enumName.c_str());
			}
		}
	}
	ImGui::EndChild();
	ImGui::Text("%u meshes", displayedMeshes);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("viewer", ImVec2(fontSize * -15.0f, 0));

	ImGui::Columns(2, nullptr, false);

	auto const& mesh = meshes[static_cast<int>(_selectedMesh)];

	static char bitfieldTitle[0x400];
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

	ImGui::Text("Bones %zu", mesh->GetBoneMatrices().size());

	ImGui::Text("%zu submeshes", mesh->GetSubMeshes().size());
	ImGui::SliderInt("submesh", &_selectedSubMesh, 0, static_cast<int>(mesh->GetSubMeshes().size()) - 1);
	ImGui::DragFloat3("position", &_cameraPosition[0], 0.5f);
	ImGui::Checkbox("View bounding box", &_viewBoundingBox);

	auto const& submesh = mesh->GetSubMeshes()[_selectedSubMesh];

	auto flags = submesh->GetFlags();
	ImGui::Text("SubMesh LOD=%u Status=%u%s%s", flags.lod, flags.status, flags.isPhysics ? " [Physics]" : "",
	            flags.isWindow ? " [Window]" : "");

	auto const& graphicsMesh = submesh->GetMesh();
	ImGui::Text("Vertices %u, Indices %u", graphicsMesh.GetVertexBuffer().GetCount(), graphicsMesh.GetIndexBuffer().GetCount());

	ImGui::NextColumn();

	ImGui::Checkbox("Show matching armature", &_matchBones);
	if (_selectedAnimation)
	{
		auto const& animation = animations[*_selectedAnimation];
		ImGui::Text("%zu frames", animation->GetFrames().size());
		ImGui::Text("Duration %u frames", animation->GetDuration());
		ImGui::SliderInt("frame", &_selectedFrame, 0, static_cast<int>(animation->GetFrames().size() - 1));
		if (_selectedFrame > static_cast<int>(animation->GetFrames().size()))
		{
			_selectedFrame = static_cast<int>(animation->GetFrames().size()) - 1;
		}
		ImGui::Text("Time %u, Bones %zu", animation->GetFrames()[_selectedFrame].time,
		            animation->GetFrames()[_selectedFrame].bones.size());
		ImGui::Text("unknown at 0x20 = 0x%08X", animation->_unknown_0x20);
		ImGui::Text("unknowns 0x24-0x34 =\n%.4f %.4f %.4f %.4f %.4f", animation->_unknown_0x24, animation->_unknown_0x28,
		            animation->_unknown_0x2C, animation->_unknown_0x30, animation->_unknown_0x34);
	}
	ImGui::Columns(1);

	ImGui::Image(_frameBuffer->GetColorAttachment().GetNativeHandle(), ImVec2(512, 512), ImVec2(0, 1), ImVec2(1, 0));

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("animations", ImVec2(fontSize * 15.0f, 0));
	auto animationSize = ImGui::GetItemRectSize();
	ImGui::BeginChild("animationSelect", ImVec2(animationSize.x - 5, animationSize.y - ImGui::GetTextLineHeight() - 5), true);
	uint32_t displayedAnimations = 0;
	if (_matchBones && _selectedAnimation.has_value() &&
	    animations[*_selectedAnimation]->GetBoneMatrices(0).size() != mesh->GetBoneMatrices().size())
	{
		_selectedAnimation.reset();
	}
	for (uint32_t i = 0; i < animations.size(); i++)
	{
		if (_filter.PassFilter(animations[i]->GetName().c_str()) &&
		    (!_matchBones || (animations[i]->GetBoneMatrices(0).size() == mesh->GetBoneMatrices().size())))
		{
			displayedAnimations++;
			if (ImGui::Selectable(animations[i]->GetName().c_str(), _selectedAnimation == i))
			{
				_selectedAnimation = i;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", animations[i]->GetName().c_str());
			}
		}
	}
	ImGui::EndChild();
	ImGui::Text("%u animations", displayedAnimations);
	ImGui::EndChild();
}

void MeshViewer::Update(Game& game, const Renderer& renderer)
{
	auto const& meshPack = game.GetMeshPack();
	auto const& meshes = meshPack.GetMeshes();
	auto const& animationPack = game.GetAnimationPack();
	auto const& animations = animationPack.GetAnimations();
	auto& shaderManager = game.GetRenderer().GetShaderManager();
	auto objectShader = shaderManager.GetShader("Object");
	auto debugShader = shaderManager.GetShader("DebugLine");

	// TODO(bwrsandman): use camera class
	glm::mat4 perspective = glm::perspective(glm::radians(70.0f), 1.0f, 1.0f, 1024.0f);
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4305)
#endif
	glm::mat4 view = glm::lookAt(_cameraPosition, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
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
		std::vector<glm::mat4> bones; // In this scope to prevent free before draw
		if (mesh->IsBoned())
		{
			bones = mesh->GetBoneMatrices();
			const std::vector<uint32_t>& boneParents = mesh->GetBoneParents();
			if (_selectedAnimation.value_or(std::numeric_limits<uint32_t>::max()) > static_cast<uint32_t>(animations.size()))
			{
				_selectedAnimation.reset();
			}
			if (_selectedAnimation.has_value() && _matchBones)
			{
				const auto& frames = animations[*_selectedAnimation]->GetFrames();
				for (uint32_t i = 0; i < frames[_selectedFrame].bones.size(); ++i)
				{
					bones[i] = frames[_selectedFrame].bones[i];
					if (boneParents[i] != std::numeric_limits<uint32_t>::max())
					{
						bones[i] = bones[boneParents[i]] * bones[i];
					}
				}
			}
			desc.modelMatrices = bones.data();
			desc.matrixCount = static_cast<uint8_t>(bones.size());
		}
		renderer.DrawMesh(*mesh, meshPack, desc, static_cast<uint8_t>(_selectedSubMesh));
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

void MeshViewer::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void MeshViewer::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
