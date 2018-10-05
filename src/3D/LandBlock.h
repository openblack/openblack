#pragma once
#ifndef OPENBLACK_LANDBLOCK_H
#define OPENBLACK_LANDBLOCK_H

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
		glm::vec2* GetMapPosition() { return &m_mapPosition; }
	private:
		LandCell m_cells[OPENBLACK_LANDBLOCK_TOTAL_CELLS];
		uint32_t m_index;
		glm::ivec2 m_blockPosition;
		glm::vec2 m_mapPosition;
	};
}

#endif
