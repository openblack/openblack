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
#include "ShaderProgram.h"

#include <map>
#include <string>

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

	const ShaderProgram* LoadShader(const std::string& name, const std::string& vertexShaderName,
	                                const std::string& fragmentShaderName);
	[[nodiscard]] const ShaderProgram* GetShader(const std::string& name) const;

	void SetCamera(RenderPass viewId, const Camera& camera);

private:
	typedef std::map<std::string, const ShaderProgram*> ShaderMap;

	ShaderMap _shaderPrograms;
};

} // namespace graphics
} // namespace openblack
