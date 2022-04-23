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

namespace openblack::ressources
{
class RNGManager
{
public:
	static RNGManager& GetInstance();
	uint16_t nextInt();
	float nextFloat();
	bool RNGManager::SetDebugMode(bool is_debug, int seed);

private:
	static std::mt19937 _generator;
	static std::mutex _generator_lock;
	static bool _debug_rng;
	RNGManager() {};
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;
};
} // namespace openblack::ressources
