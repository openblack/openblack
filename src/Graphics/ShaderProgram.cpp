/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ShaderProgram.h"

#include <spdlog/spdlog.h>

#include "FileSystem/FileSystemInterface.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Texture2D.h"

namespace openblack::graphics
{

ShaderProgram::ShaderProgram(const std::string& name, ShaderHandle vertexShader, ShaderHandle fragmentShader)
    : _name(name)
    , _program(BGFX_INVALID_HANDLE)
{
	uint16_t numShaderUniforms = 0;
	bgfx::UniformInfo info = {};
	std::vector<bgfx::UniformHandle> uniforms;

	numShaderUniforms = bgfx::getShaderUniforms(toBgfx(vertexShader));
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(toBgfx(vertexShader), uniforms.data(), numShaderUniforms);
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), fromBgfx(uniforms[i]));
	}

	numShaderUniforms = bgfx::getShaderUniforms(toBgfx(fragmentShader));
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(toBgfx(fragmentShader), uniforms.data(), numShaderUniforms);
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), fromBgfx(uniforms[i]));
	}

	_program = fromBgfx(bgfx::createProgram(toBgfx(vertexShader), toBgfx(fragmentShader), true));
	bgfx::setName(toBgfx(vertexShader), (name + "_vs").c_str());
	bgfx::setName(toBgfx(fragmentShader), (name + "_fs").c_str());
	bgfx::frame();
}

ShaderProgram::~ShaderProgram()
{
	if (bgfx::isValid(toBgfx(_program)))
	{
		bgfx::destroy(toBgfx(_program));
	}
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture) const
{
	auto uniform = _uniforms.find(samplerName);
	if (uniform != _uniforms.cend())
	{
		bgfx::setTexture(bindPoint, toBgfx(uniform->second), toBgfx(texture.GetNativeHandle()));
	}
	else
	{
		WarnMissing(samplerName);
	}
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const graphics::TextureHandle& texture) const
{
	auto uniform = _uniforms.find(samplerName);
	if (uniform != _uniforms.cend())
	{
		bgfx::setTexture(bindPoint, toBgfx(uniform->second), toBgfx(texture));
	}
	else
	{
		WarnMissing(samplerName);
	}
}

void ShaderProgram::SetUniformArray(const char* uniformName, const void* values, uint16_t count) const
{
	if (count == 0)
	{
		return;
	}
	if (const auto uniform = _uniforms.find(uniformName); uniform != _uniforms.cend())
	{
		bgfx::setUniform(toBgfx(uniform->second), values, count);
	}
	else
	{
		WarnMissing(uniformName);
	}
}

void ShaderProgram::SetUniformValue(const char* uniformName, const void* value) const
{
	auto uniform = _uniforms.find(uniformName);
	if (uniform != _uniforms.cend())
	{
		bgfx::setUniform(toBgfx(uniform->second), value);
	}
	else
	{
		WarnMissing(uniformName);
	}
}

void ShaderProgram::WarnMissing(std::string_view name) const
{
	// Uniforms are set every frame, so each missing one is warned about once
	if (_warnedMissing.emplace(name).second)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Could not find uniform {} in {} Shader", name, _name);
	}
}

} // namespace openblack::graphics
