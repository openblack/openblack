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

#include "Magic/ResourcePiles.h"

namespace openblack::ecs::components
{

/// A pile of food or wood, which rises out of the ground or sinks into it as what it holds changes. It is drawn that far
/// under its place, and not at all once wholly under the ground.
struct ResourcePile
{
	magic::piles::Rise rise;
	/// Its model's height as it is drawn, found once its model is loaded
	float height {0.0f};
	/// What it held when it last began to rise or sink to show it
	uint32_t shownAmount {0};
	/// It has begun to rise from under the ground, where it was made
	bool risen {false};
	/// Its food makes the people who take from it go faster, which sparkles over it
	bool speedUp {false};
	uint32_t speedUpVisual {0};
};

} // namespace openblack::ecs::components
