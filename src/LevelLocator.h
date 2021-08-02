/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <Common/FileSystem.h>
#include <vector>

#include "Level.h"

namespace openblack
{
class LevelLocator
{
public:
	bool LoadLevels(const FileSystem& fs);
	const std::vector<Level>& GetCampaigns() const { return _campaigns; };
	const std::vector<Level>& GetPlaygrounds() const { return _playgrounds; };

private:
	std::vector<Level> _campaigns;
	std::vector<Level> _playgrounds;
	const std::string _levelExt = ".txt";
};
} // namespace openblack
