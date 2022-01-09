/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "DebugWindow.h"

#include <memory>
#include <optional>

#include "AllMeshes.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"

#include <imgui.h>

namespace openblack
{

namespace graphics
{
class DebugLines;
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
	static constexpr graphics::RenderPass _viewId = graphics::RenderPass::MeshViewer;
	MeshId _selectedMesh;
	int _selectedSubMesh;
	std::optional<uint32_t> _selectedAnimation;
	int _selectedFrame;
	ImGuiTextFilter _filter;
	uint32_t _meshFlagFilter;
	bool _matchBones;
	glm::vec3 _cameraPosition;
	bool _viewBoundingBox;
	std::unique_ptr<graphics::DebugLines> _boundingBox;
	std::unique_ptr<graphics::FrameBuffer> _frameBuffer;
};

} // namespace gui

} // namespace openblack
