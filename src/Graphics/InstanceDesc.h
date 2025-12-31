/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include "GraphicsHandle.h"

namespace openblack::graphics
{
class Texture2D;

class InstanceDesc
{
public:
	InstanceDesc() = delete;
	InstanceDesc(DynamicVertexBufferHandle buffer, uint32_t start, uint32_t count);

	[[nodiscard]] DynamicVertexBufferHandle GetRawHandle() const { return _buffer; }
	[[nodiscard]] uint32_t GetStart() const { return _start; }
	[[nodiscard]] uint32_t GetCount() const { return _count; }

private:
	DynamicVertexBufferHandle _buffer;
	uint32_t _start;
	uint32_t _count;
};

} // namespace openblack::graphics
