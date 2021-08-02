/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Temple.h"

#include <string>

namespace openblack::entities::components
{
enum class TempleIndoorRoom
{
	ChallengeRoom,
	CreatureCave,
	CreditsRoom,
	MainRoom,
	MultiplayerRoom,
	OptionsRoom,
	SaveGameRoom
};

struct Temple
{
	std::string owner;
};

struct TempleWorshipSite
{
	std::string owner;
};

struct TempleInterior
{
	TempleIndoorRoom room;
};
}