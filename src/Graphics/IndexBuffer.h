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

#include <functional>
#include <memory>
#include <string>

#include "GraphicsHandle.h"

namespace openblack::graphics
{
struct IndexBuffer
{
	enum class Type : uint8_t
	{
		Uint16,
		Uint32,
	};
	static uint32_t GetTypeSize(Type type)
	{
		return static_cast<uint32_t>(type == Type::Uint16 ? sizeof(uint16_t) : sizeof(uint32_t));
	}

	const std::string name;
	const uint32_t count;
	const Type type;
	const IndexBufferHandle handle;
};

using IndexBufferUniquePtr = std::unique_ptr<IndexBuffer, std::function<void(IndexBuffer*)>>;

} // namespace openblack::graphics
