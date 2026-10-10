/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ShaderManager.h"

#include "Camera/Camera.h"

namespace openblack::graphics
{

ShaderManager::~ShaderManager()
{
	ShaderMap::iterator iter;
	for (iter = _shaderPrograms.begin(); iter != _shaderPrograms.end(); ++iter)
	{
		delete iter->second;
	}

	_shaderPrograms.clear();
}

void ShaderManager::LoadShaders()
{
	// The active SDL GPU renderer loads Slang pipelines directly.
}

const ShaderProgram* ShaderManager::GetShader(const std::string& name) const
{
	auto i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
	{
		return i->second;
	}

	// todo: return an empty shader?
	return nullptr;
}

void ShaderManager::SetCamera(graphics::RenderPass, const Camera&)
{
	// The bgfx renderer is inactive; camera transforms are set by the SDL GPU renderer.
}

} // namespace openblack::graphics
