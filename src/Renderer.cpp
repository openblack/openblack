/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Renderer.h"

#include "3D/Camera.h"
#include "3D/L3DMesh.h"
#include "3D/LandIsland.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Entities/Registry.h"
#include "GameWindow.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/ShaderManager.h"
#include "Profiler.h"

#include <SDL_video.h>
#include <bgfx/platform.h>
#include <spdlog/spdlog.h>

using namespace openblack;
using namespace openblack::graphics;

namespace openblack
{
struct BgfxCallback: public bgfx::CallbackI
{
	~BgfxCallback() override = default;

	void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) override
	{
		const static std::array CodeLookup = {
		    "DebugCheck", "InvalidShader", "UnableToInitialize", "UnableToCreateTexture", "DeviceLost",
		};
		spdlog::critical("bgfx: {}:{}: FATAL ({}): {}", filePath, line, CodeLookup[code], str);

		// Must terminate, continuing will cause crash anyway.
		throw std::runtime_error(std::string("bgfx: ") + filePath + ":" + std::to_string(line) + ": FATAL (" +
		                         CodeLookup[code] + "): " + str);
	}

	void traceVargs(const char* filePath, uint16_t line, const char* format, va_list argList) override
	{
		char temp[0x2000];
		char* out = temp;
		int32_t len = vsnprintf(out, sizeof(temp), format, argList);
		if ((int32_t)sizeof(temp) < len)
		{
			out = (char*)alloca(len + 1);
			len = vsnprintf(out, len, format, argList);
		}
		out[len] = '\0';
		if (len > 0 && out[len - 1] == '\n')
		{
			out[len - 1] = '\0';
		}
		spdlog::debug("bgfx: {}:{}: {}", filePath, line, out);
	}
	void profilerBegin(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override {}
	void profilerBeginLiteral(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override {}
	void profilerEnd() override {}
	// Reading and writing to shader cache
	uint32_t cacheReadSize(uint64_t id) override { return 0; }
	bool cacheRead(uint64_t id, void* data, uint32_t size) override { return false; }
	void cacheWrite(uint64_t id, const void* data, uint32_t size) override {}
	// Saving a screen shot
	void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, const void* data, uint32_t size,
	                bool yflip) override
	{
	}
	// Saving a video
	void captureBegin(uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum _format, bool yflip) override
	{
	}
	void captureEnd() override {}
	void captureFrame(const void* data, uint32_t size) override {}
};

} // namespace openblack

Renderer::Renderer(const GameWindow* window, bgfx::RendererType::Enum rendererType, bool vsync)
    : _shaderManager(std::make_unique<ShaderManager>())
    , _bgfxCallback(std::make_unique<BgfxCallback>())
{
	bgfx::Init init {};
	init.type = rendererType;

	// Get render area size
	int drawable_width;
	int drawable_height;
	if (rendererType != bgfx::RendererType::Noop)
	{
		window->GetSize(drawable_width, drawable_height);
		init.resolution.width = static_cast<uint32_t>(drawable_width);
		init.resolution.height = static_cast<uint32_t>(drawable_height);

		// Get Native Handles from SDL window
		window->GetNativeHandles(init.platformData.nwh, init.platformData.ndt);
	}

	init.resolution.reset = BGFX_RESET_NONE;
	if (vsync)
	{
		init.resolution.reset |= BGFX_RESET_VSYNC;
	}
	init.callback = dynamic_cast<bgfx::CallbackI*>(_bgfxCallback.get());

	if (!bgfx::init(init))
	{
		throw std::runtime_error("Failed to initialize bgfx.");
	}

	LoadShaders();

	// allocate vertex buffers for our debug draw
	_debugCross = DebugLines::CreateCross();

	// give debug names to views
	for (bgfx::ViewId i = 0; i < static_cast<bgfx::ViewId>(graphics::RenderPass::_count); ++i)
	{
		bgfx::setViewName(i, RenderPassNames[i].data());
	}
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
		_shaderManager->LoadShader(shader.name.data(), shader.vertexShaderName.data(), shader.fragmentShaderName.data());
	}
}

void Renderer::ConfigureView(graphics::RenderPass viewId, uint16_t width, uint16_t height) const
{
	static const uint32_t clearColor = 0x274659ff;

	bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, width, height);
}

graphics::ShaderManager& Renderer::GetShaderManager() const
{
	return *_shaderManager;
}

void Renderer::UpdateDebugCrossUniforms(const glm::vec3& position, float scale)
{
	_debugCross->SetPose(position, glm::vec3(scale, scale, scale));
}

void Renderer::DrawScene(const DrawSceneDesc& drawDesc) const
{
	// Reflection Pass
	{
		DrawSceneDesc drawPassDesc = drawDesc;
		auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::ReflectionPass);

		auto& frameBuffer = drawDesc.water.GetFrameBuffer();
		auto reflectionCamera = drawDesc.camera->Reflect(drawDesc.water.GetReflectionPlane());

		drawPassDesc.viewId = graphics::RenderPass::Reflection;
		drawPassDesc.camera = reflectionCamera.get();
		drawPassDesc.frameBuffer = &frameBuffer;
		drawPassDesc.drawWater = false;
		drawPassDesc.drawDebugCross = false;
		drawPassDesc.drawBoundingBoxes = false;
		drawPassDesc.cullBack = true;
		DrawPass(drawPassDesc);
	}

	// Main Draw Pass
	{
		auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::MainPass);
		DrawPass(drawDesc);
	}
}

void Renderer::DrawPass(const DrawSceneDesc& desc) const
{
	if (desc.frameBuffer)
	{
		desc.frameBuffer->Bind(desc.viewId);
	}
	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));

	_shaderManager->SetCamera(desc.viewId, *desc.camera);

	auto objectShader = _shaderManager->GetShader("Object");
	auto waterShader = _shaderManager->GetShader("Water");
	auto terrainShader = _shaderManager->GetShader("Terrain");
	auto debugShader = _shaderManager->GetShader("DebugLine");

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSky
		                                                                               : Profiler::Stage::MainPassDrawSky);
		if (desc.drawSky)
		{
			desc.sky.Draw(desc.viewId, glm::mat4(1.0f), *objectShader, desc.cullBack);
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawWater
		                                                                               : Profiler::Stage::MainPassDrawWater);
		if (desc.drawWater)
		{
			desc.water.Draw(desc.viewId, *waterShader);
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawIsland
		                                                                               : Profiler::Stage::MainPassDrawIsland);
		if (desc.drawIsland)
		{
			desc.island.Draw(desc.viewId, *terrainShader, desc.cullBack);
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawModels
		                                                                               : Profiler::Stage::MainPassDrawModels);
		if (desc.drawEntities)
		{
			desc.entities.DrawModels(desc.viewId, *_shaderManager);
		}

		if (desc.drawTestModel)
		{
			auto shader = _shaderManager->GetShader("Object");
			// clang-format off
			uint64_t state = 0u
			    | BGFX_STATE_WRITE_MASK
			    | BGFX_STATE_DEPTH_TEST_LESS
			    | BGFX_STATE_CULL_CCW
			    | BGFX_STATE_MSAA
			;
			// clang-format on
			desc.testModel.Submit(desc.viewId, desc.testModel.GetBoneMatrices().data(), desc.testModel.GetBoneMatrices().size(),
			                      *shader, 0, state);
		}
	}

	{
		auto section =
		    desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawDebugCross
		                                                                    : Profiler::Stage::MainPassDrawDebugCross);
		if (desc.drawDebugCross)
		{
			_debugCross->Draw(desc.viewId, *debugShader);
		}
	}

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
}

void Renderer::Frame()
{
	// Advance to next frame. Process submitted rendering primitives.
	bgfx::frame();
}
