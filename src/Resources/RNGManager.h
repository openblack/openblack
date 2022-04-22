/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include<random>

namespace openblack::ressources
{
class RNGManager
{
public:
    static RNGManager& GetInstance();
    uint16_t nextInt();
    void setSeed(unsigned int seed) { _generator.seed(seed); };

private:
    static std::mt19937 _generator;
    RNGManager() {};
    RNGManager(const RNGManager&) = delete;
    RNGManager& operator=(const RNGManager&) = delete;
}; 
} // namespace openblack
