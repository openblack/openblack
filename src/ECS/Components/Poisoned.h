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

namespace openblack::ecs::components
{

/// A living thing that has been poisoned, as by eating poisoned food; the heal miracle cures it
struct Poisoned
{
	char dummy {0};
};

/// When a creature last changed its mind about another creature for that creature's miracle, in game turns: it does so
/// no more often than every minute
struct CreatureMiracleOpinion
{
	uint32_t lastTurn {0};
	bool changed {false};
};

} // namespace openblack::ecs::components
