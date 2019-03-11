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
#ifndef OPENBLACK_LANDBLOCK_H
#define OPENBLACK_LANDBLOCK_H

#include <stdexcept>

#include <stdint.h>
#include <Graphics/OpenGL.h>
#include <glm/glm.hpp>

#include <Graphics/Mesh.h>

#include "LandCell.h"

#define OPENBLACK_LANDISLAND_HEIGHT_UNIT 0.67f
#define OPENBLACK_LANDBLOCK_TOTAL_CELLS 289

namespace OpenBlack
{
	#pragma pack(push, 1)
	struct LandBlockVertex
	{
		GLfloat position[3];
		GLfloat weight[3]; // interpolated
		GLubyte firstMaterialID[3];
		GLubyte secondMaterialID[3];
		GLubyte materialBlendCoefficient[3];
		GLubyte lightLevel;
		GLfloat alpha;
	};

	#pragma pack(pop)

	class LandBlock
	{
	public:
		LandBlock();

		void LoadFromFile(void* block, size_t block_size);
		LandCell* GetCells() {
			return _cells;
		};
		glm::ivec2* GetBlockPosition() { return &_blockPosition; }
		glm::vec2* GetMapPosition() { return &_mapPosition; }
	private:
		LandCell _cells[OPENBLACK_LANDBLOCK_TOTAL_CELLS];
		uint32_t _index;
		glm::ivec2 _blockPosition;
		glm::vec2 _mapPosition;

		void buildMesh();
		std::vector<LandBlockVertex> buildVertexList();
		std::unique_ptr<Mesh> _mesh;
	};
}

#endif
