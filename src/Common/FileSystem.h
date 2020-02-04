/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "File.h"

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

	[[nodiscard]] std::filesystem::path FindPath(const std::filesystem::path& path) const;

	//std::unique_ptr<FileStream> Open(const fs::path& path, FileMode mode);
	std::unique_ptr<File> Open(const std::filesystem::path& path, File::Mode mode);
	bool Exists(const std::filesystem::path& path);

	void SetGamePath(const std::filesystem::path& path) { _gamePath = path; }
	[[nodiscard]] const std::filesystem::path& GetGamePath() const { return _gamePath; }

	std::vector<std::byte> ReadAll(const std::filesystem::path& path);

private:
	std::filesystem::path _gamePath;
};

} // namespace openblack
