/*******************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 ******************************************************************************/

#include "RNGManager.h"

namespace openblack
{
class RNGInterface
{
public:
    virtual RNGManager& getRNG() = 0;
};

class RNG final: public RNGInterface
{
public:
    RNGManager& getRNG() override { return _rngManager; }
private:
    RNGManager _rngManager;
};

} // namespace openblack
