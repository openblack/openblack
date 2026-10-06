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
#include <set>
#include <string>
#include <string_view>

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
	/// The first count elements of an array uniform
	void SetUniformArray(const char* uniformName, const void* values, uint16_t count) const;
	[[nodiscard]] bool HasUniform(const char* uniformName) const { return _uniforms.contains(uniformName); }

	[[nodiscard]] ProgramHandle GetRawHandle() const { return _program; }

private:
	void WarnMissing(std::string_view name) const;

	std::string _name;
	ProgramHandle _program;
	std::map<std::string, UniformHandle> _uniforms;
	/// The uniforms set on this shader that it doesn't have, already warned about
	mutable std::set<std::string, std::less<>> _warnedMissing;
};

} // namespace openblack::graphics
