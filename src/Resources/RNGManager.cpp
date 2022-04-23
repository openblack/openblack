/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "RNGManager.h"

using namespace openblack;
using namespace openblack::resources;

RNGManager& RNGManager::GetInstance()
{
	static RNGManager rngManager;
	return rngManager;
}

int RNGManager::NextInt(int min, int max)
{
	static std::uniform_int_distribution<int> int_dist(min, max);
	if (_debug_rng)
	{
		std::lock_guard<std::mutex> safe_lock(_generator_lock);
		return int_dist(_generator);
	}
	thread_local std::mt19937 generator;
	return int_dist(generator);
}

float RNGManager::NextFloat(float min, float max)
{
	static std::uniform_real_distribution<float> float_dist(min, max);
	if (_debug_rng)
	{
		std::lock_guard<std::mutex> safe_lock(_generator_lock);
		return float_dist(_generator);
	}
	thread_local std::mt19937 generator;
	return float_dist(generator);
}

bool RNGManager::SetDebugMode(bool is_debug, int seed)
{
	std::lock_guard<std::mutex> safe_lock(_generator_lock);
	_debug_rng = true;
	_generator.seed(seed);
	return _debug_rng == is_debug;
}