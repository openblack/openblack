/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GameRandom.h"

#include <cmath>

using namespace openblack;

namespace
{
/// The game compares with 0 in a way that a NaN also passes
bool ZeroOrNaN(float x) noexcept
{
	return x == 0.0f || std::isnan(x);
}

/// Particles' (u * k) * x, the other way round from the game's (u * x) * k
float ParticleFloat(uint32_t u, float x) noexcept
{
	float r = static_cast<float>(u);
	r = r * game_random::k_FloatRandScale;
	r = r * x;
	return r;
}
} // namespace

float game_random::FloatRand(float x, uint32_t& seed) noexcept
{
	if (ZeroOrNaN(x))
	{
		return 0.0f;
	}
	float r = static_cast<float>(LHRand(k_FloatRandRange, seed));
	r = r * x;
	r = r * k_FloatRandScale;
	return r;
}

float GameRandomInterface::GameFloatRange(float a, float b)
{
	const float range = b - a;
	const float r = GameFloatRand(range);
	return r + a;
}

float GameRandomInterface::CrtRandom(float a, float b)
{
	float r = static_cast<float>(CrtRand());
	r = r * game_random::k_CrtRandomScale;
	const float range = b - a;
	r = r * range;
	return r + a;
}

float GameRandomInterface::ParticleFloatRand(float x)
{
	switch (GetParticleStream())
	{
	case ParticleRandomStream::Synced:
		return ParticleFloat(GameRand(game_random::k_FloatRandRange), x);
	case ParticleRandomStream::Local:
		return ParticleFloat(LocalRand(static_cast<int32_t>(game_random::k_FloatRandRange)), x);
	case ParticleRandomStream::None:
		break;
	}
	return 0.0f;
}

float GameRandomInterface::ParticleFloatRand(float a, float b)
{
	const float range = b - a;
	const float r = ParticleFloatRand(range);
	return r + a;
}

int32_t GameRandomInterface::ParticleRand(int32_t n)
{
	switch (GetParticleStream())
	{
	case ParticleRandomStream::Synced:
		return static_cast<int32_t>(GameRand(static_cast<uint32_t>(n)));
	case ParticleRandomStream::Local:
		return static_cast<int32_t>(LocalRand(n));
	case ParticleRandomStream::None:
		break;
	}
	return 0;
}

glm::vec3 GameRandomInterface::ParticleRandR3()
{
	if (GetParticleStream() == ParticleRandomStream::None)
	{
		return glm::vec3(-1.0f);
	}
	glm::vec3 p;
	float sum = 0.0f;
	do
	{
		p.x = ParticleFloatRand(2.0f) - 1.0f;
		p.y = ParticleFloatRand(2.0f) - 1.0f;
		p.z = ParticleFloatRand(2.0f) - 1.0f;
		// (z z + y y) + x x
		const float zz = p.z * p.z;
		const float yy = p.y * p.y;
		const float xx = p.x * p.x;
		sum = zz + yy;
		sum = sum + xx;
	} while (sum > 1.0f);
	return p;
}

ParticleRandomStep::ParticleRandomStep(GameRandomInterface& random, bool synced) noexcept
    : _random(random)
{
	_random.SetParticleStream(synced ? ParticleRandomStream::Synced : ParticleRandomStream::Local);
}

ParticleRandomStep::~ParticleRandomStep()
{
	_random.SetParticleStream(ParticleRandomStream::None);
}
