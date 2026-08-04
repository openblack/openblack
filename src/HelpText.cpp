/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HelpText.h"

#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <glm/glm.hpp>

#include "Graphics/RenderPass.h"
#include "Graphics/RendererInterface.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/TextRenderer.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;

HelpText::HelpText()
{
	TextRegion = {};
	m_topEntry = 0;

	AddLine(u"And Evil.", HelpTextNarrator::EvilSpirit);
	AddLine(u"Yin and Yang.", HelpTextNarrator::GoodSpirit);
	AddLine(u"Black and White.", HelpTextNarrator::EvilSpirit);
	AddLine(u"As part of you we'll guide you through this world.", HelpTextNarrator::EvilSpirit);
}

void HelpText::AddLine(const std::u16string& text, HelpTextNarrator narrator)
{
	m_topEntry = (m_topEntry + 1) % 4;
	m_entries[m_topEntry] = {.Text = text, .Narrator = narrator};
}

void HelpText::UpdateRegion()
{
	// This would be called each frame during a widescreen cinematic or when resolution changes
	// I'm just calling it non stop for now

	if (!Locator::windowing::has_value())
	{
		return;
	}

	const auto screenSize = static_cast<glm::u16vec2>(Locator::windowing::value().GetSize());

	int areaHeight = floor(screenSize.y * 0.12666667);

	TextRegion.x1 = 0;
	TextRegion.y1 = screenSize.y - areaHeight - 27;
	TextRegion.x2 = screenSize.x;
	TextRegion.y2 = screenSize.y - 27;
}

void HelpText::Draw()
{
	UpdateRegion();

	// No window? No size
	if (!Locator::windowing::has_value())
	{
		return;
	}

	const auto windowSize = static_cast<glm::u16vec2>(Locator::windowing::value().GetSize());

	{
		bgfx::setViewMode(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), bgfx::ViewMode::Sequential);

		const bgfx::Caps* caps = bgfx::getCaps();
		std::array<float, 16> ortho;
		bx::mtxOrtho(ortho.data(), 0.0f, windowSize.x, windowSize.y, 0.0f, 0.0f, 1000.0f, 0.0f, caps->homogeneousDepth);
		bgfx::setViewTransform(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), nullptr, ortho.data());
		bgfx::setViewRect(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), 0, 0, static_cast<uint16_t>(windowSize.x),
		                  static_cast<uint16_t>(windowSize.y));
	}

	auto& textRenderer = Locator::textRenderer::value();

	// lets work out our text region
	int areaHeight = TextRegion.y2 - TextRegion.y1;

	DrawBox(TextRegion.x1, TextRegion.y1, TextRegion.x2, TextRegion.y2, Color(0, 0, 0, 128));

	// Lets render 4 test lines
	float flLineAlpha = 1.0f;

	float fontSize = windowSize.y / 30.0f; // 36 on 1080p
	float y_offset = fontSize / 3.0f;

	bool isSingleLine = false;
	if (isSingleLine)
	{
		y_offset = ((areaHeight + 1) / 2) - fontSize / 2;
	}

	int count = isSingleLine ? 1 : 4;
	for (int i = 0; i < count; i++)
	{
		auto& entry = m_entries[(m_topEntry - i + 4) % 4];
		if (entry.Text.empty())
			break;

		auto font = TextRenderer::Font::Neutral;
		auto color = colors::k_White;
		if (entry.Narrator == HelpTextNarrator::GoodSpirit)
		{
			font = TextRenderer::Font::Good;
			color = colors::k_Good;
		}
		if (entry.Narrator == HelpTextNarrator::EvilSpirit)
		{
			font = TextRenderer::Font::Evil;
			color = colors::k_Evil;
		}

		auto lFontSize = fontSize * flLineAlpha;

		color.a = 255 * flLineAlpha;

		glm::vec2 pos(TextRegion.x2 / 2, TextRegion.y1 + y_offset);
		textRenderer.DrawText(entry.Text, pos, lFontSize, font, color, TextRenderer::TextAlignment::Center);

		y_offset += lFontSize * flLineAlpha;
		flLineAlpha *= (i == 0) ? 0.86f : 0.93f;
	}
}

void HelpText::DrawBox(float x1, float y1, float x2, float y2, Color color) const
{
	struct UIVertex
	{
		glm::vec2 position;
		uint32_t rgba;
	};

	std::array<UIVertex, 4> vertices = {
	    UIVertex {glm::vec2(x1, y1), static_cast<uint32_t>(color)}, UIVertex {glm::vec2(x2, y1), static_cast<uint32_t>(color)},
	    UIVertex {glm::vec2(x2, y2), static_cast<uint32_t>(color)}, UIVertex {glm::vec2(x1, y2), static_cast<uint32_t>(color)}};

	std::array<uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};

	bgfx::VertexLayout _vertexLayout;
	_vertexLayout.begin()
	    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();

	bgfx::TransientIndexBuffer tib;
	bgfx::TransientVertexBuffer tvb;
	bgfx::allocTransientIndexBuffer(&tib, static_cast<uint32_t>(indices.size()));
	bgfx::allocTransientVertexBuffer(&tvb, static_cast<uint32_t>(vertices.size()), _vertexLayout);
	bx::memCopy(tib.data, indices.data(), indices.size() * sizeof(uint16_t));
	bx::memCopy(tvb.data, vertices.data(), vertices.size() * sizeof(UIVertex));

	uint64_t state = 0u | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_MSAA;
	state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);

	bgfx::setState(state);

	auto& renderer = Locator::rendererInterface::value();
	auto& shaderManager = renderer.GetShaderManager();
	const auto* textShader = shaderManager.GetShader("UIBox");

	bgfx::setVertexBuffer(0, &tvb, 0, static_cast<uint32_t>(vertices.size()));
	bgfx::setIndexBuffer(&tib, 0, static_cast<uint32_t>(indices.size()));
	bgfx::submit(static_cast<bgfx::ViewId>(graphics::RenderPass::Gui), textShader->GetRawHandle());
}
