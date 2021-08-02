/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LevelLocator.h"

using namespace openblack;

bool LevelLocator::LoadLevels(const FileSystem& fs)
{
	_campaigns.clear();
	_playgrounds.clear();

	if (!fs.Exists(fs.ScriptsPath()))
	{
		return false;
	}

	auto campaignPaths = fs.GetAllFilePaths(fs.ScriptsPath().string(), _levelExt, false);

	for (auto path : campaignPaths)
	{
		_campaigns.emplace_back(Level(std::move(path)));
	}

	auto playgroundsPath = fs.ScriptsPath() / "Playgrounds";

	if (!fs.Exists(playgroundsPath))
	{
		return false;
	}

	auto playGroundPaths = fs.GetAllFilePaths((playgroundsPath).string(), _levelExt, false);

	for (auto path : playGroundPaths)
	{
		_playgrounds.emplace_back(Level(std::move(path)));
	}

	return true;
}
