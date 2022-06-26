/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "IStream.h"

namespace openblack
{

class MemoryStream: public IStream
{
public:
	MemoryStream(const void* data, std::size_t size);

	[[nodiscard]] std::size_t Position() const override;
	[[nodiscard]] std::size_t Size() const override;
	void Seek(std::size_t position, SeekMode seek) override;

	void Read(void* buffer, std::size_t length) override;

protected:
	const void* _data;
	std::size_t _size;
	std::size_t _position;
};

} // namespace openblack
