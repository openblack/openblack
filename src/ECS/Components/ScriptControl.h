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

/// A script holds a reference to the object
struct InScript
{
};

/// A script controls the object: it was made by a script, or a script moved, attached or set the state of it. Its own
/// player's blows don't hurt a creature under a script's control, and a villager under one returns to what the script
/// had it doing after it lands
struct ScriptControlled
{
};

} // namespace openblack::ecs::components
