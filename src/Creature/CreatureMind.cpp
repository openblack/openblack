/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMind.h"

#include "InfoConstants.h"

namespace openblack::creature
{

static_assert(std::tuple_size_v<decltype(InfoConstants::creatureDesireForType)> == k_DesireCount);

CreatureMind::CreatureMind(const InfoConstants& info, CreatureType creatureType)
{
	if (creatureType <= CreatureType::Unknown || creatureType >= CreatureType::_COUNT)
	{
		return;
	}

	const size_t creatureTypeIndex = static_cast<size_t>(creatureType) - 1;
	for (size_t desireIndex = 0; desireIndex < baseDesireWeights.size(); ++desireIndex)
	{
		baseDesireWeights.at(desireIndex) = info.creatureDesireForType.at(desireIndex).byCreatureType.at(creatureTypeIndex);
	}
}

} // namespace openblack::creature
