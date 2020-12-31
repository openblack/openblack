/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

namespace openblack::entities::components
{

enum class MagicTreeType
{
};

struct Tree
{
	/// Originally TreeInfo
	enum class Info
	{
		Beech = 0,
		Birch = 1,
		Cedar = 2,
		Conifer = 3,
		ConiferA = 4,
		Oak = 5,
		OakA = 6,
		Olive = 7,
		Palm = 8,
		PalmA = 9,
		PalmB = 10,
		PalmC = 11,
		Pine = 12,
		Bush = 13,
		BushA = 14,
		BushB = 15,
		Cypress = 16,
		CypressA = 17,
		Copse = 18,
		CopseA = 19,
		Hedge = 20,
		HedgeA = 21,
		Burnt = 22,
	};

	Info treeInfo;
};

} // namespace openblack::entities::components
