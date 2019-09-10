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

#include <cstdint>
#include <array>
#include <stdexcept>

#include <glm/glm.hpp>

#include <Graphics/Mesh.h>
#include <Graphics/ShaderProgram.h>

namespace openblack
{

struct LandVertex
{
	const float position[3];
	const float weight[3]; // interpolated
	const uint8_t firstMaterialID[4];  // force alignment 4 bytes to prevent packing
	const uint8_t secondMaterialID[4];  // force alignment 4 bytes to prevent packing
	const uint8_t materialBlendCoefficient[4];  // force alignment 4 bytes to prevent packing
	const uint8_t lightLevel;  // aligned to 4 bytes
	const float waterAlpha;

	LandVertex(glm::vec3 _position, glm::vec3 _weight,
	           uint8_t mat1, uint8_t mat2, uint8_t mat3,
	           uint8_t mat4, uint8_t mat5, uint8_t mat6,
	           uint8_t blend1, uint8_t blend2, uint8_t blend3,
	           uint8_t _lightLevel, float _alpha)
		: position{_position.x, _position.y, _position.z}
		, weight{_weight.x, _weight.y, _weight.z}
		, firstMaterialID{mat1, mat2, mat3}
		, secondMaterialID{mat4, mat5, mat6}
		, materialBlendCoefficient{blend1, blend2, blend3}
		, lightLevel{_lightLevel}
		, waterAlpha{_alpha}
	{
	}
};

class LandIsland;

class LandBlock
{
  public:
	LandBlock()
		: _index(0)
		, _cells()
		, _blockPosition(0, 0)
		, _mapPosition(0, 0)
	{}

	void Load(void* block, size_t block_size);
	void Draw(ShaderProgram& program);
	void BuildMesh(LandIsland& island);

	[[nodiscard]] const LandCell* GetCells() const { return _cells.data(); };
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
