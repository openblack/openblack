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
#include "SamplerDefaults.h"
#include "Texture2D.h"

namespace openblack::graphics
{

static_assert(ShaderProgram::k_DiscardAll == BGFX_DISCARD_ALL);

ShaderProgram::ShaderProgram(const std::string& name, ShaderHandle vertexShader, ShaderHandle fragmentShader,
                             std::vector<shader_samplers::Sampler> samplers, SamplerDefaults& samplerDefaults)
    : _name(name)
    , _program(BGFX_INVALID_HANDLE)
    , _samplers(std::move(samplers))
    , _samplerDefaults(samplerDefaults)
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
		_uniforms.Add(info.name, fromBgfx(uniforms[i]));
	}

	numShaderUniforms = bgfx::getShaderUniforms(toBgfx(fragmentShader));
	uniforms.resize(numShaderUniforms);
	bgfx::getShaderUniforms(toBgfx(fragmentShader), uniforms.data(), numShaderUniforms);
	for (uint16_t i = 0; i < numShaderUniforms; ++i)
	{
		bgfx::getUniformInfo(uniforms[i], info);
		_uniforms.Add(info.name, fromBgfx(uniforms[i]));
	}

	// Two samplers at one stage would share a texture
	for (const auto& [first, second] : shader_samplers::StageCollisions(_samplers))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "{} Shader samples {} and {} at the same stage", name, first, second);
	}
	// A sampler the backend's build of the shader doesn't have can't be bound, nor needs to be
	std::erase_if(_samplers, [this](const shader_samplers::Sampler& sampler) { return !_uniforms.Contains(sampler.name); });
	for ([[maybe_unused]] const auto& sampler : _samplers)
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("graphics"), "{} Shader samples {} at stage {} (kind {})", name, sampler.name,
		                    sampler.stage, static_cast<int>(sampler.dimension));
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
	if (const auto uniform = _uniforms.Find(samplerName))
	{
		bgfx::setTexture(bindPoint, toBgfx(*uniform), toBgfx(texture.GetNativeHandle()));
		_samplerDefaults.Set(bindPoint);
	}
	else
	{
		WarnMissing(samplerName);
	}
}

void ShaderProgram::SetTextureSampler(const char* samplerName, uint8_t bindPoint, const graphics::TextureHandle& texture) const
{
	if (const auto uniform = _uniforms.Find(samplerName))
	{
		bgfx::setTexture(bindPoint, toBgfx(*uniform), toBgfx(texture));
		_samplerDefaults.Set(bindPoint);
	}
	else
	{
		WarnMissing(samplerName);
	}
}

void ShaderProgram::SetTextureSampler(UniformHandle sampler, uint8_t bindPoint, const Texture2D& texture) const
{
	bgfx::setTexture(bindPoint, toBgfx(sampler), toBgfx(texture.GetNativeHandle()));
	_samplerDefaults.Set(bindPoint);
}

void ShaderProgram::SetTextureSampler(UniformHandle sampler, uint8_t bindPoint, const graphics::TextureHandle& texture) const
{
	bgfx::setTexture(bindPoint, toBgfx(sampler), toBgfx(texture));
	_samplerDefaults.Set(bindPoint);
}

void ShaderProgram::SetUniformValue(UniformHandle uniform, const void* value) const
{
	bgfx::setUniform(toBgfx(uniform), value);
}

void ShaderProgram::Submit(uint16_t viewId, uint32_t depth, uint8_t discardFlags) const
{
	for (const auto& sampler : _samplers)
	{
		if (!_samplerDefaults.IsSet(sampler.stage))
		{
			const auto texture = _samplerDefaults.Texture(shader_samplers::DefaultTextureFor(sampler.dimension));
			bgfx::setTexture(sampler.stage, toBgfx(*_uniforms.Find(sampler.name)), toBgfx(texture));
			// Kept for the next draw as well when this one keeps its bindings
			_samplerDefaults.Set(sampler.stage);
		}
	}
	bgfx::submit(viewId, toBgfx(_program), depth, discardFlags);
	_samplerDefaults.Discarded(discardFlags);
}

void ShaderProgram::SetUniformArray(const char* uniformName, const void* values, uint16_t count) const
{
	if (count == 0)
	{
		return;
	}
	if (const auto uniform = _uniforms.Find(uniformName))
	{
		bgfx::setUniform(toBgfx(*uniform), values, count);
	}
	else
	{
		WarnMissing(uniformName);
	}
}

void ShaderProgram::SetUniformValue(const char* uniformName, const void* value) const
{
	if (const auto uniform = _uniforms.Find(uniformName))
	{
		bgfx::setUniform(toBgfx(*uniform), value);
	}
	else
	{
		WarnMissing(uniformName);
	}
}

void ShaderProgram::WarnMissing(std::string_view name) const
{
	// Uniforms are set every frame, so each missing one is warned about once
	if (!_warnedMissing.contains(name) && _warnedMissing.emplace(name).second)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Could not find uniform {} in {} Shader", name, _name);
	}
}

} // namespace openblack::graphics
