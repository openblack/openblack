/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <span>
#include <vector>

#include <glm/vec3.hpp>

#include "3D/Rain.h"
#include "3D/Snowfall.h"

namespace openblack::ecs::systems
{

/// The snow the world shows falling where the weather snows (see snowfall)
class SnowfallSystemInterface
{
public:
	virtual ~SnowfallSystemInterface() = default;

	/// New flakes, scattered as the snow starts
	virtual void Reset() = 0;
	/// Once a frame: the flakes fall as the rain does, from as high and as fast, if they were drawn
	virtual void Update(float seconds, const rain::Fall& fall) = 0;
	/// The snow to draw this frame over the quarters of the land blocks about the camera; taking some counts the flakes
	/// as drawn
	[[nodiscard]] virtual std::vector<snowfall::Tile> TakeTiles(const glm::vec3& camera) = 0;
	[[nodiscard]] virtual std::span<const snowfall::Flake> GetFlakes() const = 0;
};

} // namespace openblack::ecs::systems
