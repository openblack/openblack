/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>

#include "IStream.h"

namespace openblack
{

enum class FileMode
{
	Read,
	Write,
	Append
};

class FileStream final: public IStream
{
public:
	FileStream(const std::filesystem::path& path, FileMode mode);
	virtual ~FileStream();

	[[nodiscard]] std::size_t Position() const override;
	[[nodiscard]] std::size_t Size() const override;
	void Seek(std::size_t position, SeekMode seek) override;

	void Read(void* buffer, std::size_t length) override;

protected:
	FILE* _file {nullptr};
	std::size_t _fileSize {0};
};

} // namespace openblack
