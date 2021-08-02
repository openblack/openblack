/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <string>
#include <algorithm>

namespace openblack
{
inline bool EndsWith(std::string const& str, std::string const& ending)
{
	if (ending.size() > str.size())
	{
		return false;
	}

	return std::equal(ending.rbegin(), ending.rend(), str.rbegin());
}

inline bool BeginsWith(std::string const& str, std::string const& ending)
{
	if (ending.size() > str.size())
	{
		return false;
	}

	return std::equal(ending.begin(), ending.end(), str.begin());
}

inline void LowerCase(std::string& str)
{
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}
}