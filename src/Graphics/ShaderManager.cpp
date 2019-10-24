/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#include <Graphics/ShaderManager.h>

#include <bgfx/embedded_shader.h>

#include <3D/Camera.h>

#include "Shaders/vs_line.bin.h"
#include "Shaders/fs_line.bin.h"
#include "Shaders/vs_object.bin.h"
#include "Shaders/fs_object.bin.h"
#include "Shaders/vs_terrain.bin.h"
#include "Shaders/fs_terrain.bin.h"
#include "Shaders/vs_water.bin.h"
#include "Shaders/fs_water.bin.h"

namespace openblack::graphics
{

const bgfx::EmbeddedShader s_embeddedShaders[] =
	{
		BGFX_EMBEDDED_SHADER(vs_line),
		BGFX_EMBEDDED_SHADER(fs_line),

		BGFX_EMBEDDED_SHADER(vs_object),
		BGFX_EMBEDDED_SHADER(fs_object),

		BGFX_EMBEDDED_SHADER(vs_terrain),
		BGFX_EMBEDDED_SHADER(fs_terrain),

		BGFX_EMBEDDED_SHADER(vs_water),
		BGFX_EMBEDDED_SHADER(fs_water),


		BGFX_EMBEDDED_SHADER_END()
	};

ShaderManager::~ShaderManager()
{
	// delete all mapped shaders
	ShaderMap::iterator iter;
	for (iter = _shaderPrograms.begin(); iter != _shaderPrograms.end(); ++iter)
		delete iter->second;

	_shaderPrograms.clear();
}

ShaderProgram* ShaderManager::LoadShader(const std::string& name, const std::string& vertexShaderName, const std::string& fragmentShaderName)
{
	bgfx::RendererType::Enum type = bgfx::getRendererType();

	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	ShaderProgram* program = new ShaderProgram(name,
		bgfx::createEmbeddedShader(s_embeddedShaders, type, vertexShaderName.c_str()),
		bgfx::createEmbeddedShader(s_embeddedShaders, type, fragmentShaderName.c_str())
	);
	_shaderPrograms[name]  = program;
	return program;
}

ShaderProgram* ShaderManager::GetShader(const std::string& name)
{
	ShaderMap::iterator i = _shaderPrograms.find(name);
	if (i != _shaderPrograms.end())
		return i->second;

	// todo: return an empty shader?
	return nullptr;
}

void ShaderManager::SetCamera(uint8_t viewId, const Camera &camera)
{
	auto view = camera.GetViewMatrix();
	auto proj = camera.GetProjectionMatrix();
	bgfx::setViewTransform(viewId, &view, &proj);
}

} // namespace openblack::graphics
