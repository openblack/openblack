/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RandomNumberManagerProduction.h"

#include <ctime>

using namespace openblack;

std::mt19937& RandomNumberManagerProduction::Generator()
{
	thread_local std::mt19937 tGenerator(static_cast<unsigned int>(time(nullptr)));
	return tGenerator;
}

std::optional<std::reference_wrapper<std::mutex>> RandomNumberManagerProduction::LockAccess()
{
	return std::nullopt;
}
