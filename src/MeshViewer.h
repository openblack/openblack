/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "AllMeshes.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"

#include <imgui.h>

#include <memory>

namespace openblack
{
class Renderer;

namespace graphics
{
class DebugLines;
}

class MeshViewer
{
public:
	explicit MeshViewer();
	void Open();
	void DrawWindow();
	void DrawScene(const Renderer& renderer);

private:
	bool _open {false};
	static constexpr graphics::RenderPass _viewId = graphics::RenderPass::MeshViewer;
	MeshId _selectedMesh {MeshId::Dummy};
	int _selectedSubMesh {0};
	ImGuiTextFilter _filter;
	uint32_t _meshFlagFilter {0xFFFFFFFF};
	glm::vec3 _cameraPosition;
	bool _viewBoundingBox {false};
	std::unique_ptr<graphics::DebugLines> _boundingBox;
	std::unique_ptr<graphics::FrameBuffer> _frameBuffer;
};
} // namespace openblack
