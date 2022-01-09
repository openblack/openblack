/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <bgfx/bgfx.h>

#include <cstddef>
#include <cstdint>
#include <string>

namespace openblack::graphics
{
class IndexBuffer
{
public:
	enum class Type : uint8_t
	{
		Uint16,
		Uint32,
	};
	static uint32_t GetTypeSize(Type type);

	IndexBuffer() = delete;
	IndexBuffer(const IndexBuffer& other) = delete;
	IndexBuffer(IndexBuffer&&) = default;

	IndexBuffer(std::string name, const void* indices, uint32_t indexCount, Type type);
	IndexBuffer(std::string name, const bgfx::Memory* memory, Type type);

	~IndexBuffer();

	[[nodiscard]] uint32_t GetCount() const;
	[[nodiscard]] uint32_t GetSize() const;
	[[nodiscard]] uint32_t GetStride() const;
	[[nodiscard]] Type GetType() const;

	void Bind(uint32_t count, uint32_t startIndex = 0) const;

private:
	std::string _name;
	uint32_t _count;
	Type _type;
	bgfx::IndexBufferHandle _handle;
};

} // namespace openblack::graphics
