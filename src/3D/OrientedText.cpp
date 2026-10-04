/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "OrientedText.h"

#include "Gui/GameFont.h"

using namespace openblack;

int openblack::OrientedTextDepthAxis(int across, int down)
{
	// The game takes the first axis unless across or down is it, then the second unless either is that too
	if (across != 0 && down != 0)
	{
		return 0;
	}
	return across == 1 || down == 1 ? 2 : 1;
}

void openblack::AppendOrientedText(std::vector<OrientedTextVertex>& vertices, const gui::GameFont& font, const TextFrame& frame,
                                   int across, int down, std::u16string_view text, glm::vec3 offset, float size, float stretch,
                                   glm::u8vec4 colour)
{
	if (text.empty() || font.GetHeight() == 0)
	{
		return;
	}
	const auto& alongText = frame.axes.at(static_cast<size_t>(across));
	const auto& downLetters = frame.axes.at(static_cast<size_t>(down));
	const auto& offPage = frame.axes.at(static_cast<size_t>(OrientedTextDepthAxis(across, down)));
	// A font unit is a size high line's height's worth of the font's height, stretched along the text
	const auto unit = alongText * (size / static_cast<float>(font.GetHeight()) * stretch);
	const auto height = downLetters * size;
	auto pen = frame.origin + (alongText * offset.x) + (downLetters * offset.y) + (offPage * offset.z);

	const auto packed = static_cast<uint32_t>(colour.r) | (static_cast<uint32_t>(colour.g) << 8) |
	                    (static_cast<uint32_t>(colour.b) << 16) | (static_cast<uint32_t>(colour.a) << 24);
	const auto atlasSize = glm::vec2(font.GetAtlasSize());
	for (const auto c : text)
	{
		if (c == u'\n' || c == u'\r' || c == u'\xF8FE')
		{
			continue;
		}
		const auto* glyph = font.Find(c);
		if (glyph == nullptr)
		{
			continue;
		}
		if (c == u' ')
		{
			pen += unit * (glyph->left + glyph->ink + glyph->right);
			continue;
		}
		pen += unit * glyph->left;
		const auto topLeft = pen;
		const auto bottomLeft = pen + height;
		const auto topRight = pen + (unit * (glyph->ink + 1.0f));
		const auto bottomRight = topRight + height;
		const auto uvMin = glm::vec2(glyph->atlasMin) / atlasSize;
		const auto uvMax = glm::vec2(glyph->atlasMax) / atlasSize;
		vertices.push_back({topLeft, uvMin, packed});
		vertices.push_back({bottomLeft, {uvMin.x, uvMax.y}, packed});
		vertices.push_back({topRight, {uvMax.x, uvMin.y}, packed});
		vertices.push_back({topRight, {uvMax.x, uvMin.y}, packed});
		vertices.push_back({bottomLeft, {uvMin.x, uvMax.y}, packed});
		vertices.push_back({bottomRight, uvMax, packed});
		pen += unit * (glyph->ink + glyph->right);
	}
}
