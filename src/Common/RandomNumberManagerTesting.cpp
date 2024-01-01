/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RandomNumberManagerTesting.h"

#include <ctime>

#include <functional>

using namespace openblack;

bool RandomNumberManagerTesting::SetSeed(int seed)
{
	std::lock_guard<std::mutex> safeLock(_generatorLock);
	_generator.seed(seed);
	return true;
}

std::mt19937& RandomNumberManagerTesting::Generator()
{
	return _generator;
}

std::optional<std::reference_wrapper<std::mutex>> RandomNumberManagerTesting::LockAccess()
{
	return std::ref(_generatorLock);
}
