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

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM

namespace openblack
{

enum class FileMode
{
	Read,
	Write,
	Append
};

class FileStream: public IOStream
{
public:
	FileStream(const fs::path& filename, FileMode mode);
	virtual ~FileStream();

	[[nodiscard]] std::size_t Position() const override;
	[[nodiscard]] std::size_t Size() const override;
	void Seek(std::size_t position, SeekMode seek) override;

	void Read(void* buffer, std::size_t length) override;
	void Write(const void* buffer, std::size_t length) override;
protected:
	FILE* _file;
	std::size_t _fileSize;
};

} // namespace openblack
