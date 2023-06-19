/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <vector>

#include "Stream.h"

namespace openblack::filesystem
{

class MemoryStream: public Stream
{
public:
	MemoryStream(std::vector<uint8_t>&& data);

	[[nodiscard]] std::size_t Position() const override;
	[[nodiscard]] std::size_t Size() const override;
	void Seek(std::size_t position, SeekMode seek) override;

	Stream& Read(uint8_t* buffer, std::size_t length) override;
	Stream& Write(const uint8_t* buffer, std::size_t length) override;

	std::string GetLine() override;

	bool IsEndOfFile() const override;

protected:
	std::vector<uint8_t> _data;
	std::size_t _position;
};

} // namespace openblack::filesystem
