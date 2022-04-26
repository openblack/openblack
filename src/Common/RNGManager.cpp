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

RNGManager::RNGManager()
{
	_debugRng = false;
}

RNGManager& RNGManager::instance()
{
	static RNGManager rngManager;
	return rngManager;
}

uint32_t RNGManager::NextUint32(uint32_t min, uint32_t max)
{
	std::uniform_int_distribution<uint32_t> int_dist(min, max);
	if (_debugRng)
	{
		std::lock_guard<std::mutex> safe_lock(_generatorLock);
		return int_dist(_generator);
	}
	thread_local std::mt19937 generator;
	return int_dist(generator);
}

float RNGManager::NextFloat(float min, float max)
{
	std::uniform_real_distribution<float> float_dist(min, max);
	if (_debugRng)
	{
		std::lock_guard<std::mutex> safe_lock(_generatorLock);
		return float_dist(_generator);
	}
	thread_local std::mt19937 generator;
	return float_dist(generator);
}

bool RNGManager::SetDebugMode(bool isDebug, int seed)
{
	std::lock_guard<std::mutex> safe_lock(_generatorLock);
	_debugRng = true;
	_generator.seed(seed);
	return _debugRng == isDebug;
}
