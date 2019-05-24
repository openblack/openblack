/* OpenBlack - A reimplementation of Lionheads's Black & White engine
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
#ifndef OPENBLACK_L3DMODEL_H
#define OPENBLACK_L3DMODEL_H

#include <Graphics/Mesh.h>

using namespace OpenBlack::Graphics;

namespace OpenBlack
{
	class L3DModel
	{
	public:
		L3DModel();
		~L3DModel();

		void LoadFromFile(const std::string& fileName);
		void LoadFromL3D(void* data, size_t size, bool pack);
		void Draw();

		unsigned int GetSubMeshCount();
	protected:
		GLuint* _submeshSkinIds;
		Mesh** _submeshes;
		unsigned int _submeshCount;

		GLuint* m_glSkins;

		bool m_bPackedMesh;
	};
}

#endif
