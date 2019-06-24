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
#include <filesystem>
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
	std::shared_ptr<File> Open(const std::filesystem::path& path, FileMode mode);
	bool Exists(const std::filesystem::path& path);

	void SetGamePath(const std::filesystem::path& path) { _gamePath = path; }
	const std::filesystem::path& GetGamePath() const { return _gamePath; }

	std::vector<std::filesystem::path> EnumFiles(const std::filesystem::path& directory);

	void Delete();
	void Rename();

  private:
	std::filesystem::path _gamePath;
};

} // namespace OpenBlack
