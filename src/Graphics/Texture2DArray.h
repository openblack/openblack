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
#ifndef OPENBLACK_GRAPHICS_TEXTURE2DARRAY_H
#define OPENBLACK_GRAPHICS_TEXTURE2DARRAY_H

#include <Graphics/Texture2D.h>

namespace OpenBlack
{
namespace Graphics
{

class Texture2DArray
{
  public:
	Texture2DArray();
	~Texture2DArray();

	void Create(unsigned int width, unsigned int height, unsigned int depth, InternalFormat internalFormat);

	void SetTexture(unsigned int layer, unsigned int width, unsigned int height, Format format, DataType type, const void* textureData);

	void Bind() const;

	unsigned int GetNativeHandle() const { return _texture; }
	unsigned int GetWidth() const { return _width; }
	unsigned int GetHeight() const { return _height; }

  private:
	unsigned int _texture;
	unsigned int _width;
	unsigned int _height;
	unsigned int _depth;
};

} // namespace Graphics
} // namespace OpenBlack

#endif
