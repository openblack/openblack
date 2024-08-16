/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <memory>

#include "InfoConstants.h"

namespace openblack
{

class InfoFile
{
public:
	std::unique_ptr<const InfoConstants> LoadFromFile(const std::filesystem::path& path);
};

} // namespace openblack
