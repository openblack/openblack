/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cassert>
#include <cstdint>

#include <filesystem>
#include <functional>
#include <iosfwd>
#include <memory>
#include <vector>

#include "Stream.h"

namespace openblack::filesystem
{
enum class Path
{
	Audio,
	Scripts,
	Data,
	Playgrounds,
	Quests,
	CreatureMind,
	Misc,
	Symbols,
	Landscape,
	Textures,
	WeatherSystem,
	CreatureMesh,
	Citadel,
	CitadelEngine,
};

class FileSystemInterface
{
public:
	static std::filesystem::path FixPath(const std::filesystem::path& path);

	virtual ~FileSystemInterface();

	template <Path pathType>
	[[nodiscard]] std::filesystem::path GetPath(bool withGamePath = false) const
	{
		std::filesystem::path result;
		if constexpr (pathType == Path::Audio)
		{
			result = "Audio";
		}
		else if constexpr (pathType == Path::Scripts)
		{
			result = "Scripts";
		}
		else if constexpr (pathType == Path::Data)
		{
			result = "Data";
		}
		else if constexpr (pathType == Path::Playgrounds)
		{
			result = GetPath<Path::Scripts>() / "Playgrounds";
		}
		else if constexpr (pathType == Path::Quests)
		{
			result = GetPath<Path::Scripts>() / "Quests";
		}
		else if constexpr (pathType == Path::CreatureMind)
		{
			result = GetPath<Path::Scripts>() / "CreatureMind";
		}
		else if constexpr (pathType == Path::Misc)
		{
			result = GetPath<Path::Data>() / "Misc";
		}
		else if constexpr (pathType == Path::Symbols)
		{
			result = GetPath<Path::Data>() / "Symbols";
		}
		else if constexpr (pathType == Path::Landscape)
		{
			result = GetPath<Path::Data>() / "Landscape";
		}
		else if constexpr (pathType == Path::Textures)
		{
			result = GetPath<Path::Data>() / "Textures";
		}
		else if constexpr (pathType == Path::WeatherSystem)
		{
			result = GetPath<Path::Data>() / "WeatherSystem";
		}
		else if constexpr (pathType == Path::CreatureMesh)
		{
			result = GetPath<Path::Data>() / "CreatureMesh";
		}
		else if constexpr (pathType == Path::Citadel)
		{
			result = GetPath<Path::Data>() / "Citadel";
		}
		else if constexpr (pathType == Path::CitadelEngine)
		{
			result = GetPath<Path::Citadel>() / "engine";
		}
		else
		{
			assert(false);
		}
		return withGamePath ? (GetGamePath() / result) : result;
	}

	[[nodiscard]] virtual std::filesystem::path FindPath(const std::filesystem::path& path) const = 0;
	virtual std::unique_ptr<std::istream> GetData(const std::filesystem::path& path) = 0;
	[[nodiscard]] virtual bool IsPathValid(const std::filesystem::path& path) = 0;
	virtual std::unique_ptr<Stream> Open(const std::filesystem::path& path, Stream::Mode mode) = 0;
	[[nodiscard]] virtual bool Exists(const std::filesystem::path& path) const = 0;
	virtual void SetGamePath(const std::filesystem::path& path) = 0;
	[[nodiscard]] virtual const std::filesystem::path& GetGamePath() const = 0;
	virtual void AddAdditionalPath(const std::filesystem::path& path) = 0;
	virtual std::vector<uint8_t> ReadAll(const std::filesystem::path& path) = 0;
	virtual void Iterate(const std::filesystem::path& path, bool recursive,
	                     const std::function<void(const std::filesystem::path&)>& function) const = 0;
};

} // namespace openblack::filesystem
