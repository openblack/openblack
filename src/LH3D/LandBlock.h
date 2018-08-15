#pragma once
#ifndef OPENBLACK_LANDBLOCK_H
#define OPENBLACK_LANDBLOCK_H

#include <stdint.h>
#include <Graphics/OpenGL.h>
#include <glm/glm.hpp>

#include "LandCell.h"

#define OPENBLACK_LANDBLOCK_TOTAL_CELLS 289 // 17 * 17

namespace OpenBlack
{
	class LandBlock
	{
	public:
		LandBlock();
		~LandBlock();

		void Draw();
	private:
		LandCell mCells[OPENBLACK_LANDBLOCK_TOTAL_CELLS];
		uint32_t index;
		glm::ivec2 blockPosition;
		glm::vec2 mapPosition;
	private:
		GLuint mVertexArray;
		GLuint mVertexBuffer;
		GLuint mElementBuffer;
	};
}

#endif
