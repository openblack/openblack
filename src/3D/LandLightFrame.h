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

namespace openblack
{

/// What the land's light of a frame is built from (see LandLightTable::Build)
struct LandLightInputs
{
	/// From 0 at night to 2 by day
	float skyType {2.0f};
	/// From -1, evil, to 1, good, as the sky shows it
	float alignment {0.0f};
	/// The clouds over the camera, 0 to 1, which darken the land
	float overcast {0.0f};
	/// A flash of lightning at the camera, 0 to 255
	uint8_t flash {0};
};

/// The land light's inputs of this frame, from the sky, the players' alignment and the weather at the camera
[[nodiscard]] LandLightInputs FrameLandLightInputs();

/// The land's light of this frame at a level of luminosity, 0xRRGGBB: what things made now and coloured by the light,
/// such as a splash on the water, keep. White without the light's palette.
[[nodiscard]] uint32_t FrameLandLight(uint8_t level);

} // namespace openblack
