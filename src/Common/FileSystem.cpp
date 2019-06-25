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

std::shared_ptr<File> FileSystem::Open(const std::filesystem::path& path, FileMode mode)
{
	if (path.empty())
		throw std::invalid_argument("empty path");

	// try absolute first
	if (path.is_absolute() && std::filesystem::exists(path))
		return std::make_shared<File>(path, mode);

	// try relative to current directory
	if (path.is_relative() && std::filesystem::exists(path))
		return std::make_shared<File>(path, mode);
		
	// try relative to game directory
	if (path.is_relative() && std::filesystem::exists(_gamePath / path))
		return std::make_shared<File>(_gamePath / path, mode);

	throw std::runtime_error("File " + path.string() + " not found");
}

bool FileSystem::Exists(const std::filesystem::path& path)
{
	if (path.empty())
		return false;

	if (path.is_absolute() && std::filesystem::exists(path))
		return true;

	if (path.is_relative() && std::filesystem::exists(path))
		return true;

	if (path.is_relative() && std::filesystem::exists(_gamePath / path))
		return true;

	return false;
}

} // namespace OpenBlack
