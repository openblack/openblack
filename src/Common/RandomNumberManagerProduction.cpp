/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "RandomNumberManagerProduction.h"
#include <ctime>

using namespace openblack;

std::mt19937& RandomNumberManagerProduction::generator()
{
	thread_local std::mt19937 tGenerator(static_cast<unsigned int>(time(nullptr)));
	return tGenerator;
}

std::mutex& RandomNumberManagerProduction::lockAll()
{
	std::mutex rMutex;
	return rMutex;
}

bool RandomNumberManagerProduction::lockCheck()
{
	return false;
}
