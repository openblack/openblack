/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <atomic>
#include <mutex>
#include <random>

namespace openblack::resources
{
class RNGManager
{
public:
	static RNGManager& GetInstance();
	int NextInt(int min = 1, int max = 500);
	float NextFloat(float min = 0.0, float max = 1.0);
	bool SetDebugMode(bool is_debug, int seed);

private:
	std::mt19937 _generator;
	std::mutex _generator_lock;
	bool _debug_rng = false;
	RNGManager() {};
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;
};
} // namespace openblack::resources
