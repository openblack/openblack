/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <filesystem>
#include <string>

namespace openblack
{
class Level
{
public:
	Level(std::filesystem::path path)
	    : _scriptPath(path)
	{
	}

	std::string GetName() const { return _scriptPath.filename().string(); };
	const std::filesystem::path& GetScriptPath() const { return _scriptPath; };

private:
	std::string _name;
	std::filesystem::path _scriptPath;
};
} // namespace openblack
