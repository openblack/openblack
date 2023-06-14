/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Level.h"

#include <fstream>

using namespace openblack;

std::string extractQuote(std::string& string)
{
	size_t const first(string.find('\"'));
	size_t const second(string.find('\"', first + 1));
	return string.substr(first + 1, second - first - 1);
}

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
			levelName = extractQuote(line);
		}
		if (line.find(gameMessageLine) != std::string::npos)
		{
			description = extractQuote(line);
		}
	}
}
