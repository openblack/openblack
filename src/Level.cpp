/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Level.h"
#include "Common/StringUtils.h"

#include <fstream>
#include <utility>

using namespace openblack;

Level::Level(std::string name, std::filesystem::path path, std::string description, LandType landType, bool isValid)
	    : _name(std::move(name))
	    , _scriptPath(std::move(path))
	    , _description(std::move(description))
	    , _landType(landType)
	    , _isValid(isValid) {};

const std::string& Level::GetName() const
{
	return _name;
}

const std::filesystem::path& Level::GetScriptPath() const
{
	return _scriptPath;
}

Level::LandType Level::GetType() const
{
	return _landType;
}

const std::string& Level::GetDescription() const
{
	return _description;
}

bool Level::IsValid() const
{
	return _isValid;
}

void Level::ParseLevel(const std::filesystem::path& path, bool& isValid, std::string& levelName, std::string& description)
{
	std::string const loadLandscapeLine("LOAD_LANDSCAPE");
	std::string const startMessageLine("START_GAME_MESSAGE");
	std::string const gameMessageLine("ADD_GAME_MESSAGE_LINE");

	std::fstream levelFile;
	levelFile.open(path, std::ios::in);
	while (!levelFile.eof())
	{
		std::string line;
		std::getline(levelFile, line);
		if (!isValid && line.find(loadLandscapeLine) != std::string::npos)
		{
			isValid = true;
		}
		if (line.find(startMessageLine) != std::string::npos)
		{
			levelName = string_utils::ExtractQuote(line);
		}
		if (line.find(gameMessageLine) != std::string::npos)
		{
			description = string_utils::ExtractQuote(line);
		}
	}
}
