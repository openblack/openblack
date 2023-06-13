/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <string>
#include <utility>

namespace openblack
{
class Level
{
public:
	Level(std::string name, std::filesystem::path path, bool isCampaign, std::string description)
	    : _name(std::move(name))
	    , _scriptPath(std::move(path))
	    , _isCampaign(isCampaign)
	    , _description(std::move(description)) {};

	[[nodiscard]] const std::string& GetName() const { return _name; };
	[[nodiscard]] const std::filesystem::path& GetScriptPath() const { return _scriptPath; };
	[[nodiscard]] bool IsCampaign() const { return _isCampaign; };

private:
	std::string _name;
	std::filesystem::path _scriptPath;
	bool _isCampaign;
	int _playerCount;
	std::string _description;
};
} // namespace openblack
