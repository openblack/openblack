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
class RandomNumberManagerTesting final: public RandomNumberManagerInterface
{
public:
	RandomNumberManagerTesting() = default;
	RandomNumberManagerTesting(const RandomNumberManagerTesting&) = delete;
	RandomNumberManagerTesting& operator=(const RandomNumberManagerTesting&) = delete;
	bool SetSeed(int seed);

private:
	std::mt19937& generator();
	std::optional<std::reference_wrapper<std::mutex>> lockAccess();
	bool lockCheck();
	std::mt19937 _generator;
	std::mutex _generatorLock;
};
} // namespace openblack
