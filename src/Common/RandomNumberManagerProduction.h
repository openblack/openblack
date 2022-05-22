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

private:
	std::mt19937& Generator() override;
	std::optional<std::reference_wrapper<std::mutex>> LockAccess() override;
};
} // namespace openblack
