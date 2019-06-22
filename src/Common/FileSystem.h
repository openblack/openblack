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
#include <list>
#include <memory>
#include <string>
#include <vector>

namespace OpenBlack
{

class FileSystem
{
  public:
	enum class SearchPathAdd
	{
		Head, // First path searched
		Tail, // Last path searched
	};

	FileSystem()  = default;
	~FileSystem() = default;

	File* Open(const std::string& path, FileMode mode);
	bool Exists(const std::string& path);

	// Search paths
	void AddSearchPath(const std::string& path, SearchPathAdd addType = SearchPathAdd::Tail);

	std::vector<std::string> EnumFiles(std::string& directory);

  private:
	std::list<std::string> _searchPaths;
};

} // namespace OpenBlack
