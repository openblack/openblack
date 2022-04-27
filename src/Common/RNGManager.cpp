/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "RNGManager.h"

using namespace openblack;

RNGManager::RNGManager()
{
	_debugRng = false;
}

bool RNGManager::SetDebugMode(bool isDebug, int seed)
{
	std::lock_guard<std::mutex> safe_lock(_generatorLock);
	_debugRng = true;
	_generator.seed(seed);
	return _debugRng == isDebug;
}
