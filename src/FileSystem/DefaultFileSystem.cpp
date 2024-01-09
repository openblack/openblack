/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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

#include <filesystem>
#include <fstream>
#include <system_error>

#include <spdlog/spdlog.h>
#include <filesystem>
#include <system_error>

#include "FileStream.h"
#include "fmt/format.h"

#ifdef _WIN32
// clang-format off
// can't sort these includes
#include <wtypes.h>
#include <winreg.h>
// clang-format on
#endif

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

bool DefaultFileSystem::IsPathValid(const std::filesystem::path& path)
{
	if (path.empty())
		return false;

	if (!Exists(path))
		return false;

	if (!std::filesystem::is_directory(path)) {
		return false;
}

	return true;
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

void DefaultFileSystem::Iterate(const std::filesystem::path& path, bool recursive,
                                const std::function<void(const std::filesystem::path&)>& function) const
{
	const auto fixedPath = FindPath(path);
	if (recursive)
	{
		for (const auto& f : std::filesystem::recursive_directory_iterator {fixedPath})
		{
			function(f);
		}
	}
	else
	{
		for (const auto& f : std::filesystem::directory_iterator {fixedPath})
		{
			function(f);
		}
	}
}

void DefaultFileSystem::SetGamePath(const std::filesystem::path& path)
{
	_gamePath = path;

#if defined(unix) || defined(__unix__) || defined(__unix)
	if (_gamePath.string().size() >= 2 && _gamePath.string().c_str()[0] == '~' && _gamePath.string().c_str()[1] == '/')
	{
		_gamePath = std::getenv("HOME") + _gamePath.string().substr(1);
	}
#endif

	if (_gamePath.empty())
	{
#ifdef _WIN32
		DWORD dataLen = 0;
		LSTATUS status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir",
		                             RRF_RT_REG_SZ, nullptr, nullptr, &dataLen);
		if (status == ERROR_SUCCESS)
		{
			std::vector<char> data(dataLen);
			status = RegGetValue(HKEY_CURRENT_USER, "SOFTWARE\\Lionhead Studios Ltd\\Black & White", "GameDir", RRF_RT_REG_SZ,
			                     nullptr, data.data(), &dataLen);

			_gamePath = std::filesystem::path(data.data());
		}
		else
		{
			throw std::runtime_error(fmt::format("Failed to find the GameDir registry value, game not installed."));
		}
#endif // _WIN32
	}

	if (!_gamePath.empty() && !Exists(_gamePath))
	{
		throw std::runtime_error(fmt::format("GamePath does not exist: '{}'", _gamePath.generic_string()));
	}
}
