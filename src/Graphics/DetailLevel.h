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

#include <algorithm>
#include <array>

/// Black & White's graphics detail levels, 0 to 6, 4 by default. Level 5 is the custom one, whose values here are where
/// the player's own settings start.
namespace openblack::graphics::detail_level
{

inline constexpr uint8_t k_Default = 4;

/// How finely the sea's texture repeats, from 0 to 1 (see sea_rows::Period). At 0 the sea is a still square.
inline constexpr std::array<float, 7> k_WaterTiling = {0.0f, 0.2f, 0.4f, 0.6f, 0.8f, 0.5f, 1.0f};

/// Whether the sky has clouds
inline constexpr std::array<bool, 7> k_Clouds = {false, false, false, true, true, true, true};

[[nodiscard]] constexpr bool Clouds(uint8_t level)
{
	return k_Clouds.at(std::min<size_t>(level, k_Clouds.size() - 1));
}

[[nodiscard]] constexpr float WaterTiling(uint8_t level)
{
	return k_WaterTiling.at(std::min<size_t>(level, k_WaterTiling.size() - 1));
}

} // namespace openblack::graphics::detail_level
