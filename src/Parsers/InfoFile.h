/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <filesystem>
#include <optional>

// NOLINTBEGIN(readability-identifier-naming): Naming comes from vanilla, this we maintain the style
#include "InfoConstants.h"

namespace openblack
{

class InfoFile
{
public:
	bool LoadFromFile(const std::filesystem::path& path, InfoConstants& infos);
};

} // namespace openblack
