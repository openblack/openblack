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
	enum class LandType : uint32_t
	{
		Skirmish = 0,
		Campaign = 1,
	};

	Level(std::string name, std::filesystem::path path, std::string description, LandType landType, bool isValid = false)
	    : _name(std::move(name))
	    , _scriptPath(std::move(path))
	    , _description(std::move(description))
	    , _landType(landType)
	    , _isValid(isValid) {};

	[[nodiscard]] const std::string& GetName() const { return _name; };
	[[nodiscard]] const std::filesystem::path& GetScriptPath() const { return _scriptPath; };
	[[nodiscard]] LandType getType() const { return _landType; };
	[[nodiscard]] const std::string& GetDescription() const { return _description; };
	[[nodiscard]] bool IsValid() const { return _isValid; };

private:
	std::string _name;
	std::filesystem::path _scriptPath;
	std::string _description;
	LandType _landType;
	bool _isValid;
};
} // namespace openblack
