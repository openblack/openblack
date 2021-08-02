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
#include "StringUtilities.h"

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

	static fs::path FixPath(const fs::path& path);

	[[nodiscard]] fs::path FindPath(const fs::path& path) const;

	std::unique_ptr<FileStream> Open(const fs::path& path, FileMode mode);
	bool Exists(const fs::path& path) const;

	void SetGamePath(const fs::path& path) { _gamePath = path; }
	[[nodiscard]] const fs::path& GetGamePath() const { return _gamePath; }

	std::vector<std::filesystem::path> GetAllFilePaths(const std::string& path, const std::string& ext, bool recursive) const
	{
		auto foundPath = FindPath(path);
		if (recursive)
			return GetAllFilePaths(std::filesystem::recursive_directory_iterator(foundPath), ext);
		else
			return GetAllFilePaths(std::filesystem::directory_iterator(foundPath), ext);
	}

	std::vector<uint8_t> ReadAll(const fs::path& path);

private:
	template <typename DirectoryIter>
	std::vector<std::filesystem::path> GetAllFilePaths(DirectoryIter it, const std::string& ext) const
	{
		std::vector<std::filesystem::path> paths;

		for (auto entry : it)
		{
			if (entry.is_directory())
				continue;

			auto curExt = entry.path().extension().string();
			LowerCase(curExt);

			if (curExt != ext)
				continue;

			paths.push_back(entry.path());
		}

		return paths;
	}

	fs::path _gamePath;
};

} // namespace openblack
