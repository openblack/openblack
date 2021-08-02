/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "FileSystem.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>

namespace openblack
{

fs::path FileSystem::FixPath(const fs::path& path)
{
	std::string result = path.generic_string();

	constexpr std::array<std::string_view, 3> caseFixTable = {
	    "\\Data\\",
	    "\\Landscape\\",
	    "\\Multi_Player\\",
	};
	for (auto& pattern : caseFixTable)
	{
		auto foundIter = std::search(result.cbegin(), result.cend(), pattern.cbegin(), pattern.cend(),
		                             [](char left, char right) { return std::toupper(left) == std::toupper(right); });
		if (foundIter != result.cend())
		{
			result.replace(foundIter, foundIter + pattern.size(), pattern.data());
		}
	}

	for (auto pos = result.find('\\'); pos != std::string::npos; pos = result.find('\\', pos + 1))
	{
		result[pos] = '/';
	}

	return result;
}

// todo: exceptions need to be replaced with real exceptions

fs::path FileSystem::FindPath(const fs::path& path) const
{
	if (path.empty())
		throw std::invalid_argument("empty path");

	// try absolute first
	if (path.is_absolute() && fs::exists(path))
		return path;

	// try relative to current directory
	if (path.is_relative() && fs::exists(path))
		return path;

	// try relative to game directory
	if (path.is_relative() && fs::exists(_gamePath / path))
		return _gamePath / path;

	throw std::runtime_error("File " + path.string() + " not found");
}

std::unique_ptr<FileStream> FileSystem::Open(const fs::path& path, FileMode mode)
{
	return std::make_unique<FileStream>(FindPath(path), mode);
}

bool FileSystem::Exists(const fs::path& path) const
{
	try
	{
		[[maybe_unused]] auto realPath = FindPath(path);
		return true;
	}
	catch (std::exception&)
	{
		return false;
	}
}

std::vector<std::byte> FileSystem::ReadAll(const fs::path& path)
{
	auto file = Open(path, FileMode::Read);
	std::size_t size = file->Size();

	std::vector<std::byte> data(size);
	file->Read(data.data(), size);

	return data;
}

} // namespace openblack
