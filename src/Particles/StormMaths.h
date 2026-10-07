/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "ParticleMaths.h"

/// The formulas of the storm miracle's particles: its clouds gathering and circling, the bolts they let fly, the drift
/// with the wind, the rain storm it lays over the land and the swirl at the point it is cast. Free of state, so they are
/// tested on their own.
namespace openblack::particles::storm
{

/// The weather a storm lays over the land around a point: within the inner radius at full strength, fading out to the
/// outer, coming in over its fading time. It pulls the temperature towards its own and adds its rain, snow, cloud and
/// wind, as percentages, to the weather there.
struct RainStorm
{
	glm::vec3 centre {0.0f};
	float innerRadius {0.0f};
	float outerRadius {0.0f};
	float fadeInSeconds {0.0f};
	/// How high its clouds are, for the rain falling from them
	float cloudHeight {0.0f};
	int8_t temperature {0};
	int8_t rain {0};
	int8_t snow {0};
	int8_t overcast {0};
	int8_t windX {0};
	int8_t windZ {0};
};

// The rain storm

/// The storm's wind gets faster with its size between these magnitudes, and its speed between these, both scaled by the
/// miracle's strength
struct StormWind
{
	float minSpeed {40.0f};
	float maxSpeed {100.0f};
	float magnitudeForMinSpeed {20.0f};
	float magnitudeForMaxSpeed {100.0f};
};

/// A wind or rain value rounded into a byte of the weather: held within -128..128 first, so that 128 comes round to -128
[[nodiscard]] int8_t WeatherByte(float value);
/// How much it rains under the storm: the miracle's rain times its strength, but never below 100. Only the value's low
/// byte counts, so above 100 it may come round to no rain at all. Without a storm miracle behind it, it rains 100; with
/// rain off, not at all.
[[nodiscard]] int8_t RainByte(bool rainOn, std::optional<float> rainAmount, float strength);
/// The storm's wind along the way it was thrown: faster the bigger the storm, scaled by its strength
[[nodiscard]] glm::ivec2 WindBytes(glm::vec3 heading, float magnitude, float strength, const StormWind& wind);
/// What the storm lays over the land: from its clouds' radius (an inner radius of at least 60 and an outer one two and
/// a half times as wide, at least 20 beyond the inner and at least 80), fading in over half the clouds' forming time,
/// mild, cloudy and raining, with the wind of its throw
[[nodiscard]] RainStorm RainStormFor(glm::vec3 centre, float cloudRadius, float cloudHeight, float timeToForm, glm::ivec2 wind,
                                     int8_t rain);

// The clouds

/// The smoothstep of the clouds gathering: 0 to 1 over ten seconds of a collection's life
[[nodiscard]] float GatherProgress(float collectionAge);
/// A value that eases from where it starts to 1 as the clouds gather
[[nodiscard]] float GatherScale(float initial, float collectionAge);
/// How far from the middle a new cloud is, from a random number in 0.7..1 and the clouds' radius
[[nodiscard]] float NewCloudRadius(float random, float cloudRadius);
/// A cloud's turning speed at a fraction of its life: fastest halfway, still at its birth and its end
[[nodiscard]] float CloudAngularSpeed(float lifeFraction, float maxAngularSpeed, float direction);
/// A cloud's distance from the middle: closing in over its life, swelling and shrinking with its angle and the
/// collection's age, at the collection's gathering scale
[[nodiscard]] float CloudDistance(float lifeFraction, float radius, float collectionAge, float angle, float radiusScale);
/// How formed a cloud is over its life: growing up to its fullest at a fraction of its life, then thinning away
[[nodiscard]] float CloudEnvelope(float lifeFraction, float fractionToMaxSize);
/// A cloud's look at a fraction of its life and its envelope
struct CloudLook
{
	uint8_t grey;
	uint8_t alpha;
	/// How much wider than tall it is
	float ratio;
	/// Its scale, before the creator's own
	float scale;
};
struct CloudShades
{
	int maxColour {120};
	int minColour {90};
	int minAlpha {0};
	int maxAlpha {180};
	float minScale {0.0f};
	float maxScale {1.0f};
	float maxRatio {10.0f};
	float minRatio {1.0f};
	float fractionToMaxSize {0.413717f};
};
[[nodiscard]] CloudLook LookOf(float lifeFraction, float scaleProvider, float ratioScale, const CloudShades& shades);
/// A lit cloud's added light, 0..255 of each of red, green and blue, as it fades from white blue over its life
[[nodiscard]] glm::u8vec3 FlashLight(float secondsLit, float flashLife);
/// The size of a strike's thunder, 1 large, 2 medium or 3 small, by a random number in 0..1
[[nodiscard]] int ThunderSize(float random);
/// Seconds until a collection's next strike, from a random number in 0.5..1: sooner for a greater tribal power
[[nodiscard]] float NextStrikeWait(float random, float switchLife, std::optional<float> tribalPower);

/// The share of the gap a randomised flash of a storm held in the hand adds to a random number up to three quarters
inline constexpr float k_FlashGapLeast = 0.25f;
/// Whether a storm held in the hand flashes this step, and if so moves its schedule on: as a plain emitter, but the gap
/// between flashes varies between a quarter and all of 1 / frequency. random075 draws a random number up to 0.75, only
/// called when the gap is randomised.
template <typename Random>
[[nodiscard]] bool ShouldFlash(maths::EmitterClock& clock, const maths::EmitterLimits& limits, float collectionAge, float dt,
                               int alive, Random&& random075)
{
	if (!clock.started)
	{
		clock.started = true;
		clock.next = collectionAge;
	}
	if (limits.maxTotal != -1 && clock.emitted >= limits.maxTotal)
	{
		return false;
	}
	if (!(clock.next < collectionAge + dt))
	{
		return false;
	}
	if (limits.maxAlive >= 0 && alive > limits.maxAlive)
	{
		return false;
	}
	float gap = 1.0f / limits.frequency;
	if (limits.randomise)
	{
		gap = (random075() + k_FlashGapLeast) * gap;
	}
	++clock.emitted;
	clock.next += gap;
	return true;
}

// The drift

/// The storm's movement after a step: easing towards a multiple of the wind at the damping rate
[[nodiscard]] glm::vec3 DriftVelocity(glm::vec3 velocity, glm::vec3 wind, float magnification, float damping, float dt);

// The swirl where it is cast

/// The swirl's radius, its speed of turning at it, and its sprites' fade
struct SwirlParams
{
	float maxRadius {1.1f};
	float minRadius {0.2f};
	float thetaDotMinRadius {4.0f};
	float thetaDotMaxRadius {0.6f};
	float radiusDot {0.5f};
	float dispersalAge {2.4f};
	float fadeOutTime {2.0f};
	float accelerationStart {1.0f};
	float accelerationEnd {3.0f};
};
/// The swirl's radius after a step: shrinking to its smallest until it disperses, then growing
[[nodiscard]] float SwirlRadius(float radius, float age, float dt, const SwirlParams& params);
/// How fast the swirl turns at its radius: fast when tight
[[nodiscard]] float SwirlTurning(float radius, const SwirlParams& params);
/// The share of its full speed the swirl moves at: none until it starts, building up to all of it
[[nodiscard]] float SwirlAcceleration(float age, const SwirlParams& params);
/// The wind a storm drifts by: the wind where it is, or none for a storm a script cast, which stays where it was put
[[nodiscard]] glm::vec3 DriftWind(glm::vec3 wind, bool scriptCasting);
/// The swirl's alpha as it disperses, 0..255, none once gone
[[nodiscard]] std::optional<float> SwirlAlpha(float age, const SwirlParams& params);

// The miracle

/// Its upkeep each turn: its plain upkeep times the square of its radius over the radius that costs that much
[[nodiscard]] float CostToMaintain(float plainCost, float radius, float radiusForNormalCost);

} // namespace openblack::particles::storm
