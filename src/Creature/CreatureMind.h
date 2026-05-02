/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>

#include "Enums.h"

namespace openblack::v120
{
struct InfoConstants;
}

namespace openblack
{
using InfoConstants = v120::InfoConstants;
}

namespace openblack::creature
{

static constexpr size_t k_DesireCount = static_cast<size_t>(CreatureDesires::Invalid);

struct CreatureMind
{
	// Species-specific values from InfoConstants::creatureDesireForType, which is parsed from info.dat.
	// Unsupported cognition behavior intentionally has no synthesized fallback values here.
	std::array<float, k_DesireCount> baseDesireWeights {};

	CreatureMind() = default;
	CreatureMind(const InfoConstants& info, CreatureType creatureType);
};

} // namespace openblack::creature
