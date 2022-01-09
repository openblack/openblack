/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <cstdint>
#include <string>

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM

namespace openblack
{
class Bitmap16B
{
public:
	Bitmap16B() = delete;

	Bitmap16B(const void* data);
	~Bitmap16B();

	[[nodiscard]] unsigned int Width() const { return _width; }
	[[nodiscard]] unsigned int Height() const { return _height; }
	uint16_t* Data() { return _data; }
	[[nodiscard]] size_t Size() const { return _size; };

private:
	unsigned int _width;
	unsigned int _height;
	uint16_t* _data;
	size_t _size;

public:
	static Bitmap16B* LoadFromFile(const fs::path& path);
};

} // namespace openblack
