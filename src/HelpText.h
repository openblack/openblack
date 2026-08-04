/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string>

#include "HelpTextEnums.h"

namespace openblack
{

class Color;

/**
 * @brief HelpText displays help text... Neat!
 *
 */
class HelpText
{
public:
	HelpText();

	void Draw();

	void AddLine(const std::u16string& text, HelpTextNarrator narrator);

private:
	void DrawBox(float x1, float y1, float x2, float y2, Color color) const;
	void UpdateRegion();

	struct Region
	{
		int x1;
		int y1;
		int x2;
		int y2;
	};

	Region TextRegion;

	struct Entry
	{
		std::u16string Text;
		HelpTextNarrator Narrator;
	};

	// 6 if we want to animate the leaving and entering line
	std::array<Entry, 4> m_entries;
	int m_topEntry;

	// Do we want to display a single middle aligned line of text?
	bool m_singleLine;
};

} // namespace openblack
