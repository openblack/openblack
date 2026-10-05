/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "GameRandom.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{

/// The game's random state (its two seeds, the particles' stream and the game thread's C runtime seed). It has no lock, as
/// in the game: only the game thread draws
class GameRandomProduction final: public GameRandomInterface
{
public:
	uint32_t GameRand(uint32_t n) override;
	float GameFloatRand(float x) override;
	uint32_t LocalRand(int32_t n) override;
	float LocalFloatRand(float x) override;
	int32_t CrtRand() override;
	void CrtSrand(uint32_t seed) override;

	[[nodiscard]] GameRandomSeeds GetSeeds() const override { return _seeds; }
	void SetSeeds(GameRandomSeeds seeds) override { _seeds = seeds; }

	[[nodiscard]] ParticleRandomStream GetParticleStream() const override { return _particleStream; }
	void SetParticleStream(ParticleRandomStream stream) override { _particleStream = stream; }

private:
	GameRandomSeeds _seeds;
	ParticleRandomStream _particleStream {ParticleRandomStream::None};
	uint32_t _crtSeed {1};
};

} // namespace openblack
