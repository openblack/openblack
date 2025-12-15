/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "InstanceDesc.h"

using namespace openblack::graphics;

InstanceDesc::InstanceDesc(bgfx::DynamicVertexBufferHandle buffer, uint32_t start, uint32_t count)
    : _buffer(buffer)
    , _start(start)
    , _count(count)
{
}
