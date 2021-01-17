/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
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
	static inline fs::path ScriptsPath() { return "Scripts"; }

	static inline fs::path PlaygroundPath() { return ScriptsPath() / "Playgrounds"; }

	static inline fs::path QuestsPath() { return ScriptsPath() / "Quests"; }

	static inline fs::path DataPath() { return "Data"; }

	static inline fs::path MiscPath() { return DataPath() / "Misc"; }

	static inline fs::path LandscapePath() { return DataPath() / "Landscape"; }

	static inline fs::path TexturePath() { return DataPath() / "Textures"; }

	static inline fs::path WeatherSystemPath() { return DataPath() / "WeatherSystem"; }

	static inline fs::path CreatureMeshPath() { return DataPath() / "CreatureMesh"; }

	static inline fs::path WorkingDirectory() { return fs::current_path(); }

	static fs::path FixPath(const fs::path& path);

	[[nodiscard]] fs::path FindPath(const fs::path& path) const;

	std::unique_ptr<FileStream> Open(const fs::path& path, FileMode mode);
	bool Exists(const fs::path& path);

	void SetGamePath(const fs::path& path) { _gamePath = path; }
	[[nodiscard]] const fs::path& GetGamePath() const { return _gamePath; }

	std::vector<std::byte> ReadAll(const fs::path& path);

private:
	fs::path _gamePath;
};

} // namespace openblack
