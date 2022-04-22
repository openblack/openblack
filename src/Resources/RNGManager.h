/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <random>
#include <mutex>

namespace openblack::ressources
{
class RNGManager
{
public:
	static RNGManager& GetInstance();
	uint16_t nextInt();
	float nextFloat();
	void setSeed(unsigned int seed);

private:
	static std::mt19937 _generator;
	static std::mutex _generator_lock;
	RNGManager() {};
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;
}; 
} // namespace openblack
