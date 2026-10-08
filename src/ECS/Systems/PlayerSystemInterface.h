/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <entt/fwd.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{
class PlayerSystemInterface
{
public:
	virtual void RegisterPlayers() = 0;
	virtual void AddPlayer(entt::entity playerEntity) = 0;
	[[nodiscard]] virtual entt::entity GetPlayer(PlayerNames name) const = 0;
	/// The player at this machine, whose hand the mouse moves. openblack is played by one player, the first.
	[[nodiscard]] virtual PlayerNames GetLocalPlayer() const { return PlayerNames::PLAYER_ONE; }
	/// Whether the computer plays a player: every player but the one at this machine
	[[nodiscard]] virtual bool IsComputerPlayer(PlayerNames name) const
	{
		return name != GetLocalPlayer() && name != PlayerNames::NEUTRAL;
	}
};
} // namespace openblack::ecs::systems
