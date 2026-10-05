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

#include <entt/entity/fwd.hpp>

#include "ECS/Systems/AlignmentSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class AlignmentSystem final: public AlignmentSystemInterface
{
public:
	[[nodiscard]] float GetPlayerAlignment(PlayerNames player) const override;
	void SetPlayerAlignment(PlayerNames player, float alignment) override;
	void AddPlayerAlignment(PlayerNames player, float change) override;
	void UpdateTurn() override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;
	[[nodiscard]] float GetCameraAlignment() const override { return _camera; }
	[[nodiscard]] float GetSkyAlignment() const override { return _sky; }

private:
	/// The entity of a player who is in the game
	[[nodiscard]] static std::optional<entt::entity> FindPlayer(PlayerNames player);

	float _camera {0.0f};
	float _sky {0.0f};
};

} // namespace openblack::ecs::systems
