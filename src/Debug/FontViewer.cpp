/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "FontViewer.h"

#include <imgui.h>

#include "Game.h"
#include "Graphics/TextRenderer.h"
#include "Graphics/Texture2D.h"
#include "Gui.h"
#include "ImGuiUtils.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::debug::gui;

FontViewer::FontViewer() noexcept
    : Window("Font Viewer", ImVec2(600.0f, 600.0f))
{
}

void FontViewer::Draw() noexcept
{
	auto& textRenderer = Locator::textRenderer::value();

	auto const& texture = *textRenderer._atlasTexture;

	// 3 fonts
	for (int i = 0; i < 3; i++)
	{
		auto font = textRenderer._fontData[i];
		ImGui::Text("Font: %s (Size: %d)", font.name.c_str(), font.size);

		for (char c = 33; c <= 126; c++)
		{
			if (c == 'A' || c == 'a')
				ImGui::NewLine();

			auto glyph = textRenderer.GetGlyph(static_cast<TextRenderer::Font>(i), c);
			ImGui::Image(texture.GetNativeHandle(), ImVec2(glyph.width / 2.0f, 80.0f / 2.0f), ImVec2(glyph.u0, glyph.v0),
			             ImVec2(glyph.u1, glyph.v1));
			ImGui::SetItemTooltip("Glyph: %c\nWidth: %d (%f)\nInt X Offset: %d\nX Offset: %f\nX Advance: %f\n\"padding\": %d",
			                      c, glyph.width, glyph.width, glyph.int_xoffset, glyph.xOffset, glyph.xAdvance, glyph.padding);
			ImGui::SameLine();
		}

		ImGui::NewLine();
	}
}

void FontViewer::Update() noexcept {}

void FontViewer::ProcessEventOpen(const SDL_Event&) noexcept {}

void FontViewer::ProcessEventAlways(const SDL_Event&) noexcept {}
