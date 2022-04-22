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
using namespace openblack::ressources;

RNGManager& RNGManager::GetInstance()
{
	static RNGManager rngManager;
	return rngManager;
}

uint16_t RNGManager::nextInt()
{
	std::uniform_int_distribution<uint16_t> int_dist(1,500);
	std::lock_guard<std::mutex> safe_lock(_generator_lock);
	return int_dist(_generator);
}

float RNGManager::nextFloat()
{
	std::uniform_real_distribution<float> float_dist(0.0, 1.0);
	std::lock_guard<std::mutex> safe_lock(_generator_lock);
	return float_dist(_generator);
}

void RNGManager::setSeed(unsigned int seed)
{
	std::lock_guard<std::mutex> safe_lock(_generator_lock);
	_generator.seed(seed);
}