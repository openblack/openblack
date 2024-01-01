/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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
};
} // namespace openblack::ecs::systems
