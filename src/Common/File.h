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

#include "Common/IFile.h"

#include <filesystem>
#include <string>

namespace openblack
{

/**
 * Basic physical file
 */
class File : public IFile
{
public:
	enum class Mode { Read, Write };

public:
	File(const std::filesystem::path& path, Mode mode);
	~File() override;

public:
	// IFile overrides
	bool Read(std::uint8_t* dest, std::size_t bytesToRead) override;
	bool Write(std::uint8_t* source, std::size_t bytesToWrite) override;
	bool Flush() override;
	bool Seek(std::size_t position, SeekOrigin origin) override;
	[[nodiscard]] std::size_t GetPosition() override;

public:
	const std::string& GetFileName() const { return _fileName; }

protected:
	FILE* _file;
	std::string _fileName;
};

} // namespace openblack
