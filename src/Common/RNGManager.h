/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <mutex>
#include <random>

namespace openblack
{
class RNGManager
{
public:
	static RNGManager& instance();
	uint32_t NextUint32(uint32_t min, uint32_t max);
	float NextFloat(float min, float max);
	bool SetDebugMode(bool is_debug, int seed);

private:
	std::mt19937 _generator;
	std::mutex _generatorLock;
	bool _debugRng = false;
	RNGManager() {};
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;
};
} // namespace openblack
