/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <memory>
#include <optional>

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

#include "3D/AllMeshes.h"
#include "DebugWindow.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"

namespace openblack
{

namespace graphics
{
class Mesh;
}

namespace gui
{
class MeshViewer: public DebugWindow
{
public:
	MeshViewer();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	static constexpr graphics::RenderPass k_ViewId = graphics::RenderPass::MeshViewer;
	entt::id_type _selectedMesh;
	int _selectedSubMesh {0};
	std::optional<entt::id_type> _selectedAnimation;
	int _selectedFrame {0};
	ImGuiTextFilter _filter;
	uint32_t _meshFlagFilter {0xFFFFFFFF};
	bool _matchBones {true};
	glm::vec3 _cameraPosition {5.0f, 3.0f, 5.0f};
	bool _viewBoundingBox {false};
	std::unique_ptr<graphics::Mesh> _boundingBox;
	std::unique_ptr<graphics::FrameBuffer> _frameBuffer;
};

} // namespace gui

} // namespace openblack
