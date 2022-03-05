/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "RenderPass.h"

#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include <memory>

namespace openblack::graphics
{
class Mesh;

class DebugLines
{
public:
	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 col;
	};

	static std::unique_ptr<Mesh> CreateCross();
	static std::unique_ptr<Mesh> CreateBox(const glm::vec4& color);
	static std::unique_ptr<Mesh> CreateLine(const glm::vec4& from, const glm::vec4& to, const glm::vec4& color);
	static std::unique_ptr<Mesh> CreateDebugLines(const Vertex* data, uint32_t vertexCount);

protected:
	DebugLines() = delete;
};

} // namespace openblack::graphics
