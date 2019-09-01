/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <Common/File.h>
#include <cstddef>
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace OpenBlack
{

/*
FileSystem
 */
class FileSystem
{
  public:
	std::unique_ptr<File> Open(const fs::path& path, FileMode mode);
	bool Exists(const fs::path& path);

	void SetGamePath(const fs::path& path) { _gamePath = path; }
	const fs::path& GetGamePath() const { return _gamePath; }

	std::vector<fs::path> EnumFiles(const fs::path& directory);

	void Delete();
	void Rename();

	std::vector<std::byte> ReadAll(const fs::path& path);

  private:
	fs::path _gamePath;
};

} // namespace OpenBlack
