/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <span>

#include <glm/vec2.hpp>

#include "ECS/Components/Weather.h"

namespace openblack::ecs::systems
{

/// The snow lying on the island (see snow_cover): the storms that snow pile it up and it melts away
class SnowSystemInterface
{
public:
	virtual ~SnowSystemInterface() = default;

	/// As a new land opens: no snow anywhere
	virtual void Reset() = 0;
	/// Once a game turn, after the storms have moved on: each live storm that snows lays its snow, then some of it melts
	virtual void ProcessTurn(std::span<const components::Storm> storms) = 0;

	/// How deep the snow lies in each cell of the grid, row by row along z
	[[nodiscard]] virtual std::span<const float> GetDepths() const = 0;
	/// How deep the snow lies at a point
	[[nodiscard]] virtual float GetDepth(glm::vec2 xz) const = 0;
	/// Changes whenever the snow does, so that what is drawn from it is only refreshed then
	[[nodiscard]] virtual uint32_t GetRevision() const = 0;
};

} // namespace openblack::ecs::systems
