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

	// Vanilla's 17-float table is indexed by its original CreatureType enum. Its Ape value is
	// column 0, while Cow through Gorilla retain their enum values 1 through 16. openblack
	// represents that Ape entry as GiantApe.
	const size_t vanillaCreatureTypeIndex = creatureType == CreatureType::GiantApe ? 0 : static_cast<size_t>(creatureType);
	for (size_t desireIndex = 0; desireIndex < desireIncreaseTimes.size(); ++desireIndex)
	{
		desireIncreaseTimes.at(desireIndex) =
		    info.creatureDesireForType.at(desireIndex).byCreatureType.at(vanillaCreatureTypeIndex);
	}
}

} // namespace openblack::creature
