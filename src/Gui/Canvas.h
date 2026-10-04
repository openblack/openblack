/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <memory>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Graphics/RenderPass.h"

namespace openblack::graphics
{
class Texture2D;
}

namespace openblack::gui
{

/// Draws the game's interface over the scene: textured, tinted rectangles in screen pixels, batched into as few draw
/// calls as the textures allow and submitted to a view, RenderPass::Interface unless another is given.
class Canvas
{
public:
	explicit Canvas(graphics::RenderPass view = graphics::RenderPass::Interface);
	~Canvas();
	Canvas(const Canvas&) = delete;
	Canvas& operator=(const Canvas&) = delete;

	/// Starts a frame of the interface for a window of a size
	void Begin(glm::u16vec2 resolution);
	/// A rectangle of a texture, tinted by colour (red, green, blue and alpha from 0 to 1). Without a texture it is
	/// filled with the colour. Corners whose minimum is beyond the maximum flip the texture.
	void DrawQuad(glm::vec2 min, glm::vec2 max, glm::vec2 uvMin, glm::vec2 uvMax, glm::vec4 colour,
	              const graphics::Texture2D* texture);
	/// A filled four sided shape, its corners in order round it, each with its own colour
	void DrawShape(const std::array<glm::vec2, 4>& corners, const std::array<glm::vec4, 4>& colours);
	/// A one pixel wide line between the centres of two pixels, along a row or a column
	void DrawLine(glm::ivec2 from, glm::ivec2 to, glm::vec4 colour);
	/// Submits the frame's interface
	void End();

	[[nodiscard]] glm::u16vec2 GetResolution() const noexcept { return _resolution; }

private:
	struct Vertex
	{
		float x;
		float y;
		float u;
		float v;
		uint32_t abgr;
	};
	struct Batch
	{
		const graphics::Texture2D* texture;
		uint32_t firstVertex;
		uint32_t vertexCount;
	};

	graphics::RenderPass _view;
	glm::u16vec2 _resolution {0, 0};
	std::vector<Vertex> _vertices;
	std::vector<Batch> _batches;
	std::unique_ptr<graphics::Texture2D> _white;
};

} // namespace openblack::gui
