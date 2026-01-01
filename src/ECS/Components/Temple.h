/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Enums.h"

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

struct TempleInteriorPart
{
	TempleRoom room;
};

struct Temple
{
	PlayerNames owner;
};
} // namespace openblack::ecs::components
