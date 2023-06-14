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
#include <utility>

#include "Common/FileSystem.h"
#include "Common/StringUtils.h"
#include "Game.h"

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

bool Level::IsLevelFile(const std::filesystem::path& path)
{
	std::string const loadLandscapeLine("LOAD_LANDSCAPE");
	std::fstream levelFile;
	levelFile.open(path, std::ios::in);
	while (!levelFile.eof())
	{
		std::string line;
		std::getline(levelFile, line);
		if (line.find(loadLandscapeLine) != std::string::npos)
			return true;
	}

	return false;
}

Level Level::ParseLevel(const std::filesystem::path& path, Level::LandType landType)
{
	std::string description;
	std::string levelName(path.stem().filename().string());
	bool isValid(false);

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
			std::filesystem::path const landscapePath(string_utils::ExtractQuote(line));
			isValid = Game::Instance()->GetFileSystem().Exists(landscapePath);
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

	return {levelName, path, description, landType, isValid};
}
