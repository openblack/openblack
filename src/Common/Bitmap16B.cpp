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

#include <Common/Bitmap16B.h>

#include <stdexcept>
#include <sstream>
#include <cassert>

#include <Common/OSFile.h>

using namespace OpenBlack;

Bitmap16B::Bitmap16B(unsigned int width, unsigned int height)
	: _width(width), _height(height)
{
	_data = new uint16_t[width * height];
	std::memset(_data, 0x00, width * height * 2);
}

Bitmap16B::Bitmap16B(const void* fileData, size_t size)
{
	auto cast = reinterpret_cast<const uint32_t *>(fileData);

	_width = *((uint32_t*)fileData + 1);
	_height = *((uint32_t*)fileData + 2);

	_data = new uint16_t[_width * _height];
	std::memcpy(_data, (uint32_t*)fileData + 4, _width * _height * 2);
}

Bitmap16B::~Bitmap16B()
{
	if (_data != nullptr)
		delete[] _data;
}

const unsigned int Bitmap16B::Width() const { return _width; }
const unsigned int Bitmap16B::Height() const { return _height; }
const uint16_t* Bitmap16B::Data() const { return _data; }

Bitmap16B* Bitmap16B::LoadFromFile(const std::string& file)
{
	size_t fileSize;
	char* fileData = OSFile::ReadAll(file.c_str(), &fileSize);
	Bitmap16B* bitmap = new Bitmap16B(fileData, fileSize);
	free(fileData);

	return bitmap;
}
