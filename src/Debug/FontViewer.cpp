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
#include "Graphics/Font/Font.h"
#include "Graphics/Texture2D.h"
#include "Gui.h"

using namespace openblack;
using namespace openblack::debug::gui;

FontViewer::FontViewer()
    : Window("Font Viewer", ImVec2(600.0f, 600.0f))
{
}

void FontViewer::Draw(Game& game)
{
	ImGuiStyle& style = ImGui::GetStyle();

	auto const& fonts = game.GetFonts();

	for (const auto& font : fonts)
	{
		auto const& glyphs = font->GetGlyphs();
		const bool font_details_opened = ImGui::TreeNode(font.get(), "Font \"%s\" %d px, %zu glyphs", font->GetName().c_str(),
		                                                 font->GetSize(), glyphs.size());
		if (font_details_opened)
		{
			if (ImGui::TreeNode("Atlas Texture"))
			{
				auto const& texture = font->GetAtlasTexture();
				ImGui::Image(texture.GetNativeHandle(), ImVec2(texture.GetWidth(), texture.GetHeight()));
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Glyphs", "Glyphs (%zu)", glyphs.size()))
			{
				for (unsigned int base = 0; base <= IM_UNICODE_CODEPOINT_MAX; base += 256)
				{
					int count = 0;
					for (unsigned int n = 0; n < 256; n++)
					{
						count += font->FindGlyphNoFallback(base + n) != nullptr ? 1 : 0;
					}

					if (count > 0 &&
					    ImGui::TreeNode(reinterpret_cast<void*>(static_cast<intptr_t>(base)), "U+%04X..U+%04X (%d %s)", base,
					                    base + 255, count, count > 1 ? "glyphs" : "glyph"))
					{
						float cellSize = font->GetSize() * 1.0f;
						float cellSpacing = style.ItemSpacing.y;
						ImVec2 basePos = ImGui::GetCursorScreenPos();
						ImDrawList* drawList = ImGui::GetWindowDrawList();

						for (unsigned int n = 0; n < 256; n++)
						{
							ImVec2 cellPos1(basePos.x + (n % 8) * (cellSize + cellSpacing),
							                basePos.y + (n / 8) * (cellSize + cellSpacing));
							ImVec2 cellPos2(cellPos1.x + cellSize, cellPos1.y + cellSize);
							const Font::Glyph* glyph = font->FindGlyphNoFallback(base + n);

							drawList->AddRect(cellPos1, cellPos2,
							                  glyph ? IM_COL32(255, 255, 255, 100) : IM_COL32(255, 255, 255, 50));
							if (glyph)
							{
								union
								{
									struct
									{
										bgfx::TextureHandle handle;
										uint8_t flags;
										uint8_t mip;
									} s;
									ImTextureID ptr;
								} texture;
								texture.s.handle = font->GetAtlasTexture().GetNativeHandle();
								texture.s.flags = IMGUI_FLAGS_ALPHA_BLEND;
								texture.s.mip = 0;

								ImVec2 glyphPos1(basePos.x + (n % 8) * (cellSize + cellSpacing),
								                 basePos.y + (n / 8) * (cellSize + cellSpacing));
								ImVec2 glyphPos2(cellPos1.x + glyph->width, cellPos1.y + font->GetSize());

								drawList->AddImage(texture.ptr, glyphPos1, glyphPos2, ImVec2(glyph->u0, glyph->v0),
								                   ImVec2(glyph->u1, glyph->v1));
							}
							if (glyph && ImGui::IsMouseHoveringRect(cellPos1, cellPos2))
							{
								ImGui::BeginTooltip();
								ImGui::Text("Codepoint: U+%04X", base + n);
								ImGui::Separator();
								ImGui::Text("UV: (%.3f,%.3f)->(%.3f,%.3f)", glyph->u0, glyph->v0, glyph->u1, glyph->v1);
								ImGui::Text("x_offset: %d width: %d", glyph->xOffset, glyph->width);
								ImGui::Text("fXoffset: %.3f fWidth: %.3f unk2: %.3f", glyph->fXoffset, glyph->fWidth,
								            glyph->unk2);
								/*ImGui::Text("Visible: %d", glyph->Visible);
								ImGui::Text("AdvanceX: %.1f", glyph->AdvanceX);
								ImGui::Text("Pos: (%.2f,%.2f)->(%.2f,%.2f)", glyph->X0, glyph->Y0, glyph->X1, glyph->Y1);
								ImGui::Text("UV: (%.3f,%.3f)->(%.3f,%.3f)", glyph->U0, glyph->V0, glyph->U1, glyph->V1);*/
								ImGui::EndTooltip();
							}
						}

						ImGui::Dummy(ImVec2((cellSize + cellSpacing) * 8, (cellSize + cellSpacing) * 32));
						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}
	}
}

void FontViewer::Update(Game& game, const Renderer& renderer) {}

void FontViewer::ProcessEventOpen(const SDL_Event& event) {}

void FontViewer::ProcessEventAlways(const SDL_Event& event) {}
