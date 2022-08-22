/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <string>

#include "Temple.h"

namespace openblack::ecs::components
{
enum class TempleRoom
{
	ChallengeRoom,
	CreatureCave,
	CreditsRoom,
	MainRoom,
	MultiplayerRoom,
	OptionsRoom,
	SaveGameRoom
};

struct TempleInteriorStructure
{
	TempleRoom room;
};
} // namespace openblack::ecs::components