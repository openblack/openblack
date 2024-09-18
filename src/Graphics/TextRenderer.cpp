/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define STB_RECT_PACK_IMPLEMENTATION

#include "TextRenderer.h"

#include <algorithm>
#include <bit>

#include <bx/bx.h>
#include <bx/math.h>
#include <spdlog/spdlog.h>
#include <stb_rect_pack.h>

#include "FileSystem/FileSystemInterface.h"
#include "Graphics/RenderPass.h"
#include "Graphics/RendererInterface.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Texture2D.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

namespace openblack
{

TextRenderer::TextRenderer()
{
	LoadFonts();

	_vertexLayout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
}

TextRenderer::~TextRenderer() = default;

TextRenderer::Glyph TextRenderer::GetGlyph(Font fontID, char16_t codepoint) const
{
	auto it = _glyphs.find(std::make_tuple(fontID, codepoint));

	if (it != _glyphs.end())
	{
		return it->second;
	}

	// todo: return that box thing
	return {};
}

void TextRenderer::LoadFonts()
{
	auto& fileSystem = Locator::filesystem::value();

	std::vector<uint8_t*> fontBitmapData;
	std::vector<stbrp_rect> rects;

	const std::array<std::string_view, 3> k_FontPaths {"j0", "f1", "f3"};
	for (int i = 0; i < static_cast<int>(Font::_COUNT); i++)
	{
		LoadFont(static_cast<Font>(i), fileSystem.GetPath<filesystem::Path::Data>() / k_FontPaths[i], rects, fontBitmapData);
	}

	BuildTextureAtlas(rects, fontBitmapData);

	for (auto& i : fontBitmapData)
	{
		delete[] i;
	}
}

void TextRenderer::LoadFont(Font font, const std::filesystem::path& path, std::vector<stbrp_rect>& rects,
                            std::vector<uint8_t*>& bitmaps)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Loading font: {}", path.stem().string());

	auto& fileSystem = Locator::filesystem::value();

	// read the entire file into a block of memory, meta looks this up by offset
	auto fntPath = path;
	fntPath.replace_extension(".fnt");

	if (!fileSystem.Exists(fntPath))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Can't find font: {}", path.stem().string());
		return;
	}

	// Load bitmap data from fnt file, store it for when we create the texture atlas
	{
		auto fntFile = fileSystem.Open(fntPath, filesystem::Stream::Mode::Read);
		const size_t size = fntFile->Size();
		auto* bitmapData = new uint8_t[size];
		fntFile->Read(bitmapData, size);

		bitmaps.push_back(bitmapData);
	}

	auto metPath = path;
	metPath.replace_extension(".met");
	auto meta = fileSystem.Open(metPath, filesystem::Stream::Mode::Read);

	auto fontSize = meta->ReadValue<uint32_t>();
	assert(fontSize == 80);

	std::u16string u16Name(0x80, '\0');
	meta->Read(reinterpret_cast<uint8_t*>(u16Name.data()), 0x100);

	_fontNames.push_back(u16Name);

	// glyphs
	const auto numGlyphs = meta->ReadValue<uint32_t>();
	std::vector<Glyph> glyphs(numGlyphs);
	for (uint32_t i = 0; i < numGlyphs; i++)
	{
		meta->Read(reinterpret_cast<uint8_t*>(&glyphs[i]),
		           sizeof(Glyph) - 16); // - 16 our renderer data.. (i probably dont want to use this now)

		_glyphs[std::make_tuple(font, glyphs[i].codepoint)] = glyphs[i];

		stbrp_rect rect;
		rect.id = (static_cast<int32_t>(font) << 24) | glyphs[i].codepoint;
		rect.w = glyphs[i].bitmapWidth + 1;
		rect.h = fontSize + 1;
		rects.push_back(rect);
	}
}

// 3x3 box blur
void BoxBlur(const uint8_t* input, uint8_t* output, int width, int height)
{
	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			int sum = 0;

			// Iterate over the 3x3 kernel
			for (int ky = -1; ky <= 1; ++ky)
			{
				for (int kx = -1; kx <= 1; ++kx)
				{
					int const px = std::clamp(x + kx, 0, width - 1);
					int const py = std::clamp(y + ky, 0, height - 1);

					// Accumulate pixel values
					sum += input[py * width + px];
				}
			}

			// Calculate the average and assign to the output
			output[y * width + x] = static_cast<uint8_t>(sum / 9);
		}
	}
}

void RenderGlyph(uint8_t* glyph, uint16_t width, uint16_t height, uint8_t* dest, size_t stride)
{
	int const pixels = width * height;
	auto* bitmap = new uint8_t[pixels];
	auto* bitmapBlurred = new uint8_t[pixels];

	int pos = 0;
	bool fill = false;
	while (pos < pixels)
	{
		// get the number of pixels to fill, if byte is 0xFF then get the next short.
		int count = *glyph++;
		if (count == 0xFF)
		{
			count = *(reinterpret_cast<uint16_t*>(glyph));
			glyph += 2;
		}

		// if count exceeds our remaining pixels, just fill the remaining
		if (count + pos > pixels)
		{
			count = pixels - pos;
		}

		// fill the number of pixels
		memset(&bitmap[pos], fill ? 0xFF : 0x00, count);

		// add count and alternate fill
		pos += count;
		fill = !fill;
	}

	BoxBlur(bitmap, bitmapBlurred, width, height);

	// copy to dest
	for (uint32_t y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			dest[y * stride + x] = bitmapBlurred[y * width + x];
		}
	}

	delete[] bitmap;
	delete[] bitmapBlurred;
}

void TextRenderer::BuildTextureAtlas(std::vector<stbrp_rect>& rects, std::vector<uint8_t*>& bitmapData)
{
	auto atlasWidth = 2048;

	// Pack our rects
	const int nodeCount = 2048; // make sure 'num_nodes' >= 'width'
	std::array<stbrp_node, nodeCount> nodes {};

	stbrp_context spc = {};
	stbrp_init_target(&spc, atlasWidth, 32768, nodes.data(), nodeCount);
	stbrp_pack_rects(&spc, rects.data(), static_cast<int>(rects.size()));

	auto atlasHeight = std::bit_ceil(static_cast<unsigned int>(spc.height));

	// Allocate our pixels
	auto* pixels = new uint8_t[atlasWidth * atlasHeight];
	std::memset(pixels, 0x00, atlasWidth * atlasHeight);

	// Render/rasterize font characters into the texture
	for (auto& rect : rects)
	{
		if (!static_cast<bool>(rect.was_packed))
		{
			continue;
		}

		size_t const stride = atlasWidth;
		size_t const offset = atlasWidth * rect.y + rect.x;

		Font const fontId = static_cast<Font>(rect.id >> 24);
		auto codepoint = static_cast<int16_t>(rect.id & 0xFFFF);

		auto key = std::make_tuple(fontId, codepoint);
		auto glyph = _glyphs[key];

		RenderGlyph(&bitmapData[static_cast<uint8_t>(fontId)][glyph.dataOffset], glyph.bitmapWidth, 80, &pixels[offset],
		            stride);

		// store the glyph uv
		_glyphs[key].u0 = static_cast<float>(rect.x) / static_cast<float>(atlasWidth);
		_glyphs[key].v0 = static_cast<float>(rect.y) / static_cast<float>(atlasHeight);
		_glyphs[key].u1 = static_cast<float>(rect.x + glyph.bitmapWidth) / static_cast<float>(atlasWidth);
		_glyphs[key].v1 = static_cast<float>(rect.y + 80) / static_cast<float>(atlasHeight);
	}

	_atlasTexture = std::make_unique<graphics::Texture2D>("Font Atlas");
	_atlasTexture->Create(static_cast<uint16_t>(atlasWidth), static_cast<uint16_t>(atlasHeight), 1, graphics::Format::A8,
	                      graphics::Wrapping::ClampEdge, graphics::Filter::Linear, pixels, atlasWidth * atlasHeight);
}

float TextRenderer::GetStringWidth(const std::u16string& text, float fontSize, Font font) const
{
	float width = 0.0f;

	for (const char16_t& c : text)
	{
		auto glyph = GetGlyph(font, c);
		width += glyph.xOffset + glyph.width + glyph.xAdvance;
	}

	return width * fontSize / 80.0f;
}

void TextRenderer::DrawText(const std::u16string& text, glm::vec2 position, float fontSize, Font font,
                            TextAlignment align) const
{
	// ...
	struct TextVertex
	{
		float x, y;
		float u, v;
		uint32_t rgba;
	};

	auto vCount = text.length() * 4;
	auto iCount = text.length() * 6;

	std::vector<TextVertex> vertices;
	std::vector<uint16_t> indices;

	const float fontScale = fontSize / 80.0f;

	float x = position.x;
	float const y = position.y;

	auto strWidth = GetStringWidth(text, fontSize, font);
	if (align == TextAlignment::Center)
	{
		x -= strWidth / 2;
	}
	if (align == TextAlignment::Right)
	{
		x -= strWidth;
	}

	// These are the only colours as far as I can tell?
	// I always thought the other gods coloured their text, but apparently not
	uint32_t rgba = 0xffffffff;
	if (font == Font::Evil)
	{
		rgba = 0xffb4b4ff;
	}
	if (font == Font::Good)
	{
		rgba = 0xffb7ebeb;
	}

	uint16_t vert = 0;

	for (const char16_t& c : text)
	{
		auto glyph = GetGlyph(font, c);

		// TODO(Matt): Handle \n and others
		if (c == ' ')
		{
			x += (glyph.width + glyph.xOffset + glyph.xAdvance) * fontScale;
			continue;
		}

		indices.insert(indices.end(),
		               {static_cast<uint16_t>(vert + 0), static_cast<uint16_t>(vert + 1), static_cast<uint16_t>(vert + 2),
		                static_cast<uint16_t>(vert + 2), static_cast<uint16_t>(vert + 1), static_cast<uint16_t>(vert + 3)});

		vert += 4;

		x += glyph.xOffset * fontScale;

		const float w = glyph.width * fontScale;
		const float h = fontSize;
		vertices.push_back(TextVertex {x, y, glyph.u0, glyph.v0, rgba});
		vertices.push_back(TextVertex {x + w, y, glyph.u1, glyph.v0, rgba});
		vertices.push_back(TextVertex {x, y + h, glyph.u0, glyph.v1, rgba});
		vertices.push_back(TextVertex {x + w, y + h, glyph.u1, glyph.v1, rgba});

		x += (glyph.width + glyph.xAdvance) * fontScale;
	}

	bgfx::TransientIndexBuffer tib;
	bgfx::TransientVertexBuffer tvb;
	bgfx::allocTransientIndexBuffer(&tib, static_cast<uint32_t>(iCount));
	bgfx::allocTransientVertexBuffer(&tvb, static_cast<uint32_t>(vCount), _vertexLayout);
	bx::memCopy(tib.data, indices.data(), indices.size() * sizeof(uint16_t));
	bx::memCopy(tvb.data, vertices.data(), vertices.size() * sizeof(TextVertex));

	uint64_t state = 0u | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
	state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	bgfx::setState(state);

	auto& renderer = Locator::rendererInterface::value();
	auto& shaderManager = renderer.GetShaderManager();
	const auto* textShader = shaderManager.GetShader("Text");

	textShader->SetTextureSampler("s_tex", 0, *_atlasTexture);

	bgfx::setVertexBuffer(0, &tvb, 0, static_cast<uint32_t>(vertices.size()));
	bgfx::setIndexBuffer(&tib, 0, static_cast<uint32_t>(indices.size()));
	bgfx::submit(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), textShader->GetRawHandle());
}

void TextRenderer::Draw()
{
	// No window? No size
	if (!Locator::windowing::has_value())
	{
		return;
	}

	const auto size = static_cast<glm::u16vec2>(Locator::windowing::value().GetSize());

	bgfx::setViewMode(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), bgfx::ViewMode::Sequential);

	const bgfx::Caps* caps = bgfx::getCaps();
	std::array<float, 16> ortho;
	bx::mtxOrtho(ortho.data(), 0.0f, size.x, size.y, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), nullptr, ortho.data());
	bgfx::setViewRect(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), 0, 0, static_cast<uint16_t>(size.x),
	                  static_cast<uint16_t>(size.y));

	DrawText(u"We're your conscience.", glm::vec2(size.x / 2, size.y - 192 + 32), 28.0f, Font::Evil, TextAlignment::Center);
	DrawText(u"Greetings.", glm::vec2(size.x / 2, size.y - 160 + 32), 28.0f, Font::Good, TextAlignment::Center);
	DrawText(u"Thank you! Thank you for your mercy!", glm::vec2(size.x / 2, size.y - 128 + 32), 28.0f, Font::Neutral,
	         TextAlignment::Center);

	DrawText(u"openblack 0.1", glm::vec2(size.x - 8, size.y - 32), 24.0f, Font::Neutral, TextAlignment::Right);
}

} // namespace openblack
