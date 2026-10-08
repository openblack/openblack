/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TownPlaythings.h"

#include <algorithm>
#include <limits>

#include "Common/GUtilsDistance.h"

namespace openblack::ecs::town_playthings
{

bool Add(std::vector<entt::entity>& playthings, entt::entity thing,
         const std::function<bool(entt::entity)>& stillAboutOfSameKind)
{
	if (std::ranges::any_of(playthings, stillAboutOfSameKind))
	{
		return false;
	}
	playthings.insert(playthings.begin(), thing);
	return true;
}

std::optional<entt::entity> Nearest(std::span<const Candidate> candidates, glm::vec3 point)
{
	std::vector<Candidate> ordered(candidates.begin(), candidates.end());
	std::ranges::stable_sort(ordered, [](const Candidate& a, const Candidate& b) {
		return a.owner != b.owner ? static_cast<uint32_t>(a.owner) < static_cast<uint32_t>(b.owner) : a.id < b.id;
	});
	std::optional<entt::entity> nearest;
	float best = std::numeric_limits<float>::max();
	for (const auto& candidate : ordered)
	{
		const float distance = gutils::GetDistanceInMetres(point, candidate.position);
		if (distance < best)
		{
			best = distance;
			nearest = candidate.town;
		}
	}
	return nearest;
}

} // namespace openblack::ecs::town_playthings
