/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

/// How near the camera draws things: close to the ground it draws them nearer, so the land and the things on it
/// aren't cut away, and further off it keeps more precision for the distance. Scripts can bring it right in for their
/// close shots.
namespace openblack::near_clipping
{

/// The near plane while a script asks for close clipping
inline constexpr float k_Close = 0.1f;
/// The near plane at or below the ground, and as high as it goes, 20 units over it
inline constexpr float k_Lowest = 0.3f;
inline constexpr float k_Highest = 3.5f;
inline constexpr float k_HighestAbove = 20.0f;

/// The near plane for the camera's height over the land beneath it
[[nodiscard]] constexpr float NearPlane(float heightAboveLand, bool close)
{
	if (close)
	{
		return k_Close;
	}
	if (!(heightAboveLand > 0.0f))
	{
		return k_Lowest;
	}
	if (heightAboveLand > k_HighestAbove)
	{
		return k_Highest;
	}
	return (heightAboveLand * 0.05f * 3.2f) + k_Lowest;
}

} // namespace openblack::near_clipping
