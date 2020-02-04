/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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
#include <string>
#include <vector>

namespace openblack
{

/*
FileSystem
 */
class FileSystem
{
public:
	static std::string FixPath(const std::string& path);

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
