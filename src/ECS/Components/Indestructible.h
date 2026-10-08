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

/// A script made the object indestructible: a villager never finishes drowning, a fence stays out of stores, the
/// miracles that destroy things spare it, and the hand can't give it to a store or an altar
struct Indestructible
{
};

} // namespace openblack::ecs::components
