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
#ifndef OPENBLACK_MESHPACK_H
#define OPENBLACK_MESHPACK_H

#include <Common/OSFile.h>
#include <3D/L3DModel.h>
#include <Graphics/Texture2DArray.h>

namespace OpenBlack
{
	class MeshPack
	{
	public:
		MeshPack(OSFile* allMeshes);

		L3DModel** Models;
		GLuint* Textures;

		uint32_t GetMeshCount();
	private:
		uint32_t m_meshCount;
	};
}

#endif // OPENBLACK_MESHPACK_H
