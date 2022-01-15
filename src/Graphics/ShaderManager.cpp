/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "ShaderManager.h"

#include <cstdint> // Shaders below need uint8_t

#define SHADER_NAME vs_line
#include "ShaderIncluder.h"
#define SHADER_NAME vs_line_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME fs_line
#include "ShaderIncluder.h"

#define SHADER_NAME vs_object
#include "ShaderIncluder.h"
#define SHADER_NAME vs_object_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME fs_object
#include "ShaderIncluder.h"
#define SHADER_NAME fs_sky
#include "ShaderIncluder.h"

#define SHADER_NAME vs_terrain
#include "ShaderIncluder.h"
#define SHADER_NAME fs_terrain
#include "ShaderIncluder.h"

#define SHADER_NAME vs_water
#include "ShaderIncluder.h"
#define SHADER_NAME fs_water
#include "ShaderIncluder.h"

#include "3D/Camera.h"

#include <bgfx/embedded_shader.h>

namespace openblack::graphics
{

const bgfx::EmbeddedShader s_embeddedShaders[] = {
    BGFX_EMBEDDED_SHADER(vs_line),    BGFX_EMBEDDED_SHADER(vs_line_instanced),   //
    BGFX_EMBEDDED_SHADER(fs_line),                                               //
    BGFX_EMBEDDED_SHADER(vs_object),  BGFX_EMBEDDED_SHADER(vs_object_instanced), //
    BGFX_EMBEDDED_SHADER(fs_object),  BGFX_EMBEDDED_SHADER(fs_sky),              //
    BGFX_EMBEDDED_SHADER(vs_terrain), BGFX_EMBEDDED_SHADER(fs_terrain),          //
    BGFX_EMBEDDED_SHADER(vs_water),   BGFX_EMBEDDED_SHADER(fs_water),            //
    BGFX_EMBEDDED_SHADER_END()                                                   //
};

ShaderManager::~ShaderManager()
{
	// delete all mapped shaders
	ShaderMap::iterator iter;
	for (iter = _shaderPrograms.begin(); iter != _shaderPrograms.end(); ++iter) delete iter->second;

	_shaderPrograms.clear();
}

const ShaderProgram* ShaderManager::LoadShader(const std::string& name, const std::string& vertexShaderName,
                                               const std::string& fragmentShaderName)
{
	bgfx::RendererType::Enum type = bgfx::getRendererType();

	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	ShaderProgram* program =
	    new ShaderProgram(name, bgfx::createEmbeddedShader(s_embeddedShaders, type, vertexShaderName.c_str()),
	                      bgfx::createEmbeddedShader(s_embeddedShaders, type, fragmentShaderName.c_str()));
	_shaderPrograms[name] = program;
	return program;
}

const ShaderProgram* ShaderManager::GetShader(const std::string& name) const
{
	auto i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	// todo: return an empty shader?
	return nullptr;
}

void ShaderManager::SetCamera(graphics::RenderPass viewId, const Camera& camera)
{
	auto view = camera.GetViewMatrix();
	auto proj = camera.GetProjectionMatrix();
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(viewId), &view, &proj);
}

} // namespace openblack::graphics
