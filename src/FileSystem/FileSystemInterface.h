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
#include <vector>

#include "Stream.h"

namespace openblack::filesystem
{
class FileSystemInterface
{
public:
	static inline std::filesystem::path ScriptsPath() { return "Scripts"; }
	static inline std::filesystem::path PlaygroundPath() { return ScriptsPath() / "Playgrounds"; }
	static inline std::filesystem::path QuestsPath() { return ScriptsPath() / "Quests"; }
	static inline std::filesystem::path DataPath() { return "Data"; }
	static inline std::filesystem::path MiscPath() { return DataPath() / "Misc"; }
	static inline std::filesystem::path SymbolsPath() { return DataPath() / "Symbols"; }
	static inline std::filesystem::path LandscapePath() { return DataPath() / "Landscape"; }
	static inline std::filesystem::path TexturePath() { return DataPath() / "Textures"; }
	static inline std::filesystem::path WeatherSystemPath() { return DataPath() / "WeatherSystem"; }
	static inline std::filesystem::path CreatureMeshPath() { return DataPath() / "CreatureMesh"; }
	static inline std::filesystem::path CitadelPath() { return DataPath() / "Citadel"; }

	static std::filesystem::path FixPath(const std::filesystem::path& path);

	virtual ~FileSystemInterface();

	[[nodiscard]] virtual std::filesystem::path FindPath(const std::filesystem::path& path) const = 0;
	virtual std::unique_ptr<Stream> Open(const std::filesystem::path& path, Stream::Mode mode) = 0;
	virtual bool Exists(const std::filesystem::path& path) const = 0;
	virtual void SetGamePath(const std::filesystem::path& path) = 0;
	[[nodiscard]] virtual const std::filesystem::path& GetGamePath() const = 0;
	virtual void AddAdditionalPath(const std::filesystem::path& path) = 0;
	virtual std::vector<std::byte> ReadAll(const std::filesystem::path& path) = 0;
};

} // namespace openblack::filesystem
