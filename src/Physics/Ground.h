/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack::physics
{

/// What the physics asks of the land under a point. The sea's surface is the height 0.
class Ground
{
public:
	virtual ~Ground() = default;

	/// The land's height under a point
	[[nodiscard]] virtual float HeightAt(glm::vec2 xz) const = 0;
	/// The flat normal of the land's triangle under a point
	[[nodiscard]] virtual glm::vec3 NormalAt(glm::vec2 xz) const = 0;
	/// Whether the map cell nearest to a point (its coordinates rounded to the nearest cell) is open sea: off the map,
	/// without a cell, or a cell whose land is at the sea's level
	[[nodiscard]] virtual bool IsSeaCell(glm::vec2 xz) const = 0;
};

} // namespace openblack::physics
