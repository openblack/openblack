/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "DefaultFileSystem.h"

#include <cctype>
#include <cstddef>

#include "FileStream.h"

using namespace openblack::filesystem;

// todo: exceptions need to be replaced with real exceptions

std::filesystem::path DefaultFileSystem::FindPath(const std::filesystem::path& path) const
{
	if (path.empty())
	{
		throw std::invalid_argument("empty path");
	}

	// try absolute first
	if (path.is_absolute())
	{
		if (std::filesystem::exists(path))
		{
			return path;
		}
	}
	else
	{
		// try relative to current directory
		if (std::filesystem::exists(path))
		{
			return path;
		}

		// try relative to game directory
		if (std::filesystem::exists(_gamePath / path))
		{
			return _gamePath / path;
		}

		// try relative to additional paths
		for (const auto& p : _additionalPaths)
		{
			if (std::filesystem::exists(p / path))
			{
				return p / path;
			}
		}
	}

	throw std::runtime_error("File " + path.string() + " not found");
}

std::unique_ptr<Stream> DefaultFileSystem::Open(const std::filesystem::path& path, Stream::Mode mode)
{
	return std::unique_ptr<Stream>(new FileStream(FindPath(path), mode));
}

bool DefaultFileSystem::Exists(const std::filesystem::path& path) const
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

std::vector<uint8_t> DefaultFileSystem::ReadAll(const std::filesystem::path& path)
{
	auto file = Open(path, Stream::Mode::Read);
	const std::size_t size = file->Size();

	std::vector<uint8_t> data(size);
	file->Read(data.data(), size);

	return data;
}
