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
using namespace openblack::ressources;


int RNGManager::nextInt()
{
    thread_local std::mt19937 _generator(_seed);
    std::uniform_int_distribution<uint16_t> int_dist(1,500);
    return int_dist(_generator);
}

RNGManager::RNGManager()
{
    std::random_device seeder;
    _seed = seeder();
}