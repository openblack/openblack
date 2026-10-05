/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureCaveTargets.h"

#include <cmath>

std::optional<openblack::CreatureCaveTargets::Target>
openblack::CreatureCaveTargets::TargetAt(const std::array<std::optional<glm::vec2>, k_Count>& places, glm::vec2 mouse,
                                         float screenHeight)
{
	const float scale = screenHeight / k_ReachScreenHeight;
	std::optional<Target> found;
	for (size_t i = 0; i < k_Count; ++i)
	{
		if (!places.at(i).has_value())
		{
			continue;
		}
		// The game projects the targets to whole pixels
		auto place = glm::ivec2(*places.at(i));
		if (static_cast<Target>(i) == Target::Creature)
		{
			place.y += static_cast<int32_t>(static_cast<float>(k_CreatureDrop) * scale);
		}
		const auto pointer = glm::ivec2(std::floor(mouse.x), std::floor(mouse.y));
		const auto reach = glm::ivec2(glm::vec2(k_Reach.at(i)) * scale);
		if (pointer.x >= place.x - reach.x && pointer.x <= place.x + reach.x && pointer.y >= place.y - reach.y &&
		    pointer.y <= place.y + reach.y)
		{
			found = static_cast<Target>(i);
		}
	}
	return found;
}
