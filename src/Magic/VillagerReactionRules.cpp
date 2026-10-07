/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "VillagerReactionRules.h"

#include <cmath>

#include <algorithm>
#include <iterator>

using namespace openblack::magic;

namespace
{
constexpr float k_MaxPriority = 255.0f;
} // namespace

float villager_reaction::SpreadDistance(glm::vec2 villager, glm::vec2 reaction)
{
	return 0.5f * (std::abs(villager.x - reaction.x) + std::abs(villager.y - reaction.y));
}

uint8_t villager_reaction::Priority(uint8_t kindPriority, bool kindReacts, const Distance& distance, float at)
{
	if (!kindReacts || at > distance.maxDistance || !(distance.maxDistance > 0.0f))
	{
		return 0;
	}
	const float nearness = (distance.maxDistance - at) / distance.maxDistance;
	const float priority =
	    std::min((1.0f + 0.5f * distance.importance * nearness) * static_cast<float>(kindPriority), k_MaxPriority);
	return static_cast<uint8_t>(static_cast<int32_t>(priority));
}

bool villager_reaction::Memory::MayReactAgain(uint32_t type, uint32_t turn, uint32_t cooldown)
{
	for (auto record = _records.begin(); record != _records.end();)
	{
		if (record->type == type)
		{
			if (turn - record->turn > cooldown)
			{
				record->turn = turn;
				return true;
			}
			return false;
		}
		record = turn - record->turn > k_ForgetAfter ? _records.erase(record) : std::next(record);
	}
	Remember(type, turn);
	return true;
}

void villager_reaction::Memory::Record(uint32_t type, uint32_t turn)
{
	const auto found = std::ranges::find(_records, type, &Entry::type);
	if (found != _records.end())
	{
		found->turn = turn;
		return;
	}
	Remember(type, turn);
}

void villager_reaction::Memory::Remember(uint32_t type, uint32_t turn)
{
	if (_records.size() >= k_Records)
	{
		_records.erase(_records.begin());
	}
	_records.push_back({.type = type, .turn = turn});
}

uint32_t villager_reaction::Memory::LastReacted(uint32_t type) const
{
	const auto found = std::ranges::find(_records, type, &Entry::type);
	return found != _records.end() ? found->turn : 0;
}
