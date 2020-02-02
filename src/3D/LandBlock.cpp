/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "LandBlock.h"

#include "Dynamics/LandBlockBulletMeshInterface.h"
#include "Graphics/VertexBuffer.h"
#include "LandIsland.h"

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LNDFile.h>
#include <glm/gtc/type_ptr.hpp>

#include <cassert>
#include <utility>

using namespace openblack;
using namespace openblack::graphics;

LandVertex::LandVertex(const glm::vec3& _position, const glm::vec3& _weight, uint32_t mat[8], uint32_t blend[3],
                       uint8_t _lightLevel, float _alpha)
    : position {_position.x, _position.y, _position.z}
    , weight {_weight.x, _weight.y, _weight.z}
    , firstMaterialID {static_cast<uint8_t>(mat[0]), static_cast<uint8_t>(mat[1]), static_cast<uint8_t>(mat[2])}
    , secondMaterialID {static_cast<uint8_t>(mat[3]), static_cast<uint8_t>(mat[4]), static_cast<uint8_t>(mat[5])}
    , materialBlendCoefficient {static_cast<uint8_t>(blend[0]), static_cast<uint8_t>(blend[1]), static_cast<uint8_t>(blend[2])}
    , lightLevel {_lightLevel}
    , waterAlpha {_alpha}
{
}

void LandBlock::BuildMesh(LandIsland& island)
{
	if (_mesh != nullptr)
		_mesh.reset();

	VertexDecl decl;
	decl.reserve(7);
	decl.emplace_back(VertexAttrib::Attribute::Position, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	// weight
	decl.emplace_back(VertexAttrib::Attribute::TexCoord1, static_cast<uint8_t>(3), VertexAttrib::Type::Float);
	// first material id
	decl.emplace_back(VertexAttrib::Attribute::Color1, static_cast<uint8_t>(3), VertexAttrib::Type::Uint8);
	// second material id
	decl.emplace_back(VertexAttrib::Attribute::Color2, static_cast<uint8_t>(3), VertexAttrib::Type::Uint8);
	// material blend coefficient
	decl.emplace_back(VertexAttrib::Attribute::TexCoord2, static_cast<uint8_t>(3), VertexAttrib::Type::Uint8, true);
	// light level, align to 4 bytes
	decl.emplace_back(VertexAttrib::Attribute::Color0, static_cast<uint8_t>(4), VertexAttrib::Type::Uint8, true);
	// water alpha
	decl.emplace_back(VertexAttrib::Attribute::Color3, static_cast<uint8_t>(1), VertexAttrib::Type::Float, true);

	auto verts = buildVertexList(island);

	auto vertexBuffer = new VertexBuffer("LandBlock", verts, decl);
	_mesh = std::make_unique<Mesh>(vertexBuffer);

	_dynamicsMeshInterface =
	    std::make_unique<dynamics::LandBlockBulletMeshInterface>(verts->data, verts->size, vertexBuffer->GetStrideBytes());

	_physicsMesh = std::make_unique<btBvhTriangleMeshShape>(_dynamicsMeshInterface.get(), true);
	_rigidBody = std::make_unique<btRigidBody>(0.0f, nullptr, _physicsMesh.get());
	btTransform transform;
	transform.setIdentity();
	transform.setOrigin(btVector3(_block->mapX, 0, _block->mapZ));
	_rigidBody->setWorldTransform(transform);
	_rigidBody->setContactStiffnessAndDamping(300, 10);
	_rigidBody->setUserIndex(-1);
}

const bgfx::Memory* LandBlock::buildVertexList(LandIsland& island)
{
	// reserve 16*16 quads of 2 tris with 3 verts = 1536
	const bgfx::Memory* verticesMem = bgfx::alloc(sizeof(LandVertex) * 1536);
	auto vertices = (LandVertex*)verticesMem->data;

	auto countries = island.GetCountries();

	// auto neighbourBlockR = island.GetBlock(glm::u8vec2(_block->blockX + 1, _block->blockZ));
	// auto neighbourBlockUp = island.GetBlock(glm::u8vec2(_block->blockX, _block->blockZ + 1));

	// we'll loop through each cell, 16x16
	// (the array is 17x17 but the 17th block is questionable data)

	int bx = _block->blockX * 16;
	int bz = _block->blockZ * 16;

	uint16_t i = 0;
	for (int x = 0; x < 16; x++)
	{
		for (int z = 0; z < 16; z++)
		{
			// top left
			auto tl = island.GetCell(glm::u16vec2(bx + x + 0, bz + z + 0));
			// top right
			auto tr = island.GetCell(glm::u16vec2(bx + x + 1, bz + z + 0));
			// bottom left
			auto bl = island.GetCell(glm::u16vec2(bx + x + 0, bz + z + 1));
			// bottom right
			auto br = island.GetCell(glm::u16vec2(bx + x + 1, bz + z + 1));

			// construct positions from cell altitudes
			glm::vec3 pTL((x + 0) * LandIsland::CellSize, tl.altitude * LandIsland::HeightUnit,
			              ((z + 0) * LandIsland::CellSize));
			glm::vec3 pTR((x + 1) * LandIsland::CellSize, tr.altitude * LandIsland::HeightUnit,
			              ((z + 0) * LandIsland::CellSize));
			glm::vec3 pBL((x + 0) * LandIsland::CellSize, bl.altitude * LandIsland::HeightUnit,
			              ((z + 1) * LandIsland::CellSize));
			glm::vec3 pBR((x + 1) * LandIsland::CellSize, br.altitude * LandIsland::HeightUnit,
			              ((z + 1) * LandIsland::CellSize));

			auto tlMat = countries[tl.properties.country].materials[tl.altitude + island.GetNoise(bx + x + 0, bz + z + 0)];
			auto trMat = countries[tr.properties.country].materials[tr.altitude + island.GetNoise(bx + x + 1, bz + z + 0)];
			auto blMat = countries[bl.properties.country].materials[bl.altitude + island.GetNoise(bx + x + 0, bz + z + 1)];
			auto brMat = countries[br.properties.country].materials[br.altitude + island.GetNoise(bx + x + 1, bz + z + 1)];

			// TODO: this is temporary way for drawing landscape, should be moved to the renderer
			// use a lambda so we're not repeating ourselves
			auto getAlpha = [](lnd::LNDCell::Properties properties) {
				if (properties.hasWater || properties.fullWater)
					return 0.0f;
				if (properties.coastLine)
					return 0.5f;
				return 1.0f;
			};
			auto make_vert = [&getAlpha](const glm::vec3& height, const glm::vec3& weight, lnd::LNDMapMaterial m[3],
			                             const lnd::LNDCell& cell) -> LandVertex {
				uint32_t mat[6] = {m[0].indices[0], m[1].indices[0], m[2].indices[0],
				                   m[0].indices[1], m[1].indices[1], m[2].indices[1]};
				uint32_t blend[3] = {m[0].coefficient, m[1].coefficient, m[2].coefficient};
				return LandVertex(height, weight, mat, blend, cell.luminosity, getAlpha(cell.properties));
			};

			// cell splitting
			// winding order = clockwise
			if (!tl.properties.split)
			{
				// TR/BR/TL  # #
				//             #
				lnd::LNDMapMaterial trbrtl[3] = {tlMat, trMat, brMat};
				vertices[i++] = make_vert(pTR, glm::vec3(0, 1, 0), trbrtl, tr);
				vertices[i++] = make_vert(pBR, glm::vec3(0, 0, 1), trbrtl, br);
				vertices[i++] = make_vert(pTL, glm::vec3(1, 0, 0), trbrtl, tl);

				// BR/BL/TL  #
				//           # #
				lnd::LNDMapMaterial brbltl[3] = {tlMat, blMat, brMat};
				vertices[i++] = make_vert(pBR, glm::vec3(0, 0, 1), brbltl, br);
				vertices[i++] = make_vert(pBL, glm::vec3(0, 1, 0), brbltl, bl);
				vertices[i++] = make_vert(pTL, glm::vec3(1, 0, 0), brbltl, tl);
			}
			else
			{
				// BL/TL/TR  # #
				//           #
				lnd::LNDMapMaterial bltltr[3] = {blMat, tlMat, trMat};
				vertices[i++] = make_vert(pBL, glm::vec3(1, 0, 0), bltltr, bl);
				vertices[i++] = make_vert(pTL, glm::vec3(0, 1, 0), bltltr, tl);
				vertices[i++] = make_vert(pTR, glm::vec3(0, 0, 1), bltltr, tr);

				// TR/BR/BL    #
				//           # #
				lnd::LNDMapMaterial trbrbl[3] = {blMat, brMat, trMat};
				vertices[i++] = make_vert(pTR, glm::vec3(0, 0, 1), trbrbl, tr);
				vertices[i++] = make_vert(pBR, glm::vec3(0, 1, 0), trbrbl, br);
				vertices[i++] = make_vert(pBL, glm::vec3(1, 0, 0), trbrbl, bl);
			}
		}
	}

	return verticesMem;
}

void LandBlock::Draw(graphics::RenderPass viewId, const ShaderProgram& program, bool cullBack) const
{
	glm::vec4 mapPosition = glm::vec4(_block->mapX, _block->mapZ, 0, 0);
	program.SetUniformValue("u_blockPosition", &mapPosition);

	constexpr auto defaultState = BGFX_STATE_WRITE_MASK | BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;

	Mesh::DrawDesc desc = {
	    /*viewId =*/viewId,
	    /*program =*/program,
	    /*count =*/_mesh->GetVertexBuffer().GetCount(),
	    /*offset =*/0,
	    /*instanceBuffer =*/nullptr,
	    /*instanceStart =*/0,
	    /*instanceCount =*/1,
	    /*state =*/defaultState | (cullBack ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW),
	    /*rgba =*/0,
	    /*skip =*/Mesh::SkipState::SkipNone,
	    /*preserveState =*/false,
	};
	_mesh->Draw(desc);
}

const lnd::LNDCell* LandBlock::GetCells() const
{
	assert(_block);
	return _block ? _block->cells : nullptr;
}

glm::ivec2 LandBlock::GetBlockPosition() const
{
	assert(_block);
	return {_block ? _block->blockX : -1, _block ? _block->blockZ : -1};
}

glm::vec4 LandBlock::GetMapPosition() const
{
	assert(_block);
	return glm::vec4(_block->mapX, _block->mapZ, 0, 0);
}
