/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "Renderer.h"

#include <cstdint>

#include <SDL_video.h>
#include <bgfx/platform.h>
#include <bimg/bimg.h>
#include <bx/file.h>
#include <glm/gtc/type_ptr.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandBlock.h"
#include "3D/OceanInterface.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "ECS/Components/DebugCross.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/SpriteMesh.h"
#include "ECS/Registry.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "EngineConfig.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/Primitive.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Technique/TechniqueManager.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Profiler.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using namespace openblack::graphics;
using namespace openblack::ecs::systems;

namespace openblack
{
// clang-format off
constexpr auto k_BgfxDefaultStateInvertedZ = 0 \
                                     | BGFX_STATE_WRITE_RGB \
                                     | BGFX_STATE_WRITE_A \
                                     | BGFX_STATE_WRITE_Z \
                                     | BGFX_STATE_CULL_CW \
                                     | BGFX_STATE_DEPTH_TEST_GREATER \
                                     | BGFX_STATE_MSAA;
// clang-format on

struct BgfxCallback: public bgfx::CallbackI
{
	constexpr static std::array<std::string_view, bgfx::Fatal::Count> k_CodeLookup = {
	    "DebugCheck",            //
	    "InvalidShader",         //
	    "UnableToInitialize",    //
	    "UnableToCreateTexture", //
	    "DeviceLost",            //
	};

	~BgfxCallback() override = default;

	void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) override
	{
		const auto* codeStr = k_CodeLookup.at(code).data();
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "bgfx: {}:{}: FATAL ({}): {}", filePath, line, codeStr, str);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
		spdlog::get("graphics")
		    ->log(spdlog::source_loc {filePath, line, SPDLOG_FUNCTION}, spdlog::level::critical, "FATAL ({}): {}", codeStr,
		          str);
#endif

		// Must terminate, continuing will cause crash anyway.
		throw std::runtime_error(std::string("bgfx: ") + filePath + ":" + std::to_string(line) + ": FATAL (" + codeStr +
		                         "): " + str);
	}

	void traceVargs([[maybe_unused]] const char* filePath, [[maybe_unused]] uint16_t line, const char* format,
	                va_list argList) override
	{
		std::array<char, 0x2000> temp;
		char* out = temp.data();
		int32_t len = vsnprintf(out, temp.size(), format, argList);
		if (static_cast<int32_t>(temp.size()) < len)
		{
			out = reinterpret_cast<char*>(alloca(len + 1));
			len = vsnprintf(out, len, format, argList);
		}
		if (len > 0)
		{
			out[len] = '\0';
			if (len > 0 && out[len - 1] == '\n')
			{
				out[len - 1] = '\0';
			}
// TODO(bwrsandman): change level to trace
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
			spdlog::get("graphics")->log(spdlog::source_loc {filePath, line, SPDLOG_FUNCTION}, spdlog::level::debug, out);
#endif
		}
		else
		{
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
			spdlog::get("graphics")
			    ->log(spdlog::source_loc {filePath, line, SPDLOG_FUNCTION}, spdlog::level::err,
			          "bgfx: failed to format message: {}", format);
#endif
		}
	}
	void profilerBegin([[maybe_unused]] const char* name, [[maybe_unused]] uint32_t abgr, [[maybe_unused]] const char* filePath,
	                   [[maybe_unused]] uint16_t line) override
	{
	}
	void profilerBeginLiteral([[maybe_unused]] const char* name, [[maybe_unused]] uint32_t abgr,
	                          [[maybe_unused]] const char* filePath, [[maybe_unused]] uint16_t line) override
	{
	}
	void profilerEnd() override {}
	// Reading and writing to shader cache
	uint32_t cacheReadSize([[maybe_unused]] uint64_t id) override { return 0; }
	bool cacheRead([[maybe_unused]] uint64_t id, [[maybe_unused]] void* data, [[maybe_unused]] uint32_t size) override
	{
		return false;
	}
	void cacheWrite([[maybe_unused]] uint64_t id, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t size) override {}
	// Saving a screenshot
	void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, const void* data,
	                [[maybe_unused]] uint32_t size, bool yflip) override
	{
		SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Taking a screenshot...");

		const auto ext = std::filesystem::path(filePath).extension();
		if (std::filesystem::path(filePath).extension() == ".png")
		{
			bx::FileWriter writer;
			bx::Error err;
			if (bx::open(&writer, filePath, false, &err))
			{
				// Strip out alpha for screenshot
				std::vector<uint32_t> noAlpha;
				noAlpha.resize(size / sizeof(noAlpha[0]), 0);
				memcpy(noAlpha.data(), data, size);
				for (uint32_t y = 0; y < height; ++y)
				{
					for (uint32_t x = 0; x < width; ++x)
					{
						noAlpha[x + pitch / sizeof(noAlpha[0]) * y] |= 0xFF000000;
					}
				}

				bimg::imageWritePng(&writer, width, height, pitch, noAlpha.data(), bimg::TextureFormat::BGRA8, yflip, &err);
				bx::close(&writer);
				SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Screenshot ({}x{}) saved at {}", width, height, filePath);
			}
			else
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to save Screenshot ({}x{}) at {}: {}", width, height,
				                    filePath, std::string(err.getMessage().getCPtr(), err.getMessage().getLength()));
			}
		}
		else
		{
			SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: {} screenshot ({}x{}) requested at {}", ext.string(),
			                   width, height, filePath);
		}
	}
	// Saving a video
	void captureBegin(uint32_t width, uint32_t height, [[maybe_unused]] uint32_t pitch,
	                  [[maybe_unused]] bgfx::TextureFormat::Enum format, [[maybe_unused]] bool yflip) override
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: Video Capture Begin ({}x{}) requested", width, height);
	}
	void captureEnd() override { SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: Video Capture End requested"); }
	void captureFrame([[maybe_unused]] const void* data, [[maybe_unused]] uint32_t size) override
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: Video Capture Frame requested");
	}
};

} // namespace openblack

std::unique_ptr<RendererInterface> RendererInterface::Create(bgfx::RendererType::Enum rendererType, bool vsync) noexcept
{
	bgfx::Init init {};
	init.type = rendererType;

	// Get render area size
	glm::uvec2 drawableSize;
	if (rendererType != bgfx::RendererType::Noop)
	{
		const auto& window = Locator::windowing::value();

		drawableSize = static_cast<glm::uvec2>(window.GetSize());
		init.resolution.width = static_cast<uint32_t>(drawableSize.x);
		init.resolution.height = static_cast<uint32_t>(drawableSize.y);

		// Get Native Handles from SDL window
		const auto handles = window.GetNativeHandles();
		init.platformData.nwh = handles.nativeWindow;
		init.platformData.ndt = handles.nativeDisplay;
	}

	uint32_t bgfxReset = BGFX_RESET_NONE;
	auto bgfxCallback = std::make_unique<BgfxCallback>();
	if (vsync)
	{
		bgfxReset |= BGFX_RESET_VSYNC;
	}
	init.resolution.reset = bgfxReset;
	init.callback = dynamic_cast<bgfx::CallbackI*>(bgfxCallback.get());

	if (!bgfx::init(init))
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to initialize bgfx.");
		return nullptr;
	}

	const bgfx::Caps* caps = bgfx::getCaps();
	if ((caps->supported & BGFX_CAPS_TEXTURE_2D_ARRAY) == 0 || caps->limits.maxTextureLayers < 9)
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Graphics device must support texture layers.");
		return nullptr;
	}

	return std::make_unique<Renderer>(bgfxReset, std::move(bgfxCallback));
}

Renderer::Renderer(uint32_t bgfxReset, std::unique_ptr<BgfxCallback>&& bgfxCallback) noexcept
    : _bgfxCallback(std::move(bgfxCallback))
    , _bgfxReset(bgfxReset)
{
	Locator::shaderManager::value().LoadShaders();
	// allocate vertex buffers for our debug draw and for primitives

	// give debug names to views
	// TODO (#749) use std::views::enumerate
	for (bgfx::ViewId i = 0; const auto& name : k_RenderPassNames)
	{
		bgfx::setViewName(i, name.data());
		++i;
	}
}

Renderer::~Renderer() noexcept
{
	Locator::shaderManager::reset();
	bgfx::frame();
	bgfx::shutdown();
}

void Renderer::ConfigureView(graphics::RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept
{
	bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 0.0f, 0);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, resolution.x, resolution.y);
}

void Renderer::Reset(glm::u16vec2 resolution) const noexcept
{
	bgfx::reset(resolution.x, resolution.y, _bgfxReset);
}

const Texture2D* GetTexture(uint32_t skinID, const std::unordered_map<SkinId, std::unique_ptr<graphics::Texture2D>>& meshSkins)
{
	const auto& textureManager = Locator::resources::value().GetTextures();

	const Texture2D* texture = nullptr;

	if (skinID != 0xFFFFFFFF)
	{
		if (meshSkins.find(skinID) != meshSkins.end())
		{
			texture = meshSkins.at(skinID).get();
		}
		else if (textureManager.Contains(skinID))
		{
			texture = &*textureManager.Handle(skinID);
		}
		else
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Could not find the texture");
		}
	}

	return texture;
}

void Renderer::DrawSubMesh(const graphics::L3DMesh& mesh, const graphics::L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc,
                           bool preserveState) const
{
	assert(&subMesh.GetMesh());
	// We don't draw physics meshes, we haven't implemented statuses (building and graves) and modern GPUs can handle high lod
	if (!desc.drawAll && (subMesh.IsPhysics() || subMesh.GetFlags().status != 0 || (subMesh.GetFlags().lodMask & 1) != 1))
	{
		return;
	}

	const auto& island = Locator::terrainSystem::value();

	auto extent = island.GetExtent();
	auto islandExtent = glm::vec4(extent.minimum, extent.maximum);
	const auto& heightMap = island.GetHeightMap();

	auto const& skins = mesh.GetSkins();
	bool lastPreserveState = false;
	const auto& primitives = subMesh.GetPrimitives();
	for (auto it = primitives.begin(); it != primitives.end(); ++it)
	{
		const auto& prim = *it;

		const bool hasNext = std::next(it) != primitives.end();

		const Texture2D* texture = GetTexture(prim.skinID, skins);
		const Texture2D* nextTexture = !hasNext ? nullptr : GetTexture(std::next(it)->skinID, skins);

		const bool primitivePreserveState = texture != nullptr && texture == nextTexture && (preserveState || hasNext);

		uint32_t skip = Mesh::SkipState::SkipNone;
		if (!lastPreserveState)
		{
			if (desc.modelMatrices != nullptr && desc.matrixCount > 0)
			{
				bgfx::setTransform(desc.modelMatrices, desc.matrixCount);
			}
			if (texture != nullptr)
			{
				desc.program->SetTextureSampler("s_diffuse", 0, *texture);
			}
			if (desc.morphWithTerrain)
			{
				desc.program->SetTextureSampler("s_heightmap", 1, heightMap);   // vs
				desc.program->SetUniformValue("u_islandExtent", &islandExtent); // vs
			}
			if (!desc.isSky)
			{
				const glm::vec4 u_skyAlphaThreshold = {
				    Locator::skySystem::value().GetCurrentSkyType(),
				    prim.thresholdAlpha ? prim.alphaCutoutThreshold : 0.0f,
				    0.0f,
				    0.0f,
				};
				desc.program->SetUniformValue("u_skyAlphaThreshold", &u_skyAlphaThreshold);
			}
		}
		else
		{
			skip |= Mesh::SkipState::SkipRenderState;
			skip |= Mesh::SkipState::SkipVertexBuffer;
		}

		{
			if (desc.instanceBuffer != nullptr && (skip & Mesh::SkipState::SkipInstanceBuffer) == 0)
			{
				bgfx::setInstanceDataBuffer(*desc.instanceBuffer, desc.instanceStart, desc.instanceCount);
			}
			if (subMesh.GetMesh().IsIndexed() && (skip & Mesh::SkipState::SkipIndexBuffer) == 0)
			{
				subMesh.GetMesh().GetIndexBuffer().Bind(prim.indicesCount, prim.indicesOffset);
			}
			if ((skip & Mesh::SkipState::SkipVertexBuffer) == 0)
			{
				subMesh.GetMesh().GetVertexBuffer().Bind();
			}
			if ((skip & Mesh::SkipState::SkipRenderState) == 0)
			{
				bgfx::setState(desc.state, desc.rgba);
			}

			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), desc.program->GetRawHandle(), 0,
			             primitivePreserveState ? BGFX_DISCARD_NONE : BGFX_DISCARD_ALL);
		}
		lastPreserveState = primitivePreserveState;
	}
}

void Renderer::DrawMesh(const graphics::L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept
{
	if (mesh.GetNumSubMeshes() == 0)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Mesh {} has no submeshes to draw", mesh.GetDebugName());
		return;
	}

	const auto& subMeshes = mesh.GetSubMeshes();

	if (subMeshIndex != std::numeric_limits<uint8_t>::max())
	{
		if (subMeshIndex >= mesh.GetNumSubMeshes())
		{
			SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "tried to draw submesh out of range ({}/{})", subMeshIndex,
			                   mesh.GetNumSubMeshes());
		}

		DrawSubMesh(mesh, *subMeshes[subMeshIndex], desc, false);
		return;
	}

	for (auto it = subMeshes.begin(); it != subMeshes.end(); ++it)
	{
		const L3DSubMesh& subMesh = **it;
		DrawSubMesh(mesh, subMesh, desc, std::next(it) != subMeshes.end());
	}
}

void Renderer::DrawScene(const DrawSceneDesc& drawDesc) const noexcept
{
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::FootprintPass);
		const auto& frameBuffer = Locator::terrainSystem::value().GetFootprintFramebuffer();

		DrawSceneDesc footprintPassDesc = drawDesc;
		std::set<Technique> footprintTechnique {Technique::Footprint};
		footprintPassDesc.viewId = graphics::RenderPass::Footprint;
		footprintPassDesc.frameBuffer = &frameBuffer;
		footprintPassDesc.sceneTechnique = footprintTechnique;

		// TODO(bwrsandman): Footprint framebuffer doesn't need to be updated each frame
		DrawPass(footprintPassDesc);
	}
	// Reflection Pass
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::ReflectionPass);
		if (drawDesc.sceneTechnique.contains(Technique::Water))
		{
			DrawSceneDesc drawPassDesc = drawDesc;

			std::set<Technique> reflectionTechnique = drawDesc.sceneTechnique;
			reflectionTechnique.erase(Technique::Water);
			reflectionTechnique.erase(Technique::DebugCross);
			reflectionTechnique.erase(Technique::BoundingBox);
			const auto& frameBuffer = Locator::oceanSystem::value().GetReflectionFramebuffer();
			auto reflectionCamera = drawDesc.camera->Reflect();

			drawPassDesc.viewId = graphics::RenderPass::Reflection;
			drawPassDesc.camera = reflectionCamera.get();
			drawPassDesc.frameBuffer = &frameBuffer;
			drawPassDesc.sceneTechnique = reflectionTechnique;
			drawPassDesc.drawBoundingBoxes = false;
			drawPassDesc.cullBack = true;

			DrawPass(drawPassDesc);
		}
	}

	// Main Draw Pass
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::MainPass);
		DrawPass(drawDesc);
	}
}

void Renderer::DrawPass(const DrawSceneDesc& desc) const
{

	auto& profiler = Locator::profiler::value();

	if (desc.frameBuffer != nullptr)
	{
		desc.frameBuffer->Bind(desc.viewId);
	}
	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));

	Locator::shaderManager::value().SetCamera(desc.viewId, *desc.camera);

	for (auto technique : desc.sceneTechnique)
	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? k_reflectionPassTechniqueStage.at(technique)
		                                    : desc.viewId == RenderPass::Main     ? k_mainPassTechniqueStage.at(technique)
		                                                                          : Profiler::Stage::FootprintDrawFootprint);
		k_TechniqueMap.at(technique)(desc);
	}

	// Enable stats or debug text.
	auto debugMode = BGFX_DEBUG_NONE;
	if (_bgfxDebug)
	{
		debugMode |= BGFX_DEBUG_STATS;
	}
	if (desc.wireframe)
	{
		debugMode |= BGFX_DEBUG_WIREFRAME;
	}
	if (_bgfxProfile)
	{
		debugMode |= BGFX_DEBUG_PROFILER;
	}
	bgfx::setDebug(debugMode);
}

void Renderer::Frame() noexcept
{
	// Advance to next frame. Process submitted rendering primitives.
	bgfx::frame();
}

void Renderer::RequestScreenshot(const std::filesystem::path& filepath) noexcept
{
	const bgfx::FrameBufferHandle mainBackbuffer = BGFX_INVALID_HANDLE;
	bgfx::requestScreenShot(mainBackbuffer, filepath.string().c_str());
}
