/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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

class ShaderManagerInterface
{
public:
	virtual void LoadShaders() = 0;
	[[nodiscard]] virtual const ShaderProgram* GetShader(const std::string& name) const = 0;

	virtual void SetCamera(RenderPass viewId, const Camera& camera) = 0;
};

} // namespace graphics
} // namespace openblack
