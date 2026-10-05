/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <glm/vec2.hpp>

namespace openblack::ecs::components
{

/// How a temple's outside looks: its player's alignment and share of influence, each from 0 to 1, and where each is
/// heading. Its mesh is blended for them as they change.
struct TempleExterior
{
	float alignment {0.5f};
	float alignmentTarget {0.5f};
	float size {0.0f};
	float sizeTarget {0.0f};
	/// The size and alignment its mesh was last blended for, none before it first is
	std::optional<glm::vec2> morphed;
};

} // namespace openblack::ecs::components
