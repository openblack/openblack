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

#include "Graphics/Mesh.h"
#include "Graphics/ShaderProgram.h"

#include <glm/fwd.hpp>

#include <cstdint>

namespace openblack
{

namespace lnd
{
struct LNDBlock;
struct LNDCell;
}

struct LandVertex
{
	float position[3];
	float weight[3];                     // interpolated
	uint8_t firstMaterialID[4];          // force alignment 4 bytes to prevent packing
	uint8_t secondMaterialID[4];         // force alignment 4 bytes to prevent packing
	uint8_t materialBlendCoefficient[4]; // force alignment 4 bytes to prevent
	                                     // packing
	uint8_t lightLevel[4];               // aligned to 4 bytes
	float waterAlpha;

	LandVertex(const glm::vec3& _position, const glm::vec3& _weight, uint32_t mat[6], uint32_t blend[3], uint8_t _lightLevel, float _alpha);
};

class LandIsland;

class LandBlock
{
public:
	LandBlock() = default;
	void Draw(graphics::RenderPass viewId, const graphics::ShaderProgram& program, bool cullBack) const;
	void BuildMesh(LandIsland& island);

	[[nodiscard]] const lnd::LNDCell* GetCells() const;
	[[nodiscard]] glm::ivec2 GetBlockPosition() const;

private:
	std::unique_ptr<lnd::LNDBlock> _block;
	std::unique_ptr<graphics::Mesh> _mesh;

	const bgfx::Memory* buildVertexList(LandIsland& island);

	friend LandIsland;
};
} // namespace openblack
