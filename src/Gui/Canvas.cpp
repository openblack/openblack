/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Canvas.h"

#include <algorithm>
#include <array>

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <glm/common.hpp>

#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/RenderPass.h"
#include "Graphics/RendererInterface.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/ShaderProgram.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"

using namespace openblack::gui;
using openblack::graphics::RenderPass;

namespace
{
/// Quads are two triangles of their own six vertices. Batches stay well within a transient buffer.
constexpr uint32_t k_MaxBatchVertices = 0x6000;

uint32_t ToAbgr(glm::vec4 colour)
{
	const auto c = glm::clamp(colour, 0.0f, 1.0f) * 255.0f + 0.5f;
	return (static_cast<uint32_t>(c.a) << 24) | (static_cast<uint32_t>(c.b) << 16) | (static_cast<uint32_t>(c.g) << 8) |
	       static_cast<uint32_t>(c.r);
}

bgfx::VertexLayout& GetLayout()
{
	static bgfx::VertexLayout layout = [] {
		bgfx::VertexLayout result;
		result.begin()
		    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		    .end();
		return result;
	}();
	return layout;
}
} // namespace

Canvas::Canvas(RenderPass view)
    : _view(view)
    , _white(std::make_unique<graphics::Texture2D>("InterfaceWhite"))
{
	static constexpr std::array<uint8_t, 4> k_White = {0xFF, 0xFF, 0xFF, 0xFF};
	_white->Create(1, 1, 1, graphics::TextureFormat::RGBA8, graphics::Wrapping::ClampEdge, graphics::Filter::Nearest,
	               bgfx::copy(k_White.data(), static_cast<uint32_t>(k_White.size())));
}

Canvas::~Canvas() = default;

void Canvas::Begin(glm::u16vec2 resolution)
{
	_resolution = resolution;
	_vertices.clear();
	_batches.clear();
	_blend = Blend::Alpha;
}

void Canvas::DrawQuad(glm::vec2 min, glm::vec2 max, glm::vec2 uvMin, glm::vec2 uvMax, glm::vec4 colour,
                      const graphics::Texture2D* texture)
{
	if (colour.a <= 0.0f || min.x == max.x || min.y == max.y)
	{
		return;
	}
	const auto* drawn = texture != nullptr ? texture : _white.get();
	if (_batches.empty() || _batches.back().texture != drawn || _batches.back().blend != _blend ||
	    _batches.back().vertexCount + 6 > k_MaxBatchVertices)
	{
		_batches.push_back(
		    {.texture = drawn, .blend = _blend, .firstVertex = static_cast<uint32_t>(_vertices.size()), .vertexCount = 0});
	}
	const auto abgr = ToAbgr(colour);
	const Vertex topLeft {.x = min.x, .y = min.y, .u = uvMin.x, .v = uvMin.y, .abgr = abgr};
	const Vertex topRight {.x = max.x, .y = min.y, .u = uvMax.x, .v = uvMin.y, .abgr = abgr};
	const Vertex bottomLeft {.x = min.x, .y = max.y, .u = uvMin.x, .v = uvMax.y, .abgr = abgr};
	const Vertex bottomRight {.x = max.x, .y = max.y, .u = uvMax.x, .v = uvMax.y, .abgr = abgr};
	_vertices.insert(_vertices.end(), {topLeft, topRight, bottomRight, topLeft, bottomRight, bottomLeft});
	_batches.back().vertexCount += 6;
}

void Canvas::DrawShape(const std::array<glm::vec2, 4>& corners, const std::array<glm::vec4, 4>& colours)
{
	const auto* white = _white.get();
	if (_batches.empty() || _batches.back().texture != white || _batches.back().blend != _blend ||
	    _batches.back().vertexCount + 6 > k_MaxBatchVertices)
	{
		_batches.push_back(
		    {.texture = white, .blend = _blend, .firstVertex = static_cast<uint32_t>(_vertices.size()), .vertexCount = 0});
	}
	std::array<Vertex, 4> vertices {};
	for (size_t i = 0; i < 4; ++i)
	{
		vertices.at(i) = {.x = corners.at(i).x, .y = corners.at(i).y, .u = 0.5f, .v = 0.5f, .abgr = ToAbgr(colours.at(i))};
	}
	_vertices.insert(_vertices.end(), {vertices[0], vertices[1], vertices[2], vertices[0], vertices[2], vertices[3]});
	_batches.back().vertexCount += 6;
}

void Canvas::DrawLine(glm::ivec2 from, glm::ivec2 to, glm::vec4 colour)
{
	const auto min = glm::min(from, to);
	const auto max = glm::max(from, to) + 1;
	DrawQuad(min, max, {0.0f, 0.0f}, {1.0f, 1.0f}, colour, nullptr);
}

void Canvas::End()
{
	const auto view = static_cast<bgfx::ViewId>(_view);
	bgfx::setViewMode(view, bgfx::ViewMode::Sequential);
	bgfx::setViewClear(view, BGFX_CLEAR_NONE);
	bgfx::setViewRect(view, 0, 0, _resolution.x, _resolution.y);
	std::array<float, 16> projection {};
	bx::mtxOrtho(projection.data(), 0.0f, static_cast<float>(_resolution.x), static_cast<float>(_resolution.y), 0.0f, 0.0f,
	             1.0f, 0.0f, bgfx::getCaps()->homogeneousDepth);
	bgfx::setViewTransform(view, nullptr, projection.data());
	bgfx::touch(view);
	if (_vertices.empty())
	{
		return;
	}

	const auto* program = Locator::rendererInterface::value().GetShaderManager().GetShader("Interface");
	auto& layout = GetLayout();
	for (const auto& batch : _batches)
	{
		if (bgfx::getAvailTransientVertexBuffer(batch.vertexCount, layout) < batch.vertexCount)
		{
			break;
		}
		bgfx::TransientVertexBuffer vertices;
		bgfx::allocTransientVertexBuffer(&vertices, batch.vertexCount, layout);
		std::copy_n(_vertices.begin() + batch.firstVertex, batch.vertexCount, reinterpret_cast<Vertex*>(vertices.data));
		bgfx::setVertexBuffer(0, &vertices);
		program->SetTextureSampler("s_texture", 0, *batch.texture);
		const auto blend = batch.blend == Blend::Additive
		                       ? BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE,
		                                                        BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE)
		                       : BGFX_STATE_BLEND_FUNC_SEPARATE(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA,
		                                                        BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | blend);
		bgfx::submit(view, graphics::toBgfx(program->GetRawHandle()));
	}
}
