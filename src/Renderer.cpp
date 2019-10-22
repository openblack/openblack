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
#include <Profiler.h>

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

Renderer::Renderer(const GameWindow &window, bool vsync)
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
	init.resolution.reset = BGFX_RESET_NONE;
	if (vsync)
	{
		init.resolution.reset |= BGFX_RESET_VSYNC;
	}
	init.callback = dynamic_cast<bgfx::CallbackI*>(_bgfxCallback.get());

	if (!bgfx::init(init)) {
		throw std::runtime_error("Failed to initialize bgfx.");
	}

	LoadShaders();

	// allocate vertex buffers for our debug draw
	_debugCross = DebugLines::CreateCross();
}

Renderer::~Renderer()
{
	_shaderManager.reset();
	_debugCross.reset();
	bgfx::shutdown();
}

void Renderer::LoadShaders()
{
	for (const auto& shader : Shaders)
	{
		_shaderManager->LoadShader(
					shader.name.data(),
					shader.vertexShaderName.data(),
					shader.fragmentShaderName.data());
	}
}

graphics::ShaderManager& Renderer::GetShaderManager() const
{
	return *_shaderManager;
}

void Renderer::UpdateDebugCrossPose(std::chrono::microseconds dt, const glm::vec3 &position, float scale)
{
	_debugCross->SetPose(position, glm::vec3(scale, scale, scale));
}

void Renderer::UploadUniforms(std::chrono::microseconds dt, uint8_t viewId, const Game &game, const Camera &camera)
{
	_shaderManager->SetCamera(viewId, camera);

	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
	terrainShader->SetUniformValue("u_timeOfDay", &game.GetConfig().timeOfDay);
	terrainShader->SetUniformValue("u_bumpmapStrength", &game.GetConfig().bumpMapStrength);
	terrainShader->SetUniformValue("u_smallBumpmapStrength", &game.GetConfig().smallBumpMapStrength);

//	ShaderProgram* objectShader = _shaderManager->GetShader("SkinnedMesh");
//	objectShader->SetUniformValue("u_model", game.GetModelMatrix());
}

void Renderer::ClearScene(uint8_t viewId, int width, int height)
{
	static const uint32_t clearColor = 0x274659ff;

	bgfx::setViewClear(viewId,
		BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH,
		clearColor,
		1.0f,
		0);
	bgfx::setViewRect(viewId, 0, 0, width, height);
	// This dummy draw call is here to make sure that view 0 is cleared if no other draw calls are submitted to view 0.
	bgfx::touch(viewId);
}

void Renderer::DrawScene(const DrawSceneDesc &desc)
{
	desc.profiler.Begin(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawScene : Profiler::Stage::MainPassDrawScene);
	ShaderProgram* objectShader = _shaderManager->GetShader("Object");
	ShaderProgram* waterShader = _shaderManager->GetShader("Water");
	ShaderProgram* terrainShader = _shaderManager->GetShader("Terrain");
	ShaderProgram* debugShader = _shaderManager->GetShader("DebugLine");

	desc.profiler.Begin(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawSky : Profiler::Stage::MainPassDrawSky);
	if (desc.drawSky)
	{
		desc.sky.Draw(desc.viewId, glm::mat4(1.0f), *objectShader);
	}
	desc.profiler.End(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawSky : Profiler::Stage::MainPassDrawSky);

	desc.profiler.Begin(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawWater : Profiler::Stage::MainPassDrawWater);
	if (desc.drawWater)
	{
		desc.water.Draw(desc.viewId, *waterShader);
	}
	desc.profiler.End(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawWater : Profiler::Stage::MainPassDrawWater);

	desc.profiler.Begin(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawIsland : Profiler::Stage::MainPassDrawIsland);
	if (desc.drawIsland)
	{
		desc.island.Draw(desc.viewId, *terrainShader, desc.cullBack);
	}
	desc.profiler.End(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawIsland : Profiler::Stage::MainPassDrawIsland);

	desc.profiler.Begin(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawModels : Profiler::Stage::MainPassDrawModels);
	if (desc.drawEntities)
	{
		desc.entities.DrawModels(desc.viewId, *_shaderManager);
	}
	desc.profiler.End(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawModels : Profiler::Stage::MainPassDrawModels);

	desc.profiler.Begin(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawDebugCross : Profiler::Stage::MainPassDrawDebugCross);
	if (desc.drawDebugCross)
	{
		_debugCross->Draw(desc.viewId, *debugShader);
	}
	desc.profiler.End(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawDebugCross : Profiler::Stage::MainPassDrawDebugCross);

	// Enable stats or debug text.
	auto debugMode = BGFX_DEBUG_NONE;
	if (desc.bgfxDebug)
	{
		debugMode |= BGFX_DEBUG_STATS;
	}
	if (desc.wireframe)
	{
		debugMode |= BGFX_DEBUG_WIREFRAME;
	}
	if (desc.profile)
	{
		debugMode |= BGFX_DEBUG_PROFILER;
	}
	bgfx::setDebug(debugMode);
	desc.profiler.End(desc.viewId == 0 ? Profiler::Stage::ReflectionDrawScene : Profiler::Stage::MainPassDrawScene);
}

void Renderer::Frame()
{
	// Advance to next frame. Process submitted rendering primitives.
	bgfx::frame();
}
