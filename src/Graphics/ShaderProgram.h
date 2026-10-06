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

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "GraphicsHandle.h"
#include "ShaderSamplers.h"
#include "UniformTable.h"

namespace openblack::graphics
{
class SamplerDefaults;
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
	/// The samplers are those the program's shaders declare, which it keeps bound on every submit with the defaults
	ShaderProgram(const std::string& name, ShaderHandle vertexShader, ShaderHandle fragmentShader,
	              std::vector<shader_samplers::Sampler> samplers, SamplerDefaults& samplerDefaults);
	~ShaderProgram();
	ShaderProgram(const ShaderProgram&) = delete;
	ShaderProgram& operator=(const ShaderProgram&) = delete;

	void SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture) const;
	void SetTextureSampler(const char* samplerName, uint8_t bindPoint, const graphics::TextureHandle& texture) const;
	void SetUniformValue(const char* uniformName, const void* value) const;
	/// The first count elements of an array uniform
	void SetUniformArray(const char* uniformName, const void* values, uint16_t count) const;
	[[nodiscard]] bool HasUniform(const char* uniformName) const { return _uniforms.Contains(uniformName); }
	/// A uniform's handle, for the draws that set it again and again without looking it up by name each time
	[[nodiscard]] std::optional<UniformHandle> FindUniform(std::string_view uniformName) const
	{
		return _uniforms.Find(uniformName);
	}
	/// The same as by name, with a handle of this program's from FindUniform
	void SetTextureSampler(UniformHandle sampler, uint8_t bindPoint, const Texture2D& texture) const;
	void SetTextureSampler(UniformHandle sampler, uint8_t bindPoint, const graphics::TextureHandle& texture) const;
	void SetUniformValue(UniformHandle uniform, const void* value) const;

	/// Draws with this program in the given view. Each sampler it declares that no texture was set for since the
	/// bindings were last discarded gets a white texture of its kind first, so that no sampler is left unbound.
	void Submit(uint16_t viewId, uint32_t depth = 0, uint8_t discardFlags = k_DiscardAll) const;

	[[nodiscard]] ProgramHandle GetRawHandle() const { return _program; }

	/// bgfx's BGFX_DISCARD_ALL, everything set for a draw dropped after it
	static constexpr uint8_t k_DiscardAll = 0xFF;

private:
	void WarnMissing(std::string_view name) const;

	std::string _name;
	ProgramHandle _program;
	UniformTable _uniforms;
	std::vector<shader_samplers::Sampler> _samplers;
	SamplerDefaults& _samplerDefaults;
	/// The uniforms set on this shader that it doesn't have, already warned about
	mutable std::set<std::string, std::less<>> _warnedMissing;
};

/// One of a program's uniforms looked up by name once, for the draws that set it again and again. Set without the
/// program having it, it is set by name, which warns of it once.
class ProgramUniform
{
public:
	ProgramUniform(const ShaderProgram& program, const char* name)
	    : _program(program)
	    , _name(name)
	    , _handle(program.FindUniform(name))
	{
	}

	void Set(const void* value) const
	{
		if (_handle)
		{
			_program.SetUniformValue(*_handle, value);
		}
		else
		{
			_program.SetUniformValue(_name, value);
		}
	}
	template <typename Texture>
	void Set(uint8_t bindPoint, const Texture& texture) const
	{
		if (_handle)
		{
			_program.SetTextureSampler(*_handle, bindPoint, texture);
		}
		else
		{
			_program.SetTextureSampler(_name, bindPoint, texture);
		}
	}

private:
	const ShaderProgram& _program;
	const char* _name;
	std::optional<UniformHandle> _handle;
};

} // namespace openblack::graphics
