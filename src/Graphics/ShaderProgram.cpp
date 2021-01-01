/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ShaderProgram.h"

#include "Common/FileSystem.h"
#include "Game.h"
#include "Texture2D.h"

#include <spdlog/spdlog.h>

namespace openblack::graphics
{

ShaderProgram::ShaderProgram(const std::string& name, bgfx::ShaderHandle&& vertexShader, bgfx::ShaderHandle&& fragmentShader)
    : _program(BGFX_INVALID_HANDLE)
{
	uint16_t numShaderUniforms = 0;
	bgfx::UniformInfo info = {};
	std::vector<bgfx::UniformHandle> uniforms;

	numShaderUniforms = bgfx::getShaderUniforms(vertexShader);
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(vertexShader, uniforms.data(), uniforms.size());
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), uniforms[i]);
	}

	numShaderUniforms = bgfx::getShaderUniforms(fragmentShader);
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(fragmentShader, uniforms.data(), uniforms.size());
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.emplace(std::string(info.name), uniforms[i]);
	}

	_program = bgfx::createProgram(vertexShader, fragmentShader, true);
	bgfx::frame();
}

ShaderProgram::~ShaderProgram()
{
	if (bgfx::isValid(_program))
		bgfx::destroy(_program);
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const Texture2D& texture) const
{
	auto uniform = _uniforms.find(samplerName);
	if (uniform != _uniforms.cend())
	{
		bgfx::setTexture(bindPoint, uniform->second, texture.GetNativeHandle());
	}
	else
	{
		spdlog::warn("Could not find texture sampler {}", samplerName);
	}
}

void ShaderProgram::SetUniformValue(const char* uniformName, const void* value) const
{
	auto uniform = _uniforms.find(uniformName);
	if (uniform != _uniforms.cend())
	{
		bgfx::setUniform(uniform->second, value);
	}
	else
	{
		spdlog::warn("Could not find uniform {}", uniformName);
	}
}

} // namespace openblack::graphics
