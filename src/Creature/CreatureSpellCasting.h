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

// A creature pays for the miracles it casts with its body. It has chants to give for its size and strength over the
// energy it has above a floor; what it gives costs it energy and tires it, by no more than seven tenths at a time, the
// energy by a share for the magic type. It can't cast a miracle that would leave it too exhausted to pay for making it.
// Pure, tested on made-up bodies.

namespace openblack::creature_spell_casting
{

/// What a creature's body is to its casting
struct Body
{
	/// Its size, 1 for a grown creature, and its strength, 0 to 1
	float size {1.0f};
	float strength {0.5f};
	float energy {1.0f};
	float exhaustion {0.0f};
};

/// The species' casting values from its tables
struct Rates
{
	/// Chants for each of its size and strength above its energy's floor
	float chantsPerEnergy {36000.0f};
	float energyFloor {0.005f};
	/// How much its size counts against its strength in turning chants into energy
	float sizeFactor {1.0f};
};

/// Paying for a miracle tires it by no more than this at a time
inline constexpr float k_MostTiredByPaying = 0.7f;
/// It can't cast once it would be this exhausted
inline constexpr float k_TooExhaustedToCast = 0.85f;

/// In a fight each miracle costs stamina at this many chants to the whole of it
inline constexpr float k_ChantsPerStamina = 10000.0f;
/// A creature tries a miracle once it has seen it this share of the times it needs to learn it
inline constexpr float k_ShareToTry = 0.5f;
/// A try short of this share of the times needed, less one, fizzles
inline constexpr float k_ShareToSucceed = 0.999f;
/// A miracle is cast this much bigger than what it is cast on
inline constexpr float k_MagnitudeOverSize = 1.1f;
/// A creature of size 1 is this tall
inline constexpr float k_HeightOfSizeOne = 15.0f;
/// A fire seed's miracle is cast at the caster's height over this, at most 1
inline constexpr float k_FireMagnitudeHeight = 10.0f;

/// The energy a number of chants comes to for the creature
[[nodiscard]] float ChantsToEnergy(float chants, const Body& body, const Rates& rates);
/// The most chants it has to give
[[nodiscard]] float MostChants(const Body& body, const Rates& rates);
/// A miracle asks it for an amount: what it gives, its energy and exhaustion paying for it (the energy by the magic
/// type's share), its energy kept between none and its size (at least 1)
float MaintainSpell(Body& body, const Rates& rates, float amount, float energyShare);
/// Whether it may cast a miracle that costs so much to make
[[nodiscard]] bool CanCast(const Body& body, const Rates& rates, float costToCreate);

/// The stamina a miracle costs in a fight, by its cost to make
[[nodiscard]] float StaminaCost(float costToCreate);
/// Whether a creature may try a miracle it has seen so often, needing to see it so often to learn it
[[nodiscard]] bool MayTry(float seen, float needed);
/// Whether its try comes off, or fizzles
[[nodiscard]] bool TrySucceeds(float seen, float needed);
/// How big the miracle is cast: by the size of what it is cast on, or the scale of a creature it is cast on, or for a
/// fire seed by the caster's height
[[nodiscard]] float CastMagnitude(float targetSize, std::optional<float> targetCreatureScale, bool fireSeed,
                                  float casterHeight);

} // namespace openblack::creature_spell_casting
