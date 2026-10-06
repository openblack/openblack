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
#include <string_view>

namespace openblack::particles::draw
{

/// How the game draws an effect among the other things that blend.
///
/// Most effects are sorted: every sprite, model, mist and ribbon takes its own place among everything else that blends,
/// the farthest from the camera first. A few spot visuals are queued: the whole effect takes one place, at its origin,
/// and is drawn there all at once in the order its collections hold it. The miracle in the hand is drawn at once, in
/// that order, just after the hand.
enum class DrawPath : uint8_t
{
	Sorted,
	Queued,
	Immediate,

	_Count
};

constexpr std::array<std::string_view, static_cast<size_t>(DrawPath::_Count)> k_DrawPathNames {"Sorted", "Queued", "Immediate"};

} // namespace openblack::particles::draw
