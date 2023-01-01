/*****************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <map>
#include <string>

#include "RenderPass.h"
#include "ShaderProgram.h"

namespace openblack
{

class Camera;

namespace graphics
{

class ShaderManager
{
public:
	ShaderManager() = default;
	~ShaderManager();

	void LoadShaders();
	[[nodiscard]] const ShaderProgram* GetShader(const std::string& name) const;

	void SetCamera(RenderPass viewId, const Camera& camera);

private:
	using ShaderMap = std::map<std::string, const ShaderProgram*>;

	ShaderMap _shaderPrograms;
};

} // namespace graphics
} // namespace openblack
