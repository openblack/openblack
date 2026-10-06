/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <string>

#include "RenderPass.h"
#include "ShaderProgram.h"

namespace openblack
{

class Camera;

namespace graphics
{

class SamplerDefaults;

class ShaderManager
{
public:
	ShaderManager();
	~ShaderManager();

	void LoadShaders();
	[[nodiscard]] const ShaderProgram* GetShader(const std::string& name) const;

	/// Drops the textures bound for the next draws, which a run of draws that kept them leaves bound
	void DiscardBindings() const;
	/// bgfx dropped every binding with the frame
	void FrameEnded() const;

	void SetCamera(RenderPass viewId, const Camera& camera);

private:
	using ShaderMap = std::map<std::string, const ShaderProgram*>;

	ShaderMap _shaderPrograms;
	/// The textures bound to the samplers draws leave unset, shared by every program
	std::unique_ptr<SamplerDefaults> _samplerDefaults;
};

} // namespace graphics
} // namespace openblack
