/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::ecs::components
{

/// Drawn added over what is behind it, among the other things that blend, whatever its model's materials say: a
/// one-shot miracle's bubble
struct Translucent
{
	/// The share of its colour added, 0 to 1
	float share {1.0f};
};

} // namespace openblack::ecs::components
