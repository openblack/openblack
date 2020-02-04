/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Bitmap16B.h"

#include "FileSystem.h"
#include "Game.h"

#include <cassert>
#include <cstring> // memset, memcpy
#include <sstream>
#include <stdexcept>

using namespace openblack;

Bitmap16B::Bitmap16B(const void* fileData)
{
	_width = *((uint32_t*)fileData + 1);
	_height = *((uint32_t*)fileData + 2);
	_size = _width * _height * 2;

	_data = new uint16_t[_width * _height];
	memcpy(_data, (uint32_t*)fileData + 4, _size);
}

Bitmap16B::~Bitmap16B()
{
    delete[] _data;
}

Bitmap16B* Bitmap16B::LoadFromFile(const std::string& strFile)
{
	auto const& data = Game::instance()->GetFileSystem().ReadAll(strFile);
	auto* bitmap = new Bitmap16B(data.data());

	return bitmap;
}
