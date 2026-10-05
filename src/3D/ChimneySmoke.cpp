/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ChimneySmoke.h"

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include "3D/Mists.h"

namespace openblack::chimney_smoke
{

using ecs::components::ChimneySmoke;

namespace
{
/// A puff's life, in steps of 255 a second
constexpr int32_t k_Life = 900;
/// The puffs start a ninth of a life apart
constexpr int32_t k_StartSpacing = 90;
constexpr float k_SpinPerSecond = 0.765f;
constexpr float k_RisePerSecond = 2.55f;
/// How strongly the drift pushes the puffs
constexpr float k_DriftGain = 1.5f;
/// The random breath across the ground, either way
constexpr float k_Breath = 3.0f;
/// The hand stirs the smoke within 15 units of the chimney, moving faster than 1
constexpr float k_HandReachSquared = 225.0f;
constexpr float k_HandStirs = 1.0f;
/// The hand's speed eases towards its motion this much each turn
constexpr float k_HandEase = 0.6f;
/// A frame's time is cut to this many seconds
constexpr float k_LongestFrame = 100.0f;
/// The puffs' opacity, of 255, until a quarter of their life, then fading to none
constexpr int32_t k_Opacity = 79;
constexpr int32_t k_FadeStart = 225;
} // namespace

ChimneySmoke Create(const glm::vec3& chimney, uint32_t rgb, const Random& random)
{
	ChimneySmoke smoke;
	smoke.chimney = chimney;
	smoke.rgb = rgb;
	for (size_t i = 0; i < ChimneySmoke::k_Puffs; ++i)
	{
		auto& puff = smoke.puffs.at(i);
		puff.position = chimney + glm::vec3(0.0f, static_cast<float>(i) * 0.5f, 0.0f);
		puff.hidden = true;
		puff.age = static_cast<int32_t>(i) * k_StartSpacing;
		puff.angle = random(0.0f, glm::pi<float>());
		puff.clockwise = (static_cast<int32_t>(random(1.0f, 100.0f)) & 1) != 0;
	}
	return smoke;
}

glm::vec3 EaseHandVelocity(const glm::vec3& velocity, const glm::vec3& moved, float millisecondsPerTurn)
{
	const float perSecond = 1000.0f / millisecondsPerTurn;
	return velocity + ((moved * perSecond - velocity) * k_HandEase);
}

HandWind WindOf(const glm::vec3& position, const glm::vec3& velocity)
{
	HandWind hand {.position = position, .wind = velocity};
	const float length = glm::length(velocity);
	hand.speed = std::clamp(length * 0.1f, 0.5f, 5.0f);
	if (velocity != glm::vec3(0.0f))
	{
		hand.wind = velocity * (hand.speed / length);
	}
	return hand;
}

glm::vec3 Drift(const glm::vec3& chimney, const HandWind& hand, const Random& random)
{
	const auto toHand = hand.position - chimney;
	if (glm::dot(toHand, toHand) < k_HandReachSquared && hand.speed > k_HandStirs)
	{
		return hand.wind;
	}
	// Across the ground, z drawn first
	const float z = random(-k_Breath, k_Breath);
	const float x = random(-k_Breath, k_Breath);
	return {x, 0.0f, z};
}

bool UpdateState(ChimneySmoke& smoke, bool lit)
{
	if (lit)
	{
		smoke.state = ChimneySmoke::State::Smoking;
	}
	else if (smoke.state == ChimneySmoke::State::Smoking)
	{
		smoke.state = ChimneySmoke::State::Dying;
	}
	return smoke.state != ChimneySmoke::State::Out;
}

void Advance(ChimneySmoke& smoke, float milliseconds, const glm::vec3& drift)
{
	const float seconds = std::min(milliseconds * 0.001f, k_LongestFrame);
	const float spin = seconds * k_SpinPerSecond;
	const bool dying = smoke.state == ChimneySmoke::State::Dying;
	// The ages move on at 255 a second, the part of a step left over kept for the next frame so that the smoke keeps
	// its pace however fast the frames come
	smoke.ageRemainder += seconds * 255.0f;
	const auto ageStep = static_cast<int32_t>(smoke.ageRemainder);
	smoke.ageRemainder -= static_cast<float>(ageStep);

	size_t shown = 0;
	for (auto& puff : smoke.puffs)
	{
		puff.age += ageStep;
		puff.angle += puff.clockwise ? spin : -spin;
		float time = seconds;
		if (puff.age > k_Life)
		{
			// It starts again at the chimney, with what is left of its age, unseen if the smoke is dying
			puff.position = smoke.chimney;
			puff.age %= k_Life;
			puff.hidden = dying;
			puff.velocity = glm::vec3(0.0f);
			time = static_cast<float>(puff.age) * (1.0f / 255.0f);
		}
		// Pushed by the drift, its speed and place moved on together, then risen
		const auto next = puff.velocity + (k_DriftGain * time * drift);
		puff.position += (puff.velocity + next) * (time * 0.5f);
		puff.velocity = next;
		puff.position.y += k_RisePerSecond * time;
		if (!puff.hidden)
		{
			++shown;
		}
	}
	if (dying && shown == 0)
	{
		smoke.state = ChimneySmoke::State::Out;
	}
}

Look LookOf(int32_t age, uint32_t rgb)
{
	const int32_t opacity =
	    age > k_FadeStart ? ((k_FadeStart - age) * k_Opacity / (k_Life - k_FadeStart)) + k_Opacity : k_Opacity;
	return {
	    .frame = mists::Frame(age),
	    .halfWidth = std::max(0.0001f, (static_cast<float>(age) * (1.0f / 450.0f)) + 0.5f),
	    .argb = (static_cast<uint32_t>(opacity) << 24u) | (rgb & 0xFFFFFFu),
	};
}

} // namespace openblack::chimney_smoke
