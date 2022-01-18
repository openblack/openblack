/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <bgfx/bgfx.h>

#include <cstdint>
#include <map>
#include <string>

namespace openblack::graphics
{
class Texture2D;

class ShaderProgram
{
public:
	enum class Type
	{
		Vertex,
		Fragment,
		Compute,
	};

	ShaderProgram() = delete;
	ShaderProgram(const std::string& name, bgfx::ShaderHandle&& vertexShader, bgfx::ShaderHandle&& fragmentShader);
	~ShaderProgram();

	void SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture) const;
	void SetUniformValue(const char* uniformName, const void* value) const;

	[[nodiscard]] bgfx::ProgramHandle GetRawHandle() const { return _program; }

private:
	std::string _name;
	bgfx::ProgramHandle _program;
	std::map<std::string, bgfx::UniformHandle> _uniforms;
};

} // namespace openblack::graphics
