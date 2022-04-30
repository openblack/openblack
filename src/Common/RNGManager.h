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

#include "RNG.h"

namespace openblack
{
class RNGManager final: public RNGInterface
{
public:
	RNGManager() = default;
	RNGManager(const RNGManager&) = delete;
	RNGManager& operator=(const RNGManager&) = delete;
	bool SetSeed(int seed);

private:
	std::mt19937& generator();
	std::mutex& lockAll();
	bool lockCheck();
};

class RNGManagerDebug final: public RNGInterface
{
public:
	RNGManagerDebug() = default;
	RNGManagerDebug(const RNGManagerDebug&) = delete;
	RNGManagerDebug& operator=(const RNGManagerDebug&) = delete;
	bool SetSeed(int seed);

private:
	std::mt19937& generator();
	std::mutex& lockAll();
	bool lockCheck();
	std::mt19937 _generator;
	std::mutex _generatorLock;
};
} // namespace openblack
