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

#include <Common/FileSystem.h>

namespace OpenBlack
{

// todo: exceptions need to be replaced with real exceptions

File* FileSystem::Open(const std::string& path, FileMode mode)
{
	if (path.empty())
		throw std::invalid_argument("filename");

	/*if (path.is_relative())
		path = gamePath / path;

	if (path.is_absolute() && std::filesystem::exists(path)) */
	return new File(path, mode);

	throw std::runtime_error(path + " not found");
}

bool FileSystem::Exists(const std::string& filename)
{
	if (filename.empty())
		return false;

	return false;
}

} // namespace OpenBlack
