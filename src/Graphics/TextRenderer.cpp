/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TextRenderer.h"

#include <bgfx/embedded_shader.h>
#include <bx/math.h>
#include <imgui.h>

#include "Graphics/ShaderManager.h"
#include "Graphics/Texture2D.h"
#include "Renderer.h"

namespace openblack
{

TextRenderer::TextRenderer(bgfx::ViewId viewId, const Renderer& renderer)
    : _vertexBuffer(BGFX_INVALID_HANDLE)
    , _indexBuffer(BGFX_INVALID_HANDLE)
    , _s_tex(BGFX_INVALID_HANDLE)
    , _u_textColor(BGFX_INVALID_HANDLE)
    , _u_textPosition(BGFX_INVALID_HANDLE)
    , _viewId(viewId)
{
	auto& shaderManager = renderer.GetShaderManager();
	_program = shaderManager.GetShader("Text")->GetRawHandle();
	CreateDeviceObjectsBgfx();
}

TextRenderer::~TextRenderer()
{
	if (bgfx::isValid(_vertexBuffer))
		bgfx::destroy(_vertexBuffer);
	if (bgfx::isValid(_indexBuffer))
		bgfx::destroy(_indexBuffer);
	if (bgfx::isValid(_s_tex))
		bgfx::destroy(_s_tex);
	if (bgfx::isValid(_u_textColor))
		bgfx::destroy(_u_textColor);
	if (bgfx::isValid(_u_textPosition))
		bgfx::destroy(_u_textPosition);
	if (bgfx::isValid(_program))
		bgfx::destroy(_program);
}

void TextRenderer::CreateDeviceObjectsBgfx()
{
	bgfx::RendererType::Enum type = bgfx::getRendererType();

	// _u_textColor = bgfx::createUniform("u_textColor", bgfx::UniformType::Vec4);
	// _u_textPosition = bgfx::createUniform("u_textPosition", bgfx::UniformType::Vec4);

	_layout.begin()
	    .add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float)
	    //.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Float)
	    .end();

	_s_tex = bgfx::createUniform("s_tex", bgfx::UniformType::Sampler);
}

void TextRenderer::DrawText(const Font& font, const std::string& text)
{
	_vertices.clear();
	_indices.clear();

	_vertices.resize(text.length() * 4);
	_indices.resize(text.length() * 6);

	float x = 50;
	int y = 800;

	int vert = 0;
	int idx = 0;

	// todo: this doesn't support utf-8
	for (const char& c : text)
	{
		if (c == ' ')
		{
			x += 10;
			continue;
		}

		const Font::Glyph* glyph = font.FindGlyphNoFallback(c);

		// todo: handle missing characters
		if (glyph == nullptr)
			continue;

		const int w = glyph->fWidth;
		const int h = font.GetSize();

		_indices[idx++] = vert;     // TL
		_indices[idx++] = vert + 1; // TR
		_indices[idx++] = vert + 2; // BL
		_indices[idx++] = vert + 1; // TR
		_indices[idx++] = vert + 3; // BR
		_indices[idx++] = vert + 2; // BL

		const float r = 1.0;
		const float g = 0.7;
		const float b = 0.7;

		// x -= glyph->unk2;

		_vertices[vert++] = TextVertex {static_cast<float>(x), static_cast<float>(y), glyph->u0, glyph->v0, r, g, b, 1.0}; // TL
		_vertices[vert++] =
		    TextVertex {static_cast<float>(x + w), static_cast<float>(y), glyph->u1, glyph->v0, r, g, b, 1.0}; // TR
		_vertices[vert++] =
		    TextVertex {static_cast<float>(x), static_cast<float>(y + h), glyph->u0, glyph->v1, r, g, b, 1.0}; // BL
		_vertices[vert++] =
		    TextVertex {static_cast<float>(x + w), static_cast<float>(y + h), glyph->u1, glyph->v1, r, g, b, 1.0}; // BR

		x += glyph->fWidth + glyph->fXoffset; // +glyph->unk2;
	}

	// todo: hold this elsewhere
	auto const& texture = font.GetAtlasTexture();
	_tempFontTex = texture.GetNativeHandle();

	// split utf8 text into codepoints
	// with each codepoint get the glyph info
	// with each glyph info make 4 verts, 6 indices - verts: lx, (ly=0???), uv (3 float)
	// spam it all into a vbo and add to bgfx list or w/e
}

void TextRenderer::Draw()
{
	// cheat and use imgui to get this lmao
	const ImGuiIO& io = ImGui::GetIO();
	const float width = io.DisplaySize.x;
	const float height = io.DisplaySize.y;

	bgfx::setViewMode(_viewId, bgfx::ViewMode::Sequential);

	const bgfx::Caps* caps = bgfx::getCaps();
	{
		float ortho[16];
		bx::mtxOrtho(ortho, 0.0f, width, height, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(_viewId, NULL, ortho);
		bgfx::setViewRect(_viewId, 0, 0, uint16_t(width), uint16_t(height));
	}

	if (!bgfx::isValid(_vertexBuffer))
	{
		if (bgfx::isValid(_vertexBuffer))
		{
			bgfx::destroy(_vertexBuffer);
		}
		_vertexBuffer = bgfx::createDynamicVertexBuffer(_vertices.size(), _layout);
	}
	if (!bgfx::isValid(_indexBuffer))
	{
		if (bgfx::isValid(_indexBuffer))
		{
			bgfx::destroy(_indexBuffer);
		}
		_indexBuffer = bgfx::createDynamicIndexBuffer(_indices.size());
	}

	bgfx::update(_vertexBuffer, 0, bgfx::makeRef(_vertices.data(), _vertices.size() * sizeof(TextVertex)));
	bgfx::update(_indexBuffer, 0, bgfx::makeRef(_indices.data(), _indices.size() * sizeof(uint16_t)));

	bgfx::ProgramHandle program = _program;
	uint64_t state = 0u | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
	state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	bgfx::setState(state);
	bgfx::setTexture(0, _s_tex, _tempFontTex);
	bgfx::setVertexBuffer(0, _vertexBuffer, 0, _vertices.size());
	bgfx::setIndexBuffer(_indexBuffer, 0, _indices.size());
	bgfx::submit(_viewId, program);
}

} // namespace openblack
