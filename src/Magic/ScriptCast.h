/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <glm/vec3.hpp>

#include "Particles/ParticleSpellLink.h"
#include "SpellRules.h"

// A miracle a script casts at a point, from a point. It is the neutral player's, as big as the script's radius, lasting
// the script's time, with its effect's own prayer power to start with and no limit on what it makes. Its effect starts
// from where the script casts it from, looking towards the point, still, at full strength, spun by the script's curl
// (one radian a second for each unit). Both points are taken as map positions. Pure, tested on its own.

namespace openblack::magic
{

/// The magic types a script may cast; any other is refused
inline constexpr int k_FirstScriptMagicType = 1;
inline constexpr int k_LastScriptMagicType = 41;
[[nodiscard]] constexpr bool IsScriptMagicType(int type)
{
	return type >= k_FirstScriptMagicType && type <= k_LastScriptMagicType;
}

/// Radians a second of spin for each unit of a script's curl
inline constexpr float k_ScriptCurlToSpin = 1.0f;

struct ScriptCast
{
	/// Where the miracle lands, as a map position
	glm::vec3 point {0.0f};
	SpellCastData cast;
	particles::ProcessInfo info;
};

[[nodiscard]] ScriptCast MakeScriptCast(float initialChants, glm::vec3 target, glm::vec3 from, float radius, float duration,
                                        float curl);

} // namespace openblack::magic
