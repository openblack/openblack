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

#include <array>
#include <optional>

#include <glm/vec2.hpp>

namespace openblack::magic
{

/// Where miracles have been lately: the map in squares of 80 units, each marked full when a miracle runs in it and
/// fading a little every turn after. What reads it (the computer players, the map) is to come.
class SpellGrid
{
public:
	static constexpr int k_Side = 64;
	static constexpr float k_CellSize = 80.0f;
	static constexpr uint8_t k_Full = 0xFF;
	static constexpr uint8_t k_FadePerTurn = 0x20;

	/// The square a point of the ground is in, none off the map
	[[nodiscard]] static std::optional<glm::ivec2> CellOf(glm::vec2 xz);
	/// A miracle runs at a point
	void Mark(glm::vec2 xz, uint8_t value = k_Full);
	/// Every square fades by a turn
	void Fade();
	[[nodiscard]] uint8_t At(glm::vec2 xz) const;
	void Clear();

private:
	std::array<std::array<uint8_t, k_Side>, k_Side> _cells {};
};

} // namespace openblack::magic
