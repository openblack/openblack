/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>

#include <string_view>

#include "Enums.h"

namespace openblack::particles
{

constexpr size_t k_ParticleTypeCount = 150;

/// The particle file a particle type plays, without its extension (such as "SF_Smoke"); empty for the types that have
/// none, which other code draws or nothing does
[[nodiscard]] std::string_view ParticleTypeFile(ParticleType type);
/// A readable name for a particle type, such as "Heal In Hand"
[[nodiscard]] std::string_view ParticleTypeName(ParticleType type);

} // namespace openblack::particles
