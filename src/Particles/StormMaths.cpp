/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "StormMaths.h"

#include <cmath>

#include <algorithm>
#include <numbers>

using namespace openblack::particles;

namespace
{
/// The weather's bytes hold a wind or rain within this, either way
constexpr float k_ByteLimit = 128.0f;
/// It rains at least this much under the storm
constexpr int k_LeastRain = 100;
/// The storm is mild and cloudy
constexpr int8_t k_StormTemperature = 20;
constexpr int8_t k_StormOvercast = 80;
/// The rain storm's radii from the clouds'
constexpr float k_OuterShare = 2.5f;
constexpr float k_LeastInner = 60.0f;
constexpr float k_OuterBeyondInner = 20.0f;
constexpr float k_LeastOuter = 80.0f;
/// It fades in over this share of the clouds' forming time
constexpr float k_FadeShareOfForming = 0.5f;
/// The clouds gather over ten seconds
constexpr float k_GatherRate = 0.1f;
/// A new cloud starts this much further out than its random number
constexpr float k_NewCloudBase = 0.7f;
/// A cloud's distance swells and shrinks by this share, turning with the collection's age at this rate
constexpr float k_DistanceWobble = 0.3f;
constexpr float k_DistanceWobbleRate = 0.1f;
/// Thunder is small below this random number, medium below the next, else large
constexpr float k_SmallThunder = 0.33f;
constexpr float k_MediumThunder = 0.66f;
/// The light of a flash: red and green at 200 of 256 of its strength, blue at 255
constexpr int k_FlashRedGreen = 200;
constexpr int k_FlashBlue = 255;
/// The drift eases towards a tenth of the wind times its magnification
constexpr float k_DriftShare = 0.1f;
constexpr float k_ByteMax = 255.0f;

float Clamp01(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}
} // namespace

int8_t storm::WeatherByte(float value)
{
	const float held = std::clamp(value, -k_ByteLimit, k_ByteLimit);
	// Rounded to the nearest, a half to the even number, as the game's rounding does
	return static_cast<int8_t>(static_cast<uint8_t>(static_cast<int32_t>(std::nearbyint(held)) & 0xFF));
}

int8_t storm::RainByte(bool rainOn, std::optional<float> rainAmount, float strength)
{
	if (!rainOn)
	{
		return 0;
	}
	if (!rainAmount.has_value())
	{
		return static_cast<int8_t>(k_LeastRain);
	}
	// Only the low byte of the whole number is looked at
	const auto low = static_cast<uint8_t>(static_cast<int32_t>(*rainAmount * strength) & 0xFF);
	return static_cast<int8_t>(low > k_LeastRain ? low : static_cast<uint8_t>(k_LeastRain));
}

glm::ivec2 storm::WindBytes(glm::vec3 heading, float magnitude, float strength, const StormWind& wind)
{
	const float share =
	    Clamp01((magnitude - wind.magnitudeForMinSpeed) / (wind.magnitudeForMaxSpeed - wind.magnitudeForMinSpeed));
	const float slowest = strength * wind.minSpeed;
	const float speed = (strength * wind.maxSpeed - slowest) * share + slowest;
	return {WeatherByte(heading.x * speed), WeatherByte(heading.z * speed)};
}

storm::RainStorm storm::RainStormFor(glm::vec3 centre, float cloudRadius, float cloudHeight, float timeToForm, glm::ivec2 wind,
                                     int8_t rain)
{
	float inner = cloudRadius;
	if (inner <= k_LeastInner)
	{
		inner = k_LeastInner;
	}
	float outer = std::max(cloudRadius * k_OuterShare, inner + k_OuterBeyondInner);
	if (outer <= k_LeastOuter)
	{
		outer = k_LeastOuter;
	}
	return {
	    .centre = centre,
	    .innerRadius = inner,
	    .outerRadius = outer,
	    .fadeInSeconds = timeToForm * k_FadeShareOfForming,
	    .cloudHeight = cloudHeight,
	    .temperature = k_StormTemperature,
	    .rain = rain,
	    .snow = 0,
	    .overcast = k_StormOvercast,
	    .windX = static_cast<int8_t>(wind.x),
	    .windZ = static_cast<int8_t>(wind.y),
	};
}

float storm::GatherProgress(float collectionAge)
{
	const float t = Clamp01(collectionAge * k_GatherRate);
	return (3.0f - (t + t)) * t * t;
}

float storm::GatherScale(float initial, float collectionAge)
{
	return (1.0f - initial) * GatherProgress(collectionAge) + initial;
}

float storm::NewCloudRadius(float random, float cloudRadius)
{
	return (random + k_NewCloudBase) * cloudRadius;
}

float storm::CloudAngularSpeed(float lifeFraction, float maxAngularSpeed, float direction)
{
	const float centred = (lifeFraction + lifeFraction) - 1.0f;
	return (1.0f - centred * centred) * direction * maxAngularSpeed;
}

float storm::CloudDistance(float lifeFraction, float radius, float collectionAge, float angle, float radiusScale)
{
	const float wobble = std::cos(collectionAge * k_DistanceWobbleRate + angle) * k_DistanceWobble + 1.0f;
	return (1.0f - lifeFraction) * radius * wobble * radiusScale;
}

float storm::CloudEnvelope(float lifeFraction, float fractionToMaxSize)
{
	if (lifeFraction < fractionToMaxSize)
	{
		return lifeFraction / fractionToMaxSize;
	}
	return 1.0f - (lifeFraction - fractionToMaxSize) / (1.0f - fractionToMaxSize);
}

storm::CloudLook storm::LookOf(float lifeFraction, float scaleProvider, float ratioScale, const CloudShades& shades)
{
	const float g = CloudEnvelope(lifeFraction, shades.fractionToMaxSize);
	const auto grey =
	    static_cast<float>(shades.maxColour) + static_cast<float>(shades.minColour - shades.maxColour) * lifeFraction;
	const auto alpha = static_cast<float>(shades.minAlpha) + static_cast<float>(shades.maxAlpha - shades.minAlpha) * g;
	return {
	    .grey = static_cast<uint8_t>(static_cast<int32_t>(grey)),
	    .alpha = static_cast<uint8_t>(static_cast<int32_t>(alpha)),
	    .ratio = (shades.maxRatio + (shades.minRatio - shades.maxRatio) * lifeFraction) * ratioScale,
	    .scale = (shades.minScale + (shades.maxScale - shades.minScale) * g) * scaleProvider,
	};
}

glm::u8vec3 storm::FlashLight(float secondsLit, float flashLife)
{
	if (!(flashLife > 0.0f) || secondsLit > flashLife)
	{
		return glm::u8vec3(0);
	}
	const auto i = static_cast<int>(static_cast<uint8_t>(static_cast<int32_t>((1.0f - secondsLit / flashLife) * k_ByteMax)));
	const auto redGreen = static_cast<uint8_t>((i * k_FlashRedGreen) >> 8);
	const auto blue = static_cast<uint8_t>((i * k_FlashBlue) >> 8);
	return {redGreen, redGreen, blue};
}

int storm::ThunderSize(float random)
{
	if (random < k_SmallThunder)
	{
		return 3;
	}
	return random < k_MediumThunder ? 2 : 1;
}

float storm::NextStrikeWait(float random, float switchLife, std::optional<float> tribalPower)
{
	const float wait = random * switchLife;
	if (!tribalPower.has_value())
	{
		return wait;
	}
	return wait / std::max(*tribalPower, 1.0f);
}

glm::vec3 storm::DriftVelocity(glm::vec3 velocity, glm::vec3 wind, float magnification, float damping, float dt)
{
	const glm::vec3 target = wind * magnification * k_DriftShare;
	return velocity + (target - velocity) * damping * dt;
}

float storm::SwirlRadius(float radius, float age, float dt, const SwirlParams& params)
{
	if (age >= params.dispersalAge)
	{
		return radius + dt * params.radiusDot;
	}
	return std::max(radius - dt * params.radiusDot, params.minRadius);
}

float storm::SwirlTurning(float radius, const SwirlParams& params)
{
	const float t = Clamp01((radius - params.minRadius) / (params.maxRadius - params.minRadius));
	return (params.thetaDotMaxRadius - params.thetaDotMinRadius) * t + params.thetaDotMinRadius;
}

float storm::SwirlAcceleration(float age, const SwirlParams& params)
{
	return Clamp01((age - params.accelerationStart) / (params.accelerationEnd - params.accelerationStart));
}

glm::vec3 storm::DriftWind(glm::vec3 wind, bool scriptCasting)
{
	return scriptCasting ? glm::vec3(0.0f) : wind;
}

std::optional<float> storm::SwirlAlpha(float age, const SwirlParams& params)
{
	if (age < params.dispersalAge)
	{
		return k_ByteMax;
	}
	const float dispersing = age - params.dispersalAge;
	if (dispersing > params.fadeOutTime)
	{
		return std::nullopt;
	}
	// The whole byte left of the share faded, truncated
	return std::trunc(k_ByteMax - ((dispersing / params.fadeOutTime) * k_ByteMax));
}

float storm::CostToMaintain(float plainCost, float radius, float radiusForNormalCost)
{
	if (!(radiusForNormalCost > 0.0f))
	{
		return plainCost;
	}
	const float share = radius / radiusForNormalCost;
	return plainCost * share * share;
}
