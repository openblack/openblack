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

#include "LandBlock.h"

#include <glm/gtc/type_ptr.hpp>

using namespace OpenBlack;

LandBlock::LandBlock()
{

}

struct LH3DLandCell // 8 bytes
{
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t l;

	uint8_t altitude;
	uint8_t savecolor;

	uint8_t flags1;
	uint8_t flags2;
};

struct LH3DLandBlock
{
	LH3DLandCell cells[289]; // 17*17
	uint32_t index; // 2312
	float mapY; // 2316
	float mapX; // 2320
	uint32_t blockY; // 2324
	uint32_t blockX; // 2328
	uint32_t clipped; // 2332 0
	uint32_t frameVisibility; // 0
	uint32_t highestAltitude; // 0
	uint32_t useSmallBump; // 0
	uint32_t forceLowResTex; // 0
	uint32_t meshLOD;// 0
	uint32_t meshBlending; // 0
	uint32_t textureBlend; // 0
	uint32_t meshLODType; // 0
	uint32_t fog; // 0
	uint32_t texPointer; // 0 if LH3DIsland::g_b_use_always_low_texture, else new Tex
	uint32_t matPointer; // 0 if LH3DIsland::g_b_use_always_low_texture, else new Mat(Tex)
	uint32_t drawSomething; // 0aaa
	uint32_t specMatBeforePtr; // 0
	uint32_t specMatAfterPtr; // 0
	float transformUVBefore[3][4];
	float transformUVAfter[3][4];
	uint32_t nextSortingPtr;
	float valueSorting;
	float lowResTexture;
	float fu_lrs; // (iu_lrs / 256)
	float fv_lrs; // (iv_lrs / 256)
	float iu_lrs; // lowrestex x
	float iv_lrs; // lowrestex z
	uint32_t smallTextUpdated;
};

void LandBlock::LoadFromFile(void* block, size_t block_size)
{
	if (block_size != sizeof(LH3DLandBlock))
	{
		throw new std::runtime_error("LandBlock size does not match struct size.");
	}

	auto lhBlock = static_cast<LH3DLandBlock*>(block);

	_index = lhBlock->index;
	_blockPosition = glm::ivec2(lhBlock->blockX, lhBlock->blockY);
	_mapPosition = glm::vec2(lhBlock->mapX, lhBlock->mapY);

	// this should just work, not graceful lol
	memcpy(&_cells, lhBlock->cells, 17 * 17 * sizeof(LH3DLandCell));
}

void LandBlock::buildMesh()
{
	if (_mesh != nullptr)
		_mesh.reset();

	VertexDecl decl(8);
	decl[1] = VertexAttrib(1, 3, GL_FLOAT, false, false, sizeof(LandBlockVertex), (void*)offsetof(LandBlockVertex, position)); // position
	decl[3] = VertexAttrib(3, 3, GL_FLOAT, false, false, sizeof(LandBlockVertex), (void*)offsetof(LandBlockVertex, weight));
	decl[4] = VertexAttrib(4, 3, GL_UNSIGNED_BYTE, true, false, sizeof(LandBlockVertex), (void*)offsetof(LandBlockVertex, firstMaterialID));
	decl[5] = VertexAttrib(5, 3, GL_UNSIGNED_BYTE, true, false, sizeof(LandBlockVertex), (void*)offsetof(LandBlockVertex, secondMaterialID));
	decl[6] = VertexAttrib(6, 3, GL_UNSIGNED_BYTE, false, true, sizeof(LandBlockVertex), (void*)offsetof(LandBlockVertex, materialBlendCoefficient));
	decl[7] = VertexAttrib(7, 1, GL_FLOAT, false, true, sizeof(LandBlockVertex), (void*)offsetof(LandBlockVertex, alpha));

	auto verts = buildVertexList();

	VertexBuffer* vertexBuffer = new VertexBuffer(verts.data(), verts.size(), sizeof(LandBlockVertex));
	_mesh = std::make_unique<Mesh>(vertexBuffer, decl, GL_TRIANGLES);
}


std::vector<LandBlockVertex> LandBlock::buildVertexList()
{
	// reserve 16*16 quads of 2 tris with 3 verts = 1536
	std::vector<LandBlockVertex> verts(1536);

	for (int x = 0; x < 16; x++)
	{
		for (int z = 0; z < 16; z++)
		{			
			LandCell tl = _cells[(z + 0) * 17 + (x + 0)];
			LandCell tr = _cells[(z + 0) * 17 + (x + 1)];
			LandCell bl = _cells[(z + 1) * 17 + (x + 0)];
			LandCell br = _cells[(z + 1) * 17 + (x + 1)];

			glm::vec3 pTL((x + 0) * 10.0f, tl.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, ((z + 0) * 10.0f));
			glm::vec3 pTR((x + 1) * 10.0f, tr.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, ((z + 0) * 10.0f));
			glm::vec3 pBL((x + 0) * 10.0f, bl.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, ((z + 1) * 10.0f));
			glm::vec3 pBR((x + 1) * 10.0f, br.Altitude() * OPENBLACK_LANDISLAND_HEIGHT_UNIT, ((z + 1) * 10.0f));

			//auto tlMat = _countries[tl.Country()].MapMaterials[tl.Altitude()];
			//auto trMat = _countries[tr.Country()].MapMaterials[tr.Altitude()];
			//auto blMat = _countries[bl.Country()].MapMaterials[bl.Altitude()];
			//auto brMat = _countries[br.Country()].MapMaterials[br.Altitude()];

			// triangle one: TL -> TR -> BR
			verts.push_back(LandVertex(tl.Color(), pTL, uvTL, glm::vec3(1.0f, 0.0f, 0.0f),
				tlMat.FirstMaterialIndex, trMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
				tlMat.SecondMaterialIndex, trMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
				tlMat.Coeficient, trMat.Coeficient, brMat.Coeficient,
				tl.Alpha()
			));
			verts.push_back(LandVertex(tr.Color(), pTR, uvTR, glm::vec3(0.0f, 1.0f, 0.0f),
				tlMat.FirstMaterialIndex, trMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
				tlMat.SecondMaterialIndex, trMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
				tlMat.Coeficient, trMat.Coeficient, brMat.Coeficient,
				tr.Alpha()
			));
			verts.push_back(LandVertex(br.Color(), pBR, uvBR, glm::vec3(0.0f, 0.0f, 1.0f),
				tlMat.FirstMaterialIndex, trMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
				tlMat.SecondMaterialIndex, trMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
				tlMat.Coeficient, trMat.Coeficient, brMat.Coeficient,
				br.Alpha()
			));

			// triangle two: BR -> BL -> TL
			verts.push_back(LandVertex(br.Color(), pBR, uvBR, glm::vec3(0.0f, 0.0f, 1.0f),
				tlMat.FirstMaterialIndex, blMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
				tlMat.SecondMaterialIndex, blMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
				tlMat.Coeficient, blMat.Coeficient, brMat.Coeficient,
				br.Alpha()
			));
			verts.push_back(LandVertex(bl.Color(), pBL, uvBL, glm::vec3(0.0f, 1.0f, 0.0f),
				tlMat.FirstMaterialIndex, blMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
				tlMat.SecondMaterialIndex, blMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
				tlMat.Coeficient, blMat.Coeficient, brMat.Coeficient,
				bl.Alpha()
			));
			verts.push_back(LandVertex(tl.Color(), pTL, uvTL, glm::vec3(1.0f, 0.0f, 0.0f),
				tlMat.FirstMaterialIndex, blMat.FirstMaterialIndex, brMat.FirstMaterialIndex,
				tlMat.SecondMaterialIndex, blMat.SecondMaterialIndex, brMat.SecondMaterialIndex,
				tlMat.Coeficient, blMat.Coeficient, brMat.Coeficient,
				tl.Alpha()
			));
		}
	}

	return verts;
}
