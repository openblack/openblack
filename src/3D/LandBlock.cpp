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
	float iv_lrs; // lowrestex y
	uint32_t smallTextUpdated;
};

void LandBlock::LoadFromFile(void* block, size_t block_size)
{
	if (block_size != sizeof(LH3DLandBlock))
	{
		throw new std::runtime_error("LandBlock size does not match struct size.");
	}

	auto lhBlock = static_cast<LH3DLandBlock*>(block);

	m_index = lhBlock->index;
	m_blockPosition = glm::ivec2(lhBlock->blockX, lhBlock->blockY);
	m_mapPosition = glm::vec2(lhBlock->mapX, lhBlock->mapY);

	// this should just work, not graceful lol
	memcpy(&m_cells, lhBlock->cells, 17 * 17 * sizeof(LH3DLandCell));
}
