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

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

/// Black & White draws its sea in rows across the screen, two pixels apart, from where a square of sea around the island
/// meets the top of the screen down to its bottom. Each row has its own ripple and fades with its distance. This works
/// out the rows on the CPU; fs_water draws them.
namespace openblack::graphics::sea_rows
{

/// The square of sea the rows are worked out from: 30000 units a side on the sea level, about the middle of the map
inline constexpr float k_SquareMinimum = -12440.0f;
inline constexpr float k_SquareMaximum = 17560.0f;

/// How long the sea's texture repeats, in units: from 2000 down to 200 as the detail level's water tiling goes from 0 to
/// 1. At a tiling of 0 the sea is instead a still square of 140000 units repeating its texture 50 times.
[[nodiscard]] constexpr float Period(float waterTiling)
{
	return 2000.0f - (1800.0f * waterTiling);
}
inline constexpr float k_StillPeriod = 2800.0f;

/// The highest and lowest the square of sea reaches on the screen, in pixels from the top, and how far away it is there
/// as one over its view depth. The top is raised to the screen's top and the bottom lowered to its bottom without
/// their depths following.
struct ScreenRange
{
	float top;
	float bottom;
	float inverseDepthTop;
	float inverseDepthBottom;
};

/// Where the square of sea lies on the screen, clipped to the view: none when it is off the screen
/// @param viewProjection world to clip space, with w the view depth
[[nodiscard]] std::optional<ScreenRange> ComputeScreenRange(const glm::mat4& viewProjection, glm::vec2 viewportSize,
                                                            float nearDistance);

/// The rows: row r is at screen y first + 2 r, and its one over view depth inverseDepth + r inverseStep
struct Rows
{
	int first;
	/// Rows 0 to count are drawn
	int count;
	float inverseDepth;
	float inverseStep;
	/// The first row is nearly clear when the sea starts below the top of the screen
	bool softTop;
};
[[nodiscard]] Rows MakeRows(const ScreenRange& range);

} // namespace openblack::graphics::sea_rows
