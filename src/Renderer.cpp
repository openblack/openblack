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

#include <sstream>

#include <glad/glad.h>
#include <bgfx/platform.h>
#include <SDL_video.h>
#include <spdlog/spdlog.h>

#include <3D/L3DMesh.h>
#include <3D/LandIsland.h>
#include <3D/Sky.h>
#include <3D/Water.h>
#include <Entities/Registry.h>
#include <Game.h>
#include <GameWindow.h>
#include <Graphics/DebugLines.h>
#include <Graphics/ShaderManager.h>

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
using namespace openblack;
using namespace openblack::graphics;

namespace openblack
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

struct BgfxCallback : public bgfx::CallbackI
{
	~BgfxCallback() override = default;

	void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) override
	{
		const static std::array CodeLookup = {
			"DebugCheck",
			"InvalidShader",
			"UnableToInitialize",
			"UnableToCreateTexture",
			"DeviceLost",
		};
		spdlog::critical("bgfx: {}:{}: FATAL ({}): {}",
			filePath, line, CodeLookup[code], str);

		// Must terminate, continuing will cause crash anyway.
		throw std::runtime_error(
			std::string("bgfx: ") + filePath + ":" + std::to_string(line) +
				": FATAL (" + CodeLookup[code] + "): " + str);
	}

	void traceVargs(const char* filePath, uint16_t line, const char* format, va_list argList) override
	{
		char temp[0x2000];
		char* out = temp;
		int32_t len = vsnprintf(out, sizeof(temp), format, argList);
		if ( (int32_t)sizeof(temp) < len)
		{
			out = (char*)alloca(len + 1);
			len = vsnprintf(out, len, format, argList);
		}
		out[len] = '\0';
		if (len > 0 && out[len - 1] == '\n') {
			out[len - 1] = '\0';
		}
		spdlog::debug("bgfx: {}:{}: {}", filePath, line, out);
	}
	void profilerBegin(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override
	{
	}
	void profilerBeginLiteral(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override
	{
	}
	void profilerEnd() override
	{
	}
	// Reading and writing to shader cache
	uint32_t cacheReadSize(uint64_t id) override
	{
		return 0;
	}
	bool cacheRead(uint64_t id, void* data, uint32_t size) override
	{
		return false;
	}
	void cacheWrite(uint64_t id, const void* data, uint32_t size) override
	{
	}
	// Saving a screen shot
	void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, const void* data, uint32_t size, bool yflip) override
	{
	}
	// Saving a video
	void captureBegin(uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum _format, bool yflip) override
	{
	}
	void captureEnd() override
	{
	}
	void captureFrame(const void* data, uint32_t size) override
	{
	}
};

struct ApiThreadArgs
{
  bgfx::PlatformData platformData;
  uint32_t width;
  uint32_t height;
};

}  // namespace openblack

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

uint32_t Renderer::GetRequiredWindowFlags()
{
	return SDL_WINDOW_OPENGL;
}

Renderer::Renderer(const GameWindow& window, const std::string binaryPath)
	: _shaderManager(std::make_unique<ShaderManager>())
	, _bgfxCallback(std::make_unique<BgfxCallback>())
{
	// Call bgfx::renderFrame before bgfx::init to signal to bgfx not to create a render thread.
	// Most graphics APIs must be used on the same thread that created the window.
	bgfx::renderFrame();

	// Create a thread to call the bgfx API from (except bgfx::renderFrame).
	ApiThreadArgs apiThreadArgs {};

	// Get render area size
	int drawable_width;
	int drawable_height;
	window.GetDrawableSize(drawable_width, drawable_height);
	apiThreadArgs.width = drawable_width;
	apiThreadArgs.height = drawable_height;

	// Get Native Handles from SDL window
	window.GetNativeHandles(apiThreadArgs.platformData.nwh, apiThreadArgs.platformData.ndt);

	// TODO(bwrsandman): This is single threaded init. Replace with multithreaded
	bgfx::Init init {};
#if USE_VULKAN
	init.type = bgfx::RendererType::Vulkan;
#else
	init.type = bgfx::RendererType::OpenGL;
#endif  // USE_VULKAN
	init.platformData = apiThreadArgs.platformData;
	init.resolution.width = (uint32_t)drawable_width;
	init.resolution.height = (uint32_t)drawable_height;
	init.resolution.reset = BGFX_RESET_VSYNC;
	init.callback = dynamic_cast<bgfx::CallbackI*>(_bgfxCallback.get());

	if (!bgfx::init(init)) {
		throw std::runtime_error("Failed to initialize bgfx.");
	}

	for (auto& attr : GetRequiredContextAttributes())
	{
		if (attr.api == Renderer::Api::OpenGl)
		{
			SDL_GL_SetAttribute(static_cast<SDL_GLattr>(attr.name), attr.value);
		}
	}

	// TODO(bwrsandman): uncomment this once rendering is taken over
	auto internal = bgfx::getInternalData();
	SDL_GL_MakeCurrent(window.GetHandle(), internal->context);
	_glcontext = std::unique_ptr<SDL_GLContext, SDLDestroyer>((SDL_GLContext*)&internal->context);

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

Renderer::~Renderer()
{
	// TODO(bwrsandman): uncomment this once rendering is taken over
	// bgfx::shutdown();
}

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

//	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
//	terrainShader->SetUniformValue("u_timeOfDay", game.GetConfig().timeOfDay);
//	terrainShader->SetUniformValue("u_bumpmapStrength", game.GetConfig().bumpMapStrength);
//	terrainShader->SetUniformValue("u_smallBumpmapStrength", game.GetConfig().smallBumpMapStrength);
//
//	ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
//	objectShader->SetUniformValue("u_model", game.GetModelMatrix());
}

void Renderer::ClearScene(int width, int height)
{
	static const uint32_t clearColor = 0x274659ff;

	bgfx::setViewClear(0,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		clearColor,
		1.0f,
		0);
	bgfx::setViewRect(0, 0, 0, width, height);
	// This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
	bgfx::touch(0);
}

void Renderer::DrawScene(const Game &game, bool drawWater, bool drawDebugCross, bool cullBack)
{
//	ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
	ShaderProgram* waterShader = _shaderManager->GetShader("Water");
//	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
	ShaderProgram* debugShader = _shaderManager->GetShader("DebugLine");
//
//	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_BLEND);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//
//	game.GetSky().Draw(*objectShader);
//
//	glEnable(GL_CULL_FACE);
//	glCullFace(cullBack ? GL_BACK : GL_FRONT);
//	glFrontFace(GL_CCW);

	if (drawWater)
	{
		game.GetWater().Draw(*waterShader);
	}

//	if (game.GetConfig().wireframe)
//		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
//
//	game.GetLandIsland().Draw(*terrainShader);
//
//	if (game.GetConfig().wireframe)
//		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
//
//	game.GetTestModel().Draw(*objectShader, 0);
//
//	glDisable(GL_CULL_FACE);
//	game.GetEntityRegistry().DrawModels(*_shaderManager);

	if (drawDebugCross)
	{

		_debugCross->Draw(*debugShader);
	}

	// Enable stats or debug text.
	if (game.GetConfig().bgfxDebug)
	{
		bgfx::setDebug(BGFX_DEBUG_STATS);
	} else
	{
		bgfx::setDebug(BGFX_DEBUG_NONE);
	}
}

void Renderer::Frame()
{
	// Advance to next frame. Process submitted rendering primitives.
	bgfx::frame();
}
