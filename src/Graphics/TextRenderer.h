/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>

#include "Graphics/Font/Font.h"

namespace openblack
{
class Renderer;

// data\\j0
// data\\f1
// data\\fneutral
// data\\f3
enum class TextType
{
	Good,
	Evil,
	Neutral
};

class TextRenderer
{
public:
	TextRenderer(bgfx::ViewId viewId, const Renderer& renderer);
	~TextRenderer();

	void CreateDeviceObjectsBgfx();

	void DrawText(const Font& font, const std::string& text);
	void Draw();

private:
	struct TextVertex
	{
		float x;
		float y;
		float u;
		float v;
		float r;
		float g;
		float b;
		float a;
	};

	std::vector<TextVertex> _vertices;
	std::vector<uint16_t> _indices;

	bgfx::TextureHandle _tempFontTex;

	bgfx::DynamicVertexBufferHandle _vertexBuffer;
	bgfx::DynamicIndexBufferHandle _indexBuffer;
	uint32_t _vertexCount;
	uint32_t _indexCount;
	bgfx::VertexLayout _layout;
	bgfx::ProgramHandle _program;
	bgfx::UniformHandle _s_tex;
	bgfx::UniformHandle _u_textColor;
	bgfx::UniformHandle _u_textPosition;
	const bgfx::ViewId _viewId;
};
} // namespace openblack
