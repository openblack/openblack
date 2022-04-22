/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "FileStream.h"

#include <cstddef>
#include <list>
#include <memory>
#include <vector>

namespace openblack
{

/*
FileSystem
 */
class FileSystem
{
public:
	static inline std::filesystem::path ScriptsPath() { return "Scripts"; }

	static inline std::filesystem::path PlaygroundPath() { return ScriptsPath() / "Playgrounds"; }

	static inline std::filesystem::path QuestsPath() { return ScriptsPath() / "Quests"; }

	static inline std::filesystem::path DataPath() { return "Data"; }

	static inline std::filesystem::path MiscPath() { return DataPath() / "Misc"; }

	static inline std::filesystem::path LandscapePath() { return DataPath() / "Landscape"; }

	static inline std::filesystem::path TexturePath() { return DataPath() / "Textures"; }

	static inline std::filesystem::path WeatherSystemPath() { return DataPath() / "WeatherSystem"; }

	static inline std::filesystem::path CreatureMeshPath() { return DataPath() / "CreatureMesh"; }

	static inline std::filesystem::path CitadelPath() { return DataPath() / "Citadel"; }

	static std::filesystem::path FixPath(const std::filesystem::path& path);

	[[nodiscard]] std::filesystem::path FindPath(const std::filesystem::path& path) const;

	std::unique_ptr<FileStream> Open(const std::filesystem::path& path, FileMode mode);
	bool Exists(const std::filesystem::path& path);

	void SetGamePath(const std::filesystem::path& path) { _gamePath = path; }
	[[nodiscard]] const std::filesystem::path& GetGamePath() const { return _gamePath; }

	std::vector<std::byte> ReadAll(const std::filesystem::path& path);

private:
	std::filesystem::path _gamePath;
};

} // namespace openblack
