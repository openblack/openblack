/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <string_view>

#include "ParticleEffect.h"

/// The sounds particles keep going: a particle starts one, which plays as long as the particle keeps it, and stops it
/// again or lets it go when it goes. The world plays them.
namespace openblack::particles
{

/// Starts a sound on a particle, the newest of its sounds; nothing for a silent one
ParticleSoundLink* StartAtomSound(Effect& effect, Atom& atom, const ParticleSound& sound);
/// The newest of a particle's sounds of an action, if any
[[nodiscard]] ParticleSoundLink* FindAtomSound(const Atom& atom, std::string_view action);
/// The particle lets go of one of its sounds, which then fades by the step each turn when it has one
void StopAtomSound(Atom& atom, const ParticleSoundLink& sound, int fadeStep = 0);
/// The particle lets go of all its sounds
void StopAllAtomSounds(Atom& atom);

/// A thrown particle's sound is large above this share of the fastest throw, medium above the next, else small
inline constexpr float k_LargeThrowShare = 0.6f;
inline constexpr float k_MediumThrowShare = 0.3f;
/// The size of a thrown particle's sound by its share of the fastest throw: 1 large, 2 medium, 3 small
[[nodiscard]] int SoundSizeFromThrow(float share);
/// The size of a landing particle's sound by how fast it struck, against the speeds of a medium and a large impact
[[nodiscard]] int SoundSizeFromImpactSpeed(float speed, float medium, float large);

} // namespace openblack::particles
