/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <map>
#include <string>

#include "GraphicsHandle.h"

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
	ShaderProgram(const std::string& name, ShaderHandle vertexShader, ShaderHandle fragmentShader);
	~ShaderProgram();

	void SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture) const;
	void SetTextureSampler(const char* samplerName, uint8_t bindPoint, const graphics::TextureHandle& texture) const;
	void SetUniformValue(const char* uniformName, const void* value) const;

	[[nodiscard]] ProgramHandle GetRawHandle() const { return _program; }

private:
	std::string _name;
	ProgramHandle _program;
	std::map<std::string, UniformHandle> _uniforms;
};

} // namespace openblack::graphics
