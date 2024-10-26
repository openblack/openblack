/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// How to add a Shader:
// Shaders in openblack are compiled using bgfx's shaderc compiler. Shaderc
// will compile different variations for different rendering APIs and
// platforms. The implementation is in Shaders.cmake.
// Shader sources are located in assets/shaders. The shader language used is a
// subset of glsl made specifically for bgfx.
// Inputs and outputs of shaders (excluding uniforms, textures samplers, etc)
// are declared in varying.def.sc.
// Once compiled they go in ${CMAKE_BINARY_DIR}/include/generated/shaders and
// are included in the openblack binary by way of ShaderManager.cpp.
// The helper header ShaderIncluder.h used with the SHADER_NAME define will
// automatically include all the shader variants in the file.
// Once included, they must be added to the s_embeddedShaders array.
// Finally, the renderer calls the shader manager to load all shaders
// named in the Shaders array.
// tldr:
// 1. Create shaders in assets/shaders
// 2. Define SHADER_NAME and include ShaderIncluder.h
// 3. Add BGFX_EMBEDDED_SHADER entries to s_embeddedShaders
// 4. Add ShaderDefinition to Shaders array

#define LOCATOR_IMPLEMENTATIONS

#include "ShaderManager.h"

#include <cstdint> // Shaders below need uint8_t

#include <bgfx/embedded_shader.h>
// BGFX has support for WSL to use windows d3d. We disable it here from the BGFX_EMBEDDED_SHADER macro.
#if BX_PLATFORM_LINUX
#undef BGFX_EMBEDDED_SHADER_DXBC
#define BGFX_EMBEDDED_SHADER_DXBC(...)
#undef BGFX_EMBEDDED_SHADER_DX9BC
#define BGFX_EMBEDDED_SHADER_DX9BC(...)
#endif

#include "Camera/Camera.h"

// clang-format off
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
#define SHADER_NAME vs_object_hm_instanced
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

#define SHADER_NAME vs_sprite
#include "ShaderIncluder.h"
#define SHADER_NAME fs_sprite
#include "ShaderIncluder.h"

#define SHADER_NAME vs_footprint_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME fs_footprint
#include "ShaderIncluder.h"
// clang-format on

namespace openblack::graphics
{

struct ShaderDefinition
{
	const std::string_view name;
	const std::string_view vertexShaderName;
	const std::string_view fragmentShaderName;
};

const std::array<bgfx::EmbeddedShader, 17> k_EmbeddedShaders = {{
    BGFX_EMBEDDED_SHADER(vs_line), BGFX_EMBEDDED_SHADER(vs_line_instanced),                                                   //
    BGFX_EMBEDDED_SHADER(fs_line),                                                                                            //
    BGFX_EMBEDDED_SHADER(vs_object), BGFX_EMBEDDED_SHADER(vs_object_instanced), BGFX_EMBEDDED_SHADER(vs_object_hm_instanced), //
    BGFX_EMBEDDED_SHADER(fs_object), BGFX_EMBEDDED_SHADER(fs_sky),                                                            //
    BGFX_EMBEDDED_SHADER(vs_terrain), BGFX_EMBEDDED_SHADER(fs_terrain),                                                       //
    BGFX_EMBEDDED_SHADER(vs_water), BGFX_EMBEDDED_SHADER(fs_water),                                                           //
    BGFX_EMBEDDED_SHADER(vs_sprite), BGFX_EMBEDDED_SHADER(fs_sprite),                                                         //
    BGFX_EMBEDDED_SHADER(vs_footprint_instanced), BGFX_EMBEDDED_SHADER(fs_footprint),                                         //
    BGFX_EMBEDDED_SHADER_END()                                                                                                //
}};

constexpr std::array k_Shaders {
    ShaderDefinition {"DebugLine", "vs_line", "fs_line"},
    ShaderDefinition {"DebugLineInstanced", "vs_line_instanced", "fs_line"},
    ShaderDefinition {"Terrain", "vs_terrain", "fs_terrain"},
    ShaderDefinition {"Object", "vs_object", "fs_object"},
    ShaderDefinition {"ObjectInstanced", "vs_object_instanced", "fs_object"},
    ShaderDefinition {"ObjectHeightMapInstanced", "vs_object_hm_instanced", "fs_object"},
    ShaderDefinition {"Sky", "vs_object", "fs_sky"},
    ShaderDefinition {"Water", "vs_water", "fs_water"},
    ShaderDefinition {"Sprite", "vs_sprite", "fs_sprite"},
    ShaderDefinition {"FootprintInstanced", "vs_footprint_instanced", "fs_footprint"},
};

ShaderManager::~ShaderManager()
{
	// delete all mapped shaders
	ShaderMap::iterator iter;
	for (iter = _shaderPrograms.begin(); iter != _shaderPrograms.end(); ++iter)
	{
		delete iter->second;
	}

	_shaderPrograms.clear();
}

void ShaderManager::LoadShaders()
{
	for (const auto& shader : k_Shaders)
	{
		bgfx::RendererType::Enum type = bgfx::getRendererType();
		auto vs = bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, shader.vertexShaderName.data());
		assert(bgfx::isValid(vs));
		auto fs = bgfx::createEmbeddedShader(k_EmbeddedShaders.data(), type, shader.fragmentShaderName.data());
		assert(bgfx::isValid(fs));
		_shaderPrograms[shader.name.data()] = new ShaderProgram(shader.name.data(), vs, fs);
	}
}

const ShaderProgram* ShaderManager::GetShader(const std::string& name) const
{
	auto i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
	{
		return i->second;
	}

	// todo: return an empty shader?
	return nullptr;
}

void ShaderManager::SetCamera(graphics::RenderPass viewId, const Camera& camera)
{
	auto view = camera.GetViewMatrix(Camera::Interpolation::Current);
	auto proj = camera.GetProjectionMatrix();
	bgfx::setViewTransform(static_cast<bgfx::ViewId>(viewId), &view, &proj);
}

} // namespace openblack::graphics
