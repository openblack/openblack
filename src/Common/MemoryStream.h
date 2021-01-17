/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "IOStream.h"

namespace openblack
{

class MemoryStream: public IOStream
{
public:
	MemoryStream(void* data, std::size_t size);

	[[nodiscard]] std::size_t Position() const override;
	[[nodiscard]] std::size_t Size() const override;
	void Seek(std::size_t position, SeekMode seek) override;
	void Read(void* buffer, std::size_t length) override;
	void Write(const void* buffer, std::size_t length) override;
protected:
	void* _data;
	std::size_t _size;
	std::size_t _position;
};

} // namespace openblack
