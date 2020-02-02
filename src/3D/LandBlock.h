/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/Mesh.h"
#include "Graphics/ShaderProgram.h"

#include <glm/fwd.hpp>

#include <cstdint>

class btBvhTriangleMeshShape;
class btRigidBody;

namespace openblack
{

namespace lnd
{
struct LNDBlock;
struct LNDCell;
} // namespace lnd

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

	LandVertex(const glm::vec3& _position, const glm::vec3& _weight, uint32_t mat[6], uint32_t blend[3], uint8_t _lightLevel,
	           float _alpha);
};

class LandIsland;

namespace dynamics
{
class LandBlockBulletMeshInterface;
}

class LandBlock
{
public:
	LandBlock() = default;
	void Draw(graphics::RenderPass viewId, const graphics::ShaderProgram& program, bool cullBack) const;
	void BuildMesh(LandIsland& island);

	[[nodiscard]] const graphics::Mesh& GetMesh() const { return *_mesh; }
	[[nodiscard]] const lnd::LNDCell* GetCells() const;
	[[nodiscard]] glm::ivec2 GetBlockPosition() const;
	[[nodiscard]] glm::vec4 GetMapPosition() const;
	[[nodiscard]] std::unique_ptr<btRigidBody>& GetRigidBody() { return _rigidBody; };

private:
	std::unique_ptr<lnd::LNDBlock> _block;
	std::unique_ptr<graphics::Mesh> _mesh;
	std::unique_ptr<dynamics::LandBlockBulletMeshInterface> _dynamicsMeshInterface;
	std::unique_ptr<btBvhTriangleMeshShape> _physicsMesh;
	std::unique_ptr<btRigidBody> _rigidBody;

	const bgfx::Memory* buildVertexList(LandIsland& island);

	friend LandIsland;
};
} // namespace openblack
