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

#include "LandCell.h"

#define OPENBLACK_LANDBLOCK_TOTAL_CELLS 289

namespace OpenBlack
{
	class LandBlock
	{
	public:
		LandBlock();

		void LoadFromFile(void* block, size_t block_size);
		LandCell* GetCells() {
			return m_cells;
		};
		glm::ivec2* GetBlockPosition() { return &m_blockPosition; }
		glm::vec2* GetMapPosition() { return &m_mapPosition; }
	private:
		LandCell m_cells[OPENBLACK_LANDBLOCK_TOTAL_CELLS];
		uint32_t m_index;
		glm::ivec2 m_blockPosition;
		glm::vec2 m_mapPosition;
	};
}

#endif
