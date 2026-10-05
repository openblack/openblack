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

#include <glm/vec3.hpp>

namespace openblack
{

/// The temple's light, which Temple::Draw works out each frame from the alignment of the realm the player came in from:
/// a colour the rooms and what is in them are multiplied by, and one added after (0xE05FE8 and 0xE05FE4). An evil temple
/// is lit red, with a red pulse, and a good one has a slow rainbow pulse, each the stronger the further the alignment
/// leans. A neutral temple is lit as it is.
struct TempleLight
{
	glm::vec3 multiply {1.0f};
	glm::vec3 add {0.0f};

	/// The light at an alignment, from -1, evil, to 1, good, as GetTickCount's milliseconds have it pulse every 4.096
	/// seconds
	[[nodiscard]] static TempleLight At(float alignment, uint32_t milliseconds);
	/// ApplyCitadelColoring: an object's colour, in the temple's light
	[[nodiscard]] glm::vec3 Colour(glm::vec3 colour) const;
};

} // namespace openblack
