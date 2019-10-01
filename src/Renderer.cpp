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

#include "Renderer.h"

#include <3D/L3DMesh.h>
#include <3D/LandIsland.h>
#include <3D/Sky.h>
#include <3D/Water.h>
#include <Entities/Registry.h>
#include <Game.h>
#include <GameWindow.h>
#include <Graphics/DebugLines.h>
#include <Graphics/ShaderManager.h>
#include <SDL_video.h>
#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <sstream>

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
using namespace openblack;
using namespace openblack::graphics;

namespace
{
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
	if (auto renderer = reinterpret_cast<const Renderer*>(userParam))
	{
		renderer->MessageCallback(source, type, id, severity, length, message);
	}
	else
	{
		spdlog::warn("MessageCallback user param not properly installed");
	}
}
} // namespace

std::vector<Renderer::RequiredAttribute> Renderer::GetRequiredWindowingAttributes()
{
	return std::vector<RequiredAttribute> {
		{ Api::OpenGl, SDL_GL_RED_SIZE, 8 },
		{ Api::OpenGl, SDL_GL_GREEN_SIZE, 8 },
		{ Api::OpenGl, SDL_GL_BLUE_SIZE, 8 },
		{ Api::OpenGl, SDL_GL_ALPHA_SIZE, 8 },
		{ Api::OpenGl, SDL_GL_DOUBLEBUFFER, 1 },
	};
}

std::vector<Renderer::RequiredAttribute> Renderer::GetRequiredContextAttributes()
{
	// Create a debug context?
	bool useDebug = true;
	return std::vector<RequiredAttribute> {
		{ Api::OpenGl, SDL_GL_CONTEXT_MAJOR_VERSION, 3 },
		{ Api::OpenGl, SDL_GL_CONTEXT_MINOR_VERSION, 3 },
		{ Api::OpenGl, SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE },

		{ Api::OpenGl, SDL_GL_CONTEXT_FLAGS, useDebug ? SDL_GL_CONTEXT_DEBUG_FLAG : 0 },
	};
}

uint32_t Renderer::GetRequiredFlags()
{
	return SDL_WINDOW_OPENGL;
}

Renderer::Renderer(const GameWindow& window, const std::string binaryPath):
    _shaderManager(std::make_unique<ShaderManager>())
{
	for (auto& attr : GetRequiredContextAttributes())
	{
		if (attr.api == Renderer::Api::OpenGl)
		{
			SDL_GL_SetAttribute(static_cast<SDL_GLattr>(attr.name), attr.value);
		}
	}

	auto context = SDL_GL_CreateContext(window.GetHandle());
	if (context == nullptr)
	{
		// try again, maybe on wayland
		spdlog::debug("Could not create OpenGL context, try again without MULTISAMPLEBUFFERS");
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
		context = SDL_GL_CreateContext(window.GetHandle());
		if (context == nullptr)
		{
			throw std::runtime_error("Could not create OpenGL context: " + std::string(SDL_GetError()));
		}
	}
	_glcontext = std::unique_ptr<SDL_GLContext, SDLDestroyer>(&context);

	spdlog::info("OpenGL context successfully created.");

	int majorVersion, minorVersion;
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, &majorVersion);
	SDL_GL_GetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, &minorVersion);

	spdlog::info("OpenGL version: {}.{}", majorVersion, minorVersion);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		throw std::runtime_error("Failed to initialize OpenGL loader!\n");
	}

	if (GLAD_GL_ARB_debug_output)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(::MessageCallback, this);
		glDebugMessageControl(GL_DEBUG_SOURCE_API, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
	}
	else
	{
		spdlog::warn("GL_ARB_debug_output not supported");
	}

	if (!GLAD_GL_ARB_texture_storage)
		spdlog::error("GL_ARB_texture_storage unsupported");
	if (!GLAD_GL_EXT_texture_compression_s3tc)
		spdlog::error("GL_EXT_texture_compression_s3tc unsupported");

	LoadShaders(binaryPath);

	// allocate vertex buffers for our debug draw
	_debugCross = DebugLines::CreateCross();
}

Renderer::~Renderer() = default;

void Renderer::LoadShaders(const std::string &binaryPath)
{
	fs::path binPath{binaryPath};
	for (auto& shader : Shaders)
	{
		_shaderManager->LoadShader(
					shader.name,
					(binPath / shader.vertexShaderFile).generic_string(),
					(binPath / shader.fragmentShaderFile).generic_string());
	}
}

SDL_GLContext& Renderer::GetGLContext() const
{
	return *_glcontext;
}

graphics::ShaderManager& Renderer::GetShaderManager() const
{
	return *_shaderManager;
}

void Renderer::MessageCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const std::string& message) const
{
	spdlog::debug("GL CALLBACK: {} type = {}, severity = {}, message = {}\n",
	              (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
	              type, severity, message);
}

void Renderer::UpdateDebugCrossPose(std::chrono::microseconds dt, const glm::vec3 &position, float scale)
{
	_debugCross->SetPose(position, scale);
}

void Renderer::UploadUniforms(std::chrono::microseconds dt, const Game &game, const Camera &camera)
{
	_shaderManager->SetCamera(camera);

	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
	terrainShader->SetUniformValue("u_timeOfDay", game.GetConfig().timeOfDay);
	terrainShader->SetUniformValue("u_bumpmapStrength", game.GetConfig().bumpMapStrength);
	terrainShader->SetUniformValue("u_smallBumpmapStrength", game.GetConfig().smallBumpMapStrength);

	ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
	objectShader->SetUniformValue("u_model", game.GetModelMatrix());
}

void Renderer::ClearScene(int width, int height)
{
	glViewport(0, 0, width, height);
	glClearColor(39.0f / 255.0f, 70.0f / 255.0f, 89.0f / 255.0f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::DrawScene(const Game &game, bool drawWater, bool drawDebugCross, bool cullBack)
{
	ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
	ShaderProgram* waterShader = _shaderManager->GetShader("Water");
	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
	ShaderProgram* debugShader = _shaderManager->GetShader("DebugLine");

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	game.GetSky().Draw(*objectShader);

	glEnable(GL_CULL_FACE);
	glCullFace(cullBack ? GL_BACK : GL_FRONT);
	glFrontFace(GL_CCW);

	if (drawWater)
	{
		game.GetWater().Draw(*waterShader);
	}

	if (game.GetConfig().wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	game.GetLandIsland().Draw(*terrainShader);

	if (game.GetConfig().wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	game.GetTestModel().Draw(*objectShader, 0);

	glDisable(GL_CULL_FACE);
	game.GetEntityRegistry().DrawModels(*_shaderManager);

	if (drawDebugCross)
	{
		glDisable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);

		_debugCross->Draw(*debugShader);
	}
}
