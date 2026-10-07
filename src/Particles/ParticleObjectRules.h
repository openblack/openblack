/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

namespace openblack::particles
{
class ParticleClassRegistry;

/// The rules that tie an effect to the game objects it is given: an unseen atom kept on each, and atoms let out from
/// points of its model
void RegisterObjectRules(ParticleClassRegistry& registry);

} // namespace openblack::particles
