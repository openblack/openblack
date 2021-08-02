/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <memory>
#include <optional>

#include "Common/MeshLookup.h"
#include "AllMeshes.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"

#include <imgui.h>

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
	bool _open;
	static constexpr graphics::RenderPass _viewId = graphics::RenderPass::MeshViewer;
	MeshId _selectedMesh;
	int _selectedSubMesh;
	std::optional<uint32_t> _selectedAnimation;
	int _selectedFrame;
	ImGuiTextFilter _filter;
	uint32_t _meshFlagFilter;
	uint32_t _meshFlags;
	bool _matchBones;
	glm::vec3 _cameraPosition;
	bool _viewBoundingBox;
	std::unique_ptr<graphics::DebugLines> _boundingBox;
	std::unique_ptr<graphics::FrameBuffer> _frameBuffer;
};
} // namespace openblack
