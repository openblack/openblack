/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <span>
#include <string_view>

#include <entt/entity/fwd.hpp>

#include "3D/MapCoords.h"

namespace openblack::fireflies
{
class RewardTable;
}

namespace openblack::ecs::systems
{

/// The land's fireflies (components::Firefly). By day they hide in trees and rocks; each evening, once the sky darkens
/// past dusk, they come out one a turn to hover by the buildings, and each morning, once it brightens past dawn, they go
/// back one a turn to the nearest tree or rock. Each nightfall after a morning tops them up to the land's most. A hand
/// lifting the tree or rock one hides in catches it, for a one-shot miracle from the land's reward table.
class FireflySystemInterface
{
public:
	virtual ~FireflySystemInterface() = default;

	/// A firefly hiding at a spot, as a land's script places one
	virtual entt::entity Create(const map_coords::MapCoords& spot) = 0;
	/// Once a game turn: the night sends one out or one home, and every firefly flies on
	virtual void ProcessTurn() = 0;
	/// Once a frame: each firefly that is out is drawn where it drifts, by the frame's game time and how far through the
	/// turn the frame is
	virtual void Update(float milliseconds, float turnFraction) = 0;
	/// A hand takes up whatever stands at a spot: a firefly hiding exactly there is caught, and leaves its reward there.
	/// Whether one was
	virtual bool Catch(const map_coords::MapCoords& spot) = 0;
	/// A land's script sets a miracle's weight in the reward table, by its effect's name; an unknown name is ignored
	virtual void SetRewardWeight(std::string_view magicName, float weight) = 0;
	[[nodiscard]] virtual const fireflies::RewardTable& GetRewards() const = 0;
	/// The fireflies in the order the night sends them
	[[nodiscard]] virtual std::span<const entt::entity> GetFireflies() const = 0;
	/// As a land closes: no fireflies, the reward weights cleared, and the next nightfall tops them up
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems
