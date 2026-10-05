/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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

#include "ShaderManager.h"

#include <cstdint> // Shaders below need uint8_t

#include <array>

#include <bgfx/embedded_shader.h>
// BGFX has support for WSL to use windows d3d. We disable it here from the BGFX_EMBEDDED_SHADER macro.
#if BX_PLATFORM_LINUX
#undef BGFX_EMBEDDED_SHADER_DXBC
#define BGFX_EMBEDDED_SHADER_DXBC(...)
#undef BGFX_EMBEDDED_SHADER_DX9BC
#define BGFX_EMBEDDED_SHADER_DX9BC(...)
#endif

#include "Camera/Camera.h"
#include "GraphicsHandleBgfx.h"

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
#define SHADER_NAME vs_object_environment
#include "ShaderIncluder.h"
#define SHADER_NAME fs_object_environment
#include "ShaderIncluder.h"
#define SHADER_NAME vs_object_static_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME vs_object_lightmap_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME fs_object_lightmap
#include "ShaderIncluder.h"
#define SHADER_NAME fs_object_reflective_lightmap
#include "ShaderIncluder.h"
#define SHADER_NAME fs_reflection
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
#define SHADER_NAME fs_land_alpha
#include "ShaderIncluder.h"
#define SHADER_NAME vs_celestial
#include "ShaderIncluder.h"
#define SHADER_NAME fs_celestial
#include "ShaderIncluder.h"
#define SHADER_NAME vs_mist
#include "ShaderIncluder.h"
#define SHADER_NAME fs_mist
#include "ShaderIncluder.h"
#define SHADER_NAME vs_land_luminosity
#include "ShaderIncluder.h"
#define SHADER_NAME fs_land_luminosity
#include "ShaderIncluder.h"
#define SHADER_NAME fs_land_shade
#include "ShaderIncluder.h"
#define SHADER_NAME fs_land_colour
#include "ShaderIncluder.h"
#define SHADER_NAME fs_sky_dome
#include "ShaderIncluder.h"

#define SHADER_NAME vs_vegetation
#include "ShaderIncluder.h"
#define SHADER_NAME vs_vegetation_hm_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME fs_vegetation
#include "ShaderIncluder.h"
#define SHADER_NAME fs_shadow_caster
#include "ShaderIncluder.h"
#define SHADER_NAME vs_object_shadow_instanced
#include "ShaderIncluder.h"
#define SHADER_NAME fs_object_shadow
#include "ShaderIncluder.h"

#define SHADER_NAME vs_beam
#include "ShaderIncluder.h"
#define SHADER_NAME fs_beam
#include "ShaderIncluder.h"

#define SHADER_NAME vs_interface
#include "ShaderIncluder.h"
#define SHADER_NAME vs_text3d
#include "ShaderIncluder.h"
#define SHADER_NAME fs_interface
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

const std::array<bgfx::EmbeddedShader, 45> k_EmbeddedShaders = {{
    BGFX_EMBEDDED_SHADER(vs_line),
    BGFX_EMBEDDED_SHADER(vs_line_instanced), //
    BGFX_EMBEDDED_SHADER(fs_line),           //
    BGFX_EMBEDDED_SHADER(vs_object),
    BGFX_EMBEDDED_SHADER(vs_object_instanced),
    BGFX_EMBEDDED_SHADER(vs_object_hm_instanced), //
    BGFX_EMBEDDED_SHADER(fs_object),
    BGFX_EMBEDDED_SHADER(vs_object_environment),
    BGFX_EMBEDDED_SHADER(fs_object_environment),
    BGFX_EMBEDDED_SHADER(vs_object_static_instanced),
    BGFX_EMBEDDED_SHADER(vs_object_lightmap_instanced),
    BGFX_EMBEDDED_SHADER(fs_reflection),
    BGFX_EMBEDDED_SHADER(fs_object_lightmap),
    BGFX_EMBEDDED_SHADER(fs_object_reflective_lightmap),
    BGFX_EMBEDDED_SHADER(fs_sky), //
    BGFX_EMBEDDED_SHADER(vs_terrain),
    BGFX_EMBEDDED_SHADER(fs_terrain), //
    BGFX_EMBEDDED_SHADER(vs_water),
    BGFX_EMBEDDED_SHADER(fs_water), //
    BGFX_EMBEDDED_SHADER(vs_sprite),
    BGFX_EMBEDDED_SHADER(fs_sprite), //
    BGFX_EMBEDDED_SHADER(vs_footprint_instanced),
    BGFX_EMBEDDED_SHADER(fs_footprint),       //
    BGFX_EMBEDDED_SHADER(fs_land_alpha),      //
    BGFX_EMBEDDED_SHADER(vs_celestial),       //
    BGFX_EMBEDDED_SHADER(fs_celestial),       //
    BGFX_EMBEDDED_SHADER(vs_mist),            //
    BGFX_EMBEDDED_SHADER(fs_mist),            //
    BGFX_EMBEDDED_SHADER(vs_land_luminosity), //
    BGFX_EMBEDDED_SHADER(fs_land_luminosity), //
    BGFX_EMBEDDED_SHADER(fs_land_shade),      //
    BGFX_EMBEDDED_SHADER(fs_land_colour),     //
    BGFX_EMBEDDED_SHADER(fs_sky_dome),        //
    BGFX_EMBEDDED_SHADER(vs_vegetation),
    BGFX_EMBEDDED_SHADER(vs_vegetation_hm_instanced),
    BGFX_EMBEDDED_SHADER(fs_vegetation),    //
    BGFX_EMBEDDED_SHADER(fs_shadow_caster), //
    BGFX_EMBEDDED_SHADER(vs_object_shadow_instanced),
    BGFX_EMBEDDED_SHADER(fs_object_shadow), //
    BGFX_EMBEDDED_SHADER(vs_beam),
    BGFX_EMBEDDED_SHADER(fs_beam), //
    BGFX_EMBEDDED_SHADER(vs_interface),
    BGFX_EMBEDDED_SHADER(vs_text3d),
    BGFX_EMBEDDED_SHADER(fs_interface), //
    BGFX_EMBEDDED_SHADER_END()          //
}};

constexpr std::array k_Shaders {
    ShaderDefinition {"DebugLine", "vs_line", "fs_line"},
    ShaderDefinition {"DebugLineInstanced", "vs_line_instanced", "fs_line"},
    ShaderDefinition {"Terrain", "vs_terrain", "fs_terrain"},
    ShaderDefinition {"Object", "vs_object", "fs_object"},
    ShaderDefinition {"ObjectEnvironment", "vs_object_environment", "fs_object_environment"},
    ShaderDefinition {"ObjectInstanced", "vs_object_instanced", "fs_object"},
    ShaderDefinition {"ObjectHeightMapInstanced", "vs_object_hm_instanced", "fs_object"},
    ShaderDefinition {"ObjectStaticInstanced", "vs_object_static_instanced", "fs_object"},
    ShaderDefinition {"ObjectLightmapInstanced", "vs_object_lightmap_instanced", "fs_object_lightmap"},
    ShaderDefinition {"Reflection", "vs_object", "fs_reflection"},
    ShaderDefinition {"ObjectReflectiveLightmapInstanced", "vs_object_lightmap_instanced", "fs_object_reflective_lightmap"},
    ShaderDefinition {"Sky", "vs_object", "fs_sky"},
    ShaderDefinition {"Water", "vs_water", "fs_water"},
    ShaderDefinition {"Sprite", "vs_sprite", "fs_sprite"},
    ShaderDefinition {"FootprintInstanced", "vs_footprint_instanced", "fs_footprint"},
    ShaderDefinition {"LandAlphaInstanced", "vs_footprint_instanced", "fs_land_alpha"},
    ShaderDefinition {"Celestial", "vs_celestial", "fs_celestial"},
    ShaderDefinition {"Mist", "vs_mist", "fs_mist"},
    ShaderDefinition {"LandLuminosity", "vs_land_luminosity", "fs_land_luminosity"},
    ShaderDefinition {"LandShade", "vs_land_luminosity", "fs_land_shade"},
    ShaderDefinition {"LandColour", "vs_land_luminosity", "fs_land_colour"},
    ShaderDefinition {"SkyDome", "vs_land_luminosity", "fs_sky_dome"},
    ShaderDefinition {"Vegetation", "vs_vegetation", "fs_vegetation"},
    ShaderDefinition {"VegetationHeightMapInstanced", "vs_vegetation_hm_instanced", "fs_vegetation"},
    ShaderDefinition {"ShadowCaster", "vs_object", "fs_shadow_caster"},
    ShaderDefinition {"ObjectShadowInstanced", "vs_object_shadow_instanced", "fs_object_shadow"},
    ShaderDefinition {"Beam", "vs_beam", "fs_beam"},
    ShaderDefinition {"Interface", "vs_interface", "fs_interface"},
    ShaderDefinition {"Text3D", "vs_text3d", "fs_interface"},
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
		_shaderPrograms[shader.name.data()] = new ShaderProgram(shader.name.data(), fromBgfx(vs), fromBgfx(fs));
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
