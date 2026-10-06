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

#include "ParticleEffect.h"

/// What the shields do to other effects' particles: a particle that crosses into a shield strikes it, and unless its
/// miracle gets through, it is put back on the shield's surface and bounces off
namespace openblack::particles
{

/// How much bigger than a particle's own scale a shield counts it, for crossing into it
inline constexpr float k_ShieldParticleMargin = 1.25f;

/// A particle moved from a point into a live shield: the shield is told where it was struck, the particle's miracle is
/// sent the strike and, unless the miracle got through, the particle bounces off. Whether it bounced.
bool DeflectOffShields(Effect& effect, Atom& atom, const glm::vec3& previousGlobal);

/// The shield's sparks: it was struck at a point
void StrikeShield(ShieldSphere& shield, const glm::vec3& point);

} // namespace openblack::particles
