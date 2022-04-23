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
	uint16_t nextInt();
	float nextFloat();
	bool SetDebugMode(bool is_debug, int seed);

private:
	std::mt19937 _generator;
	std::mutex _generator_lock;
	bool _debug_rng = false;
	RNGManager() {};
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;
};
} // namespace openblack::ressources
