/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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
class ShaderProgram;

class DebugLines
{
public:
	struct Vertex
	{
		glm::vec4 pos;
		glm::vec4 col;
	};

	static std::unique_ptr<DebugLines> CreateCross();
	static std::unique_ptr<DebugLines> CreateBox(const glm::vec4& color);
	static std::unique_ptr<DebugLines> CreateLine(const glm::vec4& from, const glm::vec4& to, const glm::vec4& color);
	static std::unique_ptr<DebugLines> CreateDebugLines(const Vertex* data, uint32_t vertexCount);

	virtual ~DebugLines();

	void Draw(graphics::RenderPass viewId, const ShaderProgram& program) const;
	void Draw(graphics::RenderPass viewId, const bgfx::DynamicVertexBufferHandle& instanceBuffer, uint32_t instanceStart,
	          uint32_t instanceCount, const ShaderProgram& program) const;

	void SetPose(const glm::vec3& center, const glm::vec3& size);

protected:
	explicit DebugLines(std::unique_ptr<Mesh>&& mesh);

	std::unique_ptr<Mesh> _mesh;
	glm::mat4 _model;
};

} // namespace openblack::graphics
