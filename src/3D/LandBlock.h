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

#include "LandCell.h"

#include <Graphics/Mesh.h>
#include <Graphics/OpenGL.h>
#include <Graphics/ShaderProgram.h>
#include <glm/glm.hpp>
#include <array> 
#include <stdexcept>
#include <stdint.h>

namespace openblack
{


struct LandVertex
{
	GLfloat position[3];
	GLfloat weight[3]; // interpolated
	GLubyte firstMaterialID[4];  // force alignment 4 bytes to prevent packing
	GLubyte secondMaterialID[4];  // force alignment 4 bytes to prevent packing
	GLubyte materialBlendCoefficient[4];  // force alignment 4 bytes to prevent packing
	GLubyte lightLevel;  // aligned to 4 bytes
	GLfloat waterAlpha;

	LandVertex() { }
	LandVertex(glm::vec3 _position, glm::vec3 _weight,
	           GLubyte mat1, GLubyte mat2, GLubyte mat3,
	           GLubyte mat4, GLubyte mat5, GLubyte mat6,
	           GLubyte blend1, GLubyte blend2, GLubyte blend3,
	           GLubyte _lightLevel, GLfloat _alpha)
	{
		position[0]                 = _position.x;
		position[1]                 = _position.y;
		position[2]                 = _position.z;
		weight[0]                   = _weight.x;
		weight[1]                   = _weight.y;
		weight[2]                   = _weight.z;
		firstMaterialID[0]          = mat1;
		firstMaterialID[1]          = mat2;
		firstMaterialID[2]          = mat3;
		secondMaterialID[0]         = mat4;
		secondMaterialID[1]         = mat5;
		secondMaterialID[2]         = mat6;
		materialBlendCoefficient[0] = blend1;
		materialBlendCoefficient[1] = blend2;
		materialBlendCoefficient[2] = blend3;
		lightLevel                  = _lightLevel;
		waterAlpha                  = _alpha;
	}
};

class LandIsland;

class LandBlock
{
  public:
	LandBlock() :
	    _index(0), _blockPosition(0, 0), _mapPosition(0, 0), _cells() {}

	void Load(void* block, size_t block_size);
	void Draw(ShaderProgram& program);
	void BuildMesh(LandIsland& island);

	const LandCell* GetCells() const { return _cells.data(); };
	const glm::ivec2& GetBlockPosition() { return _blockPosition; }
	const glm::vec2& GetMapPosition() { return _mapPosition; }


  private:
	uint32_t _index; // the blocks index in the block array (do we need to know this?)
	std::array<LandCell, 289> _cells;
	glm::ivec2 _blockPosition; // position in the 32x32 block map
	glm::vec2 _mapPosition; // absolute position in the world
	std::unique_ptr<Mesh> _mesh;

	std::vector<LandVertex> buildVertexList(LandIsland& island);
};
} // namespace openblack
