/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Graphics/ShaderProgram.h"
#include "LandIslandInterface.h"

class btBvhTriangleMeshShape;
class btRigidBody;

namespace openblack
{

namespace lnd
{
struct LNDBlock;
struct LNDCell;
} // namespace lnd

namespace graphics
{
class Mesh;
}

struct LandVertex
{
	glm::vec3 position;
	glm::vec3 weight;                     // interpolated
	glm::u8vec4 firstMaterialID;          // force alignment 4 bytes to prevent packing
	glm::u8vec4 secondMaterialID;         // force alignment 4 bytes to prevent packing
	glm::u8vec4 materialBlendCoefficient; // force alignment 4 bytes to prevent packing
	glm::u8vec4 lightLevel;               // aligned to 4 bytes
	float waterAlpha;

	LandVertex(const glm::vec3& position, const glm::vec3& weight, const std::array<uint32_t, 6>& mat, const glm::uvec3& blend,
	           uint8_t lightLevel, float alpha);
};

class LandIslandInterface;

namespace dynamics
{
class LandBlockBulletMeshInterface;
}

class LandBlock
{
public:
	LandBlock() = default;
	void BuildMesh(LandIslandInterface& island);

	[[nodiscard]] const graphics::Mesh& GetMesh() const { return *_mesh; }
	[[nodiscard]] const lnd::LNDCell* GetCells() const;
	[[nodiscard]] glm::ivec2 GetBlockPosition() const;
	[[nodiscard]] glm::vec2 GetMapPosition() const;
	[[nodiscard]] std::unique_ptr<btRigidBody>& GetRigidBody() { return _rigidBody; };
	[[nodiscard]] const std::unique_ptr<lnd::LNDBlock>& GetLndBlock() const { return _block; };
	void SetLndBlock(const lnd::LNDBlock& block);

private:
	std::unique_ptr<lnd::LNDBlock> _block;
	std::unique_ptr<graphics::Mesh> _mesh;
	std::unique_ptr<dynamics::LandBlockBulletMeshInterface> _dynamicsMeshInterface;
	std::unique_ptr<btBvhTriangleMeshShape> _physicsMesh;
	std::unique_ptr<btRigidBody> _rigidBody;

	const bgfx::Memory* BuildVertexList(LandIslandInterface& island);
};
} // namespace openblack
