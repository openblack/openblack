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

#include "RandomNumberManager.h"

namespace openblack
{
class RandomNumberManagerProduction final: public RandomNumberManagerInterface
{
public:
	RandomNumberManagerProduction() = default;
	RandomNumberManagerProduction(const RandomNumberManagerProduction&) = delete;
	RandomNumberManagerProduction& operator=(const RandomNumberManagerProduction&) = delete;
	bool SetSeed(int seed);

private:
	std::mt19937& generator();
	std::mutex& lockAll();
	bool lockCheck();
};
} // namespace openblack
