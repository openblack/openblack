/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TornadoMaths.h"

#include <cmath>

#include <algorithm>
#include <numbers>

using namespace openblack::particles;

namespace
{
/// The weight of the faster noise in the foot's wander
constexpr float k_FastNoiseWeight = 0.5f;
/// The rates of the noise the foot's wander reads, against the wander's own
constexpr float k_SecondRate = 1.3f;
constexpr float k_ThirdRate = 2.0f;
constexpr float k_FourthRate = 2.6f;
/// Something is pulled to the wall at half its distance from it a second
constexpr float k_WallPull = -0.5f;
/// Spin outside the wall falls with the distance, kept finite on the axis
constexpr float k_SpinDistanceGuard = 0.1f;
/// The tornado's tribal power is held within these
constexpr float k_LeastTribalPower = 1.0f;
constexpr float k_MostTribalPower = 5.0f;
/// The smallest share of a pot's size a small tornado makes it
constexpr float k_SmallestPotShare = 0.2f;

float Share(float h)
{
	return std::clamp(h, 0.0f, 1.0f);
}
} // namespace

float tornado::Bias(float t, float b)
{
	if (!(t > 0.0f))
	{
		return 0.0f;
	}
	return std::pow(t, std::log(b) / std::log(0.5f));
}

float tornado::Gain(float t, float g)
{
	const float exponent = std::log(1.0f - g) / std::log(0.5f);
	if (t < 0.5f)
	{
		const float x = 2.0f * t;
		return x > 0.0f ? std::pow(x, exponent) * 0.5f : 0.0f;
	}
	const float x = 2.0f - 2.0f * t;
	return 1.0f - (x > 0.0f ? std::pow(x, exponent) * 0.5f : 0.0f);
}

float tornado::WallRadius(const Funnel& funnel, float h)
{
	const float t = Share(h);
	return (funnel.baseRadius + (funnel.topRadius - funnel.baseRadius) * t * t) * funnel.scale;
}

float tornado::WallScale(const Funnel& funnel, float h)
{
	const float t = Share(h);
	return (funnel.baseScale + (funnel.topScale - funnel.baseScale) * t * t) * funnel.scale;
}

glm::vec3 tornado::AxisCentre(const Funnel& funnel, glm::vec3 base, glm::vec3 top, float h)
{
	const float t = Share(h);
	const float bent = Gain(t, funnel.bend);
	glm::vec3 centre {base.x + (top.x - base.x) * bent, base.y + (top.y - base.y) * t, base.z + (top.z - base.z) * bent};
	// The wiggle follows the bend: its angle goes with the bent share of the height
	const float angle = bent * static_cast<float>(funnel.wiggleCount) * std::numbers::pi_v<float>;
	const float amplitude = funnel.scale * funnel.wiggleAmplitude;
	centre.x += std::cos(angle) * amplitude;
	centre.z += std::sin(angle) * amplitude;
	return centre;
}

float tornado::SpinRate(const Funnel& funnel, float h, float orbitFactor)
{
	const float bias = std::clamp(funnel.spinBias, 0.0f, 1.0f);
	return (funnel.baseSpin + (funnel.topSpin - funnel.baseSpin) * Bias(Share(h), bias)) * orbitFactor * funnel.spinMultiplier;
}

float tornado::SpinOutside(float spin, float wallRadius, float distance)
{
	return distance > wallRadius ? wallRadius / (distance + k_SpinDistanceGuard) * spin : spin;
}

float tornado::RadialRate(float wallRadius, float distance, float dt)
{
	const float first = (distance - wallRadius) * k_WallPull;
	const float second = ((first * dt + distance) - wallRadius) * k_WallPull;
	return (first + second) * 0.5f;
}

float tornado::RiseSpeed(float y, float baseY, float topY, float target, float spring)
{
	return -(y - (baseY + (topY - baseY) * target)) * spring;
}

float tornado::HeightShare(float y, float baseY, float topY)
{
	const float height = topY - baseY;
	if (height == 0.0f)
	{
		return 0.0f;
	}
	return Share((y - baseY) / height);
}

glm::vec2 tornado::FootWander(float n1, float n2, float n3, float n4, float amplitude, float scale)
{
	const float reach = scale * amplitude;
	return {(n3 * k_FastNoiseWeight + n1) * reach, (n4 * k_FastNoiseWeight + n2) * reach};
}

tornado::WanderTimes tornado::FootWanderTimes(float age, float frequency)
{
	const float t = age * frequency;
	return {.a = t, .b = t * k_SecondRate, .c = t * k_ThirdRate, .d = t * k_FourthRate};
}

float tornado::CloseFade(float sinceClose, float fadeOut)
{
	if (sinceClose < 0.0f)
	{
		return 1.0f;
	}
	if (sinceClose <= fadeOut)
	{
		return 1.0f - sinceClose / fadeOut;
	}
	return 0.0f;
}

uint8_t tornado::FadeAlpha(float age, float fadeIn, float closeFade)
{
	const float in = fadeIn > 0.0f ? Share(age / fadeIn) : 1.0f;
	const float fade = Share(std::min(in, closeFade));
	return static_cast<uint8_t>(static_cast<int>(fade * 255.0f));
}

glm::vec3 tornado::Launch(float fromVertical, float heading, float speedShare, float speed, float scale, glm::vec3 footVelocity)
{
	const float along = speedShare * speed;
	const float across = std::sin(fromVertical) * along;
	return {scale * std::cos(heading) * across + footVelocity.x, std::cos(fromVertical) * along * scale + footVelocity.y,
	        scale * std::sin(heading) * across + footVelocity.z};
}

float tornado::Reach(const Funnel& funnel, float tribalPower)
{
	return (WallRadius(funnel, 0.0f) + WallRadius(funnel, 1.0f)) *
	       std::clamp(tribalPower, k_LeastTribalPower, k_MostTribalPower);
}

bool tornado::Fits(const Funnel& funnel, float radius)
{
	return radius < 2.0f * WallRadius(funnel, 0.0f) && radius < WallRadius(funnel, 1.0f);
}

float tornado::PileTake(float scale, float least, float most)
{
	return least + (most - least) * Share(scale);
}

float tornado::PotScale(float scale, float random)
{
	return random * std::clamp(scale, k_SmallestPotShare, 1.0f);
}
