/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "GameRandomProduction.h"

using namespace openblack;

uint32_t GameRandomProduction::GameRand(uint32_t n)
{
	return n == 0 ? 0 : game_random::LHRand(n, _seeds.synced);
}

float GameRandomProduction::GameFloatRand(float x)
{
	return game_random::FloatRand(x, _seeds.synced);
}

uint32_t GameRandomProduction::LocalRand(int32_t n)
{
	return n == 0 ? 0 : game_random::LHRand(static_cast<uint32_t>(n), _seeds.local);
}

float GameRandomProduction::LocalFloatRand(float x)
{
	return game_random::FloatRand(x, _seeds.local);
}

int32_t GameRandomProduction::CrtRand()
{
	return game_random::CrtRand(_crtSeed);
}

void GameRandomProduction::CrtSrand(uint32_t seed)
{
	_crtSeed = seed;
}
