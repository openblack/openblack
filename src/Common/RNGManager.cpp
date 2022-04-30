/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "RNGManager.h"
#include <ctime>

using namespace openblack;

bool RNGManager::SetSeed(int seed)
{
	return false;
}

std::mt19937& RNGManager::generator()
{
	thread_local std::mt19937 tGenerator(static_cast<unsigned int>(time(nullptr)));
	return tGenerator;
}

std::mutex& RNGManager::lockAll()
{
	std::mutex rMutex;
	return rMutex;
}

bool RNGManager::lockCheck()
{
	return false;
}

bool RNGManagerDebug::SetSeed(int seed)
{
	std::lock_guard<std::mutex> safe_lock(_generatorLock);
	_generator.seed(seed);
	return true;
}

std::mt19937& RNGManagerDebug::generator()
{
	return _generator;
}

std::mutex& RNGManagerDebug::lockAll()
{
	return _generatorLock;
}

bool RNGManagerDebug::lockCheck()
{
	return true;
}
