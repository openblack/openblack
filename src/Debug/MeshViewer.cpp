/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "MeshViewer.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <imgui_bitfield.h>
#include <spdlog/spdlog.h>

#include "3D/Camera.h"
#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "Debug/Gui.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Texture2D.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Renderer.h"
#include "Resources/MeshId.h"
#include "Resources/Resources.h"

using namespace openblack;
using namespace openblack::debug::gui;

MeshViewer::MeshViewer()
    : Window("MeshPack Viewer", ImVec2(950.0f, 780.0f))
    , _selectedMesh(resources::MeshIdToResourceId(MeshId::Dummy))
    , _boundingBox(graphics::DebugLines::CreateBox(glm::vec4(1.0f, 0.0f, 0.0f, 0.5f)))
    , _frameBuffer(std::make_unique<graphics::FrameBuffer>("MeshViewer", static_cast<uint16_t>(512), static_cast<uint16_t>(512),
                                                           graphics::Format::RGBA8, graphics::Format::Depth24Stencil8))
{
}

void MeshViewer::Draw([[maybe_unused]] Game& game)
{

	float fontSize = ImGui::GetFontSize();
	auto const& meshes = Locator::resources::value().GetMeshes();
	auto const& animations = Locator::resources::value().GetAnimations();

	_filter.Draw();
	ImGui::InputScalar("Mesh flag filter", ImGuiDataType_U32, &_meshFlagFilter, nullptr, nullptr, "%08X",
	                   ImGuiInputTextFlags_CharsHexadecimal);
	uint32_t hoverIndex;
	ImGuiBitField::BitField("Mesh flag bit-field filter", &_meshFlagFilter, &hoverIndex);
	if (ImGui::IsItemHovered() && hoverIndex < k_L3DMeshFlagNames.size())
	{
		ImGui::SetTooltip("%s", k_L3DMeshFlagNames.at(hoverIndex).data());
	}

	ImGui::BeginChild("meshes", ImVec2(fontSize * 15.0f, 0));
	auto meshSize = ImGui::GetItemRectSize();
	ImGui::BeginChild("meshesSelect", ImVec2(meshSize.x - 5, meshSize.y - ImGui::GetTextLineHeight() - 5), true);
	uint32_t displayedMeshes = 0;

	meshes.Each([this, &displayedMeshes](entt::id_type id, const L3DMesh& mesh) {
		if (_filter.PassFilter(mesh.GetDebugName().c_str()) && (mesh.GetFlags() & _meshFlagFilter) != 0u)
		{
			displayedMeshes++;
			auto flags = mesh.GetNumSubMeshes() > 0 ? 0 : ImGuiSelectableFlags_Disabled;
			if (ImGui::Selectable(mesh.GetDebugName().c_str(), id == _selectedMesh, flags))
			{
				_selectedMesh = id;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", mesh.GetDebugName().c_str());
			}
		}
	});
	ImGui::EndChild();
	ImGui::Text("%u meshes", displayedMeshes);
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("viewer", ImVec2(fontSize * -15.0f, 0));

	ImGui::Columns(2, nullptr, false);

	auto mesh = meshes.Handle(_selectedMesh);

	static std::array<char, 0x400> bitfieldTitle;
	{
		int32_t offset = 0;
		int32_t newLines = 1;
		for (uint32_t flag = 1; const auto& name : k_L3DMeshFlagNames)
		{
			if ((mesh->GetFlags() & flag) != 0)
			{
				auto writen = std::snprintf(bitfieldTitle.data() + offset, bitfieldTitle.size() - offset, "%s%s%s",
				                            offset != 0 ? "|" : "", offset > newLines * 100 ? "\n" : "", name.data());
				while (offset > newLines * 100)
				{
					newLines++;
				}
				offset += writen;
			}
			flag <<= 1;
		}
	}

	std::array<char, 0x20> meshFlagStr;
	std::snprintf(meshFlagStr.data(), meshFlagStr.size(), "Mesh flag=0x%X", mesh->GetFlags());
	if (ImGui::TreeNodeEx(meshFlagStr.data()))
	{
		ImGui::Text("%s", bitfieldTitle.data());
		ImGui::TreePop();
	}

	ImGui::Text("Bones %zu", mesh->GetBoneMatrices().size());

	if (mesh->ContainsExtraMetrics())
	{
		const auto& metrics = mesh->GetExtraMetrics();
		if (!metrics.empty() && ImGui::TreeNode("ExtraMetrics", "Extra Metrics %zu", metrics.size()))
		{
			std::array<char, 0x20> label;
			for (int i = 0; auto& m : const_cast<std::vector<glm::mat4>&>(metrics))
			{
				if (i != 0)
				{
					ImGui::Separator();
				}
				snprintf(label.data(), label.size(), "m[%d][0]", i);
				ImGui::InputFloat4(label.data(), glm::value_ptr(m[0]), "%.3f", ImGuiInputTextFlags_ReadOnly);
				snprintf(label.data(), label.size(), "m[%d][1]", i);
				ImGui::InputFloat4(label.data(), glm::value_ptr(m[1]), "%.3f", ImGuiInputTextFlags_ReadOnly);
				snprintf(label.data(), label.size(), "m[%d][2]", i);
				ImGui::InputFloat4(label.data(), glm::value_ptr(m[2]), "%.3f", ImGuiInputTextFlags_ReadOnly);
				snprintf(label.data(), label.size(), "m[%d][3]", i);
				ImGui::InputFloat4(label.data(), glm::value_ptr(m[3]), "%.3f", ImGuiInputTextFlags_ReadOnly);
				++i;
			}
			ImGui::TreePop();
		}
	}

	ImGui::Text("%zu submeshes", mesh->GetSubMeshes().size());
	ImGui::SliderInt("submesh", &_selectedSubMesh, 0, static_cast<int>(mesh->GetSubMeshes().size()) - 1);
	ImGui::DragFloat3("position", &_cameraPosition[0], 0.5f);
	ImGui::Checkbox("View bounding box", &_viewBoundingBox);
	ImGui::Checkbox("Show matching armature", &_matchBones);
	ImGui::SameLine();

	if (ImGui::Button("Spawn", ImVec2(100, 100)) && _selectedSubMesh >= 0)
	{
		auto& registry = Game::Instance()->GetEntityRegistry();
		auto& camera = Game::Instance()->GetCamera();
		auto entity = registry.Create();
		auto pos = camera.GetPosition();
		auto rot = glm::eulerAngleY(camera.GetRotation().y);
		auto scale = glm::vec3(1.0f, 1.0f, 1.0f);
		registry.Assign<ecs::components::Transform>(entity, pos, rot, scale);
		registry.Assign<ecs::components::Mesh>(entity, _selectedMesh, static_cast<int8_t>(0), static_cast<int8_t>(0));
	}

	auto const& subMeshes = mesh->GetSubMeshes();

	if (_selectedSubMesh >= subMeshes.size())
	{
		_selectedSubMesh = subMeshes.size() - 1;
		SPDLOG_LOGGER_WARN(spdlog::get("game"), "Selected submesh ({}) is out of bounds for the given mesh ({})",
		                   _selectedSubMesh, mesh->GetDebugName());
	}

	if (_selectedSubMesh >= mesh->GetNumSubMeshes())
	{
		_selectedSubMesh = 0;
	}

	auto const& submesh = mesh->GetSubMeshes()[_selectedSubMesh];

	auto flags = submesh->GetFlags();
	std::array<char, 0x20> subMeshFlagStr;
	uint32_t flagsRaw;
	memcpy(&flagsRaw, &flags, sizeof(flagsRaw));
	std::snprintf(subMeshFlagStr.data(), subMeshFlagStr.size(), "SubMesh flag=0x%X", flagsRaw);
	if (ImGui::TreeNodeEx(subMeshFlagStr.data()))
	{
		ImGui::Text("%s%s%s\nLOD Mask=0%s%s%s\nStatus=%u\nunknown1=0x%x\nunknown2=0x%x\nunknown2=0x%x",
		            flags.hasBones ? "[Bones] " : "", flags.isPhysics ? "[Physics] " : "", flags.isWindow ? "[Window] " : "",
		            (flags.lodMask & 0b001) == 0 ? "" : "|1", (flags.lodMask & 0b010) == 0 ? "" : "|2",
		            (flags.lodMask & 0b100) == 0 ? "" : "|4", flags.status, flags.unknown1, flags.unknown2, flags.unknown3);
		ImGui::TreePop();
	}

	auto const& graphicsMesh = submesh->GetMesh();
	ImGui::Text("Vertices %u, Indices %u", graphicsMesh.GetVertexBuffer().GetCount(), graphicsMesh.GetIndexBuffer().GetCount());

	ImGui::NextColumn();

	if (mesh->ContainsLandscapeFeature() && ImGui::TreeNodeEx("Footprint", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::SliderInt("index", &_selectedFootprint, 0, static_cast<int>(mesh->GetFootprints().size() - 1));
		if (_selectedFootprint > static_cast<int>(mesh->GetFootprints().size()))
		{
			_selectedFootprint = static_cast<int>(mesh->GetFootprints().size()) - 1;
		}
		const auto& footprint = mesh->GetFootprints().at(_selectedFootprint);
		ImGui::Image(footprint.texture->GetNativeHandle(), ImVec2(128, 128));
		ImGui::TreePop();
	}

	if (_selectedAnimation)
	{
		auto const& animation = animations.Handle(*_selectedAnimation);
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

	ImGui::Image(_frameBuffer->GetColorAttachment().GetNativeHandle(), ImVec2(512, 512));

	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::BeginChild("animations", ImVec2(fontSize * 15.0f, 0));
	auto animationSize = ImGui::GetItemRectSize();
	ImGui::BeginChild("animationSelect", ImVec2(animationSize.x - 5, animationSize.y - ImGui::GetTextLineHeight() - 5), true);
	uint32_t displayedAnimations = 0;
	if (_matchBones && _selectedAnimation.has_value() &&
	    animations.Handle(*_selectedAnimation)->GetBoneMatrices(0).size() != mesh->GetBoneMatrices().size())
	{
		_selectedAnimation.reset();
	}
	animations.Each([this, &mesh, &displayedAnimations](entt::id_type id, const L3DAnim& animation) {
		if (_filter.PassFilter(animation.GetName().c_str()) &&
		    (!_matchBones || (animation.GetBoneMatrices(0).size() == mesh->GetBoneMatrices().size())))
		{
			displayedAnimations++;
			if (ImGui::Selectable(animation.GetName().c_str(), _selectedAnimation == id))
			{
				_selectedAnimation = id;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", animation.GetName().c_str());
			}
		}
	});
	ImGui::EndChild();
	ImGui::Text("%u animations", displayedAnimations);
	ImGui::EndChild();
}

void MeshViewer::Update([[maybe_unused]] Game& game, const Renderer& renderer)
{
	auto const& meshes = Locator::resources::value().GetMeshes();
	auto const& animations = Locator::resources::value().GetAnimations();
	auto& shaderManager = renderer.GetShaderManager();
	const auto* objectShader = shaderManager.GetShader("Object");
	const auto* debugShader = shaderManager.GetShader("DebugLine");

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
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(k_ViewId), &view, &perspective);

	_frameBuffer->Bind(k_ViewId);
	// TODO(bwrsandman): The setting of viewport and clearing should probably be
	// done in framebuffer bind
	static const uint32_t clearColor = 0x274659ff;
	bgfx::setViewClear(static_cast<bgfx::ViewId>(k_ViewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
	uint16_t width;
	uint16_t height;
	_frameBuffer->GetSize(width, height);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(k_ViewId), 0, 0, width, height);

	// clang-format off
	uint64_t state = 0u
		| BGFX_STATE_WRITE_MASK
		| BGFX_STATE_DEPTH_TEST_LESS
		| BGFX_STATE_CULL_CCW // TODO(bwrsandman): Some meshes wind one way and
		                      //                   some others (i.e. rocks, gate)
		| BGFX_STATE_MSAA;
	// clang-format on

	const auto& mesh = meshes.Handle(_selectedMesh);
	if (_selectedSubMesh >= 0 && static_cast<uint32_t>(_selectedSubMesh) < mesh->GetSubMeshes().size())
	{
		const auto identity = glm::mat4(1.0f);
		Renderer::L3DMeshSubmitDesc desc = {};
		desc.viewId = k_ViewId;
		desc.program = objectShader;
		desc.state = state;
		desc.modelMatrices = &identity;
		desc.matrixCount = 1;
		desc.drawAll = true;
		std::vector<glm::mat4> bones; // In this scope to prevent free before draw
		if (mesh->IsBoned())
		{
			bones = mesh->GetBoneMatrices();
			const std::vector<uint32_t>& boneParents = mesh->GetBoneParents();
			if (_selectedAnimation.has_value() && _matchBones)
			{
				const auto& frames = animations.Handle(*_selectedAnimation)->GetFrames();
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
		renderer.DrawMesh(*mesh, desc, static_cast<uint8_t>(_selectedSubMesh));
		if (_viewBoundingBox)
		{
			auto box = mesh->GetBoundingBox();
			auto model = glm::translate(box.Center()) * glm::scale(box.Size());
			bgfx::setTransform(glm::value_ptr(model));
			_boundingBox->GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES, 0);
			bgfx::submit(static_cast<bgfx::ViewId>(k_ViewId), debugShader->GetRawHandle());
		}
	}
}

void MeshViewer::ProcessEventOpen([[maybe_unused]] const SDL_Event& event) {}

void MeshViewer::ProcessEventAlways([[maybe_unused]] const SDL_Event& event) {}
