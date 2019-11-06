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

#include <Common/FileSystem.h>

#include <cstddef>
#include <algorithm>
#include <array>

namespace openblack
{

std::string FileSystem::FixPath(const std::string& path)
{
	std::string result = path;

	constexpr std::array<std::string_view, 3> caseFixTable = {
		"\\Data\\",
		"\\Landscape\\",
		"\\Multi_Player\\",
	};
	for (auto& pattern : caseFixTable)
	{
		auto foundIter = std::search(result.cbegin(), result.cend(), pattern.cbegin(), pattern.cend(),
		                             [] (char left, char right){ return std::toupper(left) == std::toupper(right); });
		if (foundIter != result.cend())
		{
			result.replace(foundIter, foundIter + pattern.size(), pattern.data());
		}
	}

	for (auto pos = result.find('\\'); pos != std::string::npos; pos = result.find('\\', pos + 1))
	{
		result[pos] = '/';
	}

	return result;
}

// todo: exceptions need to be replaced with real exceptions

std::unique_ptr<FileStream> FileSystem::Open(const fs::path& path, FileMode mode)
{
	if (path.empty())
		throw std::invalid_argument("empty path");

	// try absolute first
	if (path.is_absolute() && fs::exists(path))
		return std::make_unique<FileStream>(path, mode);

	// try relative to current directory
	if (path.is_relative() && fs::exists(path))
		return std::make_unique<FileStream>(path, mode);
		
	// try relative to game directory
	if (path.is_relative() && fs::exists(_gamePath / path))
		return std::make_unique<FileStream>(_gamePath / path, mode);

	throw std::runtime_error("File " + path.string() + " not found");
}

bool FileSystem::Exists(const fs::path& path)
{
	if (path.empty())
		return false;

	if (path.is_absolute() && fs::exists(path))
		return true;

	if (path.is_relative() && fs::exists(path))
		return true;

	if (path.is_relative() && fs::exists(_gamePath / path))
		return true;

	return false;
}

std::vector<std::byte> FileSystem::ReadAll(const fs::path& path)
{
	auto file = Open(path, FileMode::Read);
	std::size_t size = file->Size();

	std::vector<std::byte> data(size);
	file->Read(data.data(), size);

	return data;
}

} // namespace openblack
