/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/entity/entity.hpp>

#include "Enums.h"

namespace openblack::ecs::components
{

/// A rock, static or dead tree a player put down gently by a town: the town's artefact, which its people dance round
struct TownArtefact
{
	/// The town it belongs to, none while it is out of any town (held in a hand)
	entt::entity town {entt::null};
	/// The player who made it an artefact, or last took it
	std::optional<PlayerNames> player;
	/// Its worth to the town: how much it impresses villagers when it is made, raised as people dance round it
	float value {0.0f};
};

} // namespace openblack::ecs::components
