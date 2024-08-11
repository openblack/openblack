/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>
#include <optional>

#include <entt/core/fwd.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

#include "3D/AllMeshes.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"
#include "Window.h"

namespace openblack
{

namespace graphics
{
class Mesh;
}

namespace debug::gui
{
class MeshViewer: public Window
{
public:
	MeshViewer();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const graphics::Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	static constexpr graphics::RenderPass k_ViewId = graphics::RenderPass::MeshViewer;
	entt::id_type _selectedMesh;
	int _selectedSubMesh {0};
	std::optional<entt::id_type> _selectedAnimation;
	int _selectedFootprint {0};
	int _selectedFrame {0};
	ImGuiTextFilter _filter;
	uint32_t _meshFlagFilter {0xFFFFFFFF};
	bool _matchBones {true};
	glm::vec3 _cameraPosition {5.0f, 3.0f, 5.0f};
	glm::vec3 _handPosition {0.0f, 0.0f, 0.0f};
	glm::vec3 _spawnLocation {0.0f, 0.0f, 0.0f};
	float _spawnRotation {0.0f};
	float _spawnScale {1.0f};
	bool _viewBoundingBox {false};
	std::unique_ptr<graphics::Mesh> _boundingBox;
	std::unique_ptr<graphics::FrameBuffer> _frameBuffer;
};

} // namespace debug::gui

} // namespace openblack
