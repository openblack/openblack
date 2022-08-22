/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Renderer.h"

#include <SDL_video.h>
#include <bgfx/platform.h>
#include <bimg/bimg.h>
#include <bx/file.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/Camera.h"
#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/LandIsland.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Registry.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "Game.h"
#include "GameWindow.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/Primitive.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/VertexBuffer.h"
#include "Locator.h"
#include "Profiler.h"
#include "Resources/ResourceManager.h"
#include "Resources/Resources.h"

using namespace openblack;
using namespace openblack::graphics;
using namespace openblack::ecs::systems;

namespace openblack
{
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
	// Saving a screen shot
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
				bimg::imageWritePng(&writer, width, height, pitch, data, bimg::TextureFormat::BGRA8, yflip, &err);
				bx::close(&writer);
				SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Screenshot ({}x{}) saved at {}", width, height, filePath);
			}
			else
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to save Screenshot ({}x{}) at {}: {}", width, height,
				                    filePath, std::string(err.getMessage().getPtr(), err.getMessage().getLength()));
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

Renderer::Renderer(const GameWindow* window, bgfx::RendererType::Enum rendererType, bool vsync)
    : _shaderManager(std::make_unique<ShaderManager>())
    , _bgfxCallback(std::make_unique<BgfxCallback>())
{
	bgfx::Init init {};
	init.type = rendererType;

	// Get render area size
	int drawableWidth;
	int drawableHeight;
	if (rendererType != bgfx::RendererType::Noop)
	{
		window->GetSize(drawableWidth, drawableHeight);
		init.resolution.width = static_cast<uint32_t>(drawableWidth);
		init.resolution.height = static_cast<uint32_t>(drawableHeight);

		// Get Native Handles from SDL window
		window->GetNativeHandles(init.platformData.nwh, init.platformData.ndt);
	}

	_bgfxReset = BGFX_RESET_NONE;
	if (vsync)
	{
		_bgfxReset |= BGFX_RESET_VSYNC;
	}
	init.resolution.reset = _bgfxReset;
	init.callback = dynamic_cast<bgfx::CallbackI*>(_bgfxCallback.get());

	if (!bgfx::init(init))
	{
		throw std::runtime_error("Failed to initialize bgfx.");
	}

	const bgfx::Caps* caps = bgfx::getCaps();
	if ((caps->supported & BGFX_CAPS_TEXTURE_2D_ARRAY) == 0 || caps->limits.maxTextureLayers < 9)
	{
		throw std::runtime_error("Graphics device must support texture layers.");
	}

	_shaderManager->LoadShaders();

	// allocate vertex buffers for our debug draw and for primitives
	_debugCross = DebugLines::CreateCross();
	_plane = Primitive::CreatePlane();

	// give debug names to views
	for (bgfx::ViewId i = 0; const auto& name : k_RenderPassNames)
	{
		bgfx::setViewName(i, name.data());
		++i;
	}
}

Renderer::~Renderer()
{
	_plane.reset();
	_shaderManager.reset();
	_debugCross.reset();
	bgfx::frame();
	bgfx::shutdown();
}

void Renderer::ConfigureView(graphics::RenderPass viewId, uint16_t width, uint16_t height, uint32_t clearColor) const
{
	bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, width, height);
}

void Renderer::Reset(uint16_t width, uint16_t height) const
{
	bgfx::reset(width, height, _bgfxReset);
}

graphics::ShaderManager& Renderer::GetShaderManager() const
{
	return *_shaderManager;
}

void Renderer::UpdateDebugCrossUniforms(const glm::mat4& pose)
{
	_debugCrossPose = pose;
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

void Renderer::DrawSubMesh(const L3DMesh& mesh, const L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc,
                           bool preserveState) const
{
	assert(&subMesh.GetMesh());
	// We don't draw physics meshes, we haven't implemented statuses (building and graves) and modern GPUs can handle high lod
	if (!desc.drawAll && (subMesh.IsPhysics() || subMesh.GetFlags().status != 0 || (subMesh.GetFlags().lodMask & 1) != 1))
	{
		return;
	}

	glm::vec2 extentMin;
	glm::vec2 extentMax;

	const auto& island = Game::Instance()->GetLandIsland();
	island.GetExtent(extentMin, extentMax);
	auto islandExtent = glm::vec4(extentMin, extentMax);
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

		bool primitivePreserveState = texture != nullptr && texture == nextTexture && (preserveState || hasNext);

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
				    desc.skyType,
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

void Renderer::DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const
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

void Renderer::DrawFootprintPass(const DrawSceneDesc& drawDesc) const
{
	const auto viewId = graphics::RenderPass::Footprint;
	auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::FootprintPass);
	if (drawDesc.drawIsland)
	{
		const auto& island = Game::Instance()->GetLandIsland();
		island.GetFootprintFramebuffer().Bind(viewId);

		// This dummy draw call is here to make sure that view is cleared if no
		// other draw calls are submitted to view
		bgfx::touch(static_cast<bgfx::ViewId>(viewId));

		// _shaderManager->SetCamera(viewId, *drawDesc.camera); // TODO
		glm::mat4 view;
		glm::mat4 proj;
		island.GetOrthoViewProj(view, proj);
		bgfx::setViewTransform(static_cast<bgfx::ViewId>(viewId), &view, &proj);

		const auto& meshManager = Locator::resources::value().GetMeshes();
		const auto& renderCtx = Locator::rendereringSystem::value().GetContext();
		const auto* footprintShaderInstanced = _shaderManager->GetShader("FootprintInstanced");
		for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
		{
			auto mesh = meshManager.Handle(meshId);
			if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
			{
				continue;
			}
			const auto& footprint = mesh->GetFootprints()[0];
			footprintShaderInstanced->SetTextureSampler("s_footprint", 0, *footprint.texture);
			footprint.mesh->GetVertexBuffer().Bind();
			bgfx::setInstanceDataBuffer(renderCtx.instanceUniformBuffer, placers.offset, placers.count);
			uint64_t state = 0u                       //
			                 | BGFX_STATE_WRITE_RGB   //
			                 | BGFX_STATE_WRITE_A     //
			                 | BGFX_STATE_BLEND_ALPHA //
			                 | BGFX_STATE_CULL_CW     //
			                 | BGFX_STATE_MSAA;
			bgfx::setState(state);
			bgfx::submit(static_cast<bgfx::ViewId>(viewId), footprintShaderInstanced->GetRawHandle());
		}
	}
}

void Renderer::DrawScene(const DrawSceneDesc& drawDesc) const
{
	// TODO(bwrsandman): Footprint framebuffer doesn't need to be updated each frame
	DrawFootprintPass(drawDesc);
	// Reflection Pass
	{
		auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::ReflectionPass);
		if (drawDesc.drawWater)
		{
			DrawSceneDesc drawPassDesc = drawDesc;

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
	}

	// Main Draw Pass
	{
		auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::MainPass);
		DrawPass(drawDesc);
	}
}

void Renderer::DrawPass(const DrawSceneDesc& desc) const
{
	const auto& meshManager = Locator::resources::value().GetMeshes();

	if (desc.frameBuffer != nullptr)
	{
		desc.frameBuffer->Bind(desc.viewId);
	}
	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));

	_shaderManager->SetCamera(desc.viewId, *desc.camera);

	const auto* skyShader = _shaderManager->GetShader("Sky");
	const auto* waterShader = _shaderManager->GetShader("Water");
	const auto* terrainShader = _shaderManager->GetShader("Terrain");
	const auto* debugShader = _shaderManager->GetShader("DebugLine");
	const auto* spriteShader = _shaderManager->GetShader("Sprite");
	const auto* debugShaderInstanced = _shaderManager->GetShader("DebugLineInstanced");
	const auto* objectShaderInstanced = _shaderManager->GetShader("ObjectInstanced");
	const auto* objectShaderHeightMapInstanced = _shaderManager->GetShader("ObjectHeightMapInstanced");

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSky
		                                                                               : Profiler::Stage::MainPassDrawSky);
		if (desc.drawSky)
		{
			const auto modelMatrix = glm::mat4(1.0f);
			const glm::vec4 u_typeAlignment = {desc.sky.GetCurrentSkyType(), Game::Instance()->GetConfig().skyAlignment + 1.0f,
			                                   0.0f, 0.0f};

			skyShader->SetTextureSampler("s_diffuse", 0, *desc.sky._texture);
			skyShader->SetUniformValue("u_typeAlignment", &u_typeAlignment);

			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = desc.viewId;
			submitDesc.program = skyShader;
			submitDesc.state = BGFX_STATE_DEFAULT;
			if (!desc.cullBack)
			{
				submitDesc.state &= ~BGFX_STATE_CULL_MASK;
				submitDesc.state |= BGFX_STATE_CULL_CCW;
			}
			submitDesc.modelMatrices = &modelMatrix;
			submitDesc.matrixCount = 1;
			submitDesc.isSky = true;

			DrawMesh(*desc.sky._model, submitDesc, 0);
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawWater
		                                                                               : Profiler::Stage::MainPassDrawWater);
		if (desc.drawWater)
		{
			const auto& mesh = desc.water._mesh;
			mesh->GetIndexBuffer().Bind(mesh->GetIndexBuffer().GetCount(), 0);
			mesh->GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEFAULT);
			auto diffuse = Locator::resources::value().GetTextures().Handle(Water::k_DiffuseTextureId);
			auto alpha = Locator::resources::value().GetTextures().Handle(Water::k_AlphaTextureId);
			waterShader->SetTextureSampler("s_diffuse", 0, *diffuse);
			waterShader->SetTextureSampler("s_alpha", 1, *alpha);
			waterShader->SetTextureSampler("s_reflection", 2, desc.water.GetFrameBuffer().GetColorAttachment());
			const glm::vec4 u_sky = {desc.sky.GetCurrentSkyType(), 0.0f, 0.0f, 0.0f};
			waterShader->SetUniformValue("u_sky", &u_sky); // fs
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), waterShader->GetRawHandle());
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawIsland
		                                                                               : Profiler::Stage::MainPassDrawIsland);
		if (desc.drawIsland)
		{
			glm::vec2 extentMin;
			glm::vec2 extentMax;
			desc.island.GetExtent(extentMin, extentMax);
			auto islandExtent = glm::vec4(extentMin, extentMax);
			for (const auto& block : desc.island.GetBlocks())
			{
				auto texture = Locator::resources::value().GetTextures().Handle(LandIsland::k_SmallBumpTextureId);

				terrainShader->SetTextureSampler("s0_materials", 0, desc.island.GetAlbedoArray());
				terrainShader->SetTextureSampler("s1_bump", 1, desc.island.GetBump());
				terrainShader->SetTextureSampler("s2_smallBump", 2, *texture);
				terrainShader->SetTextureSampler("s3_footprints", 3,
				                                 desc.island.GetFootprintFramebuffer().GetColorAttachment());

				// pack uniforms
				const glm::vec4 mapPositionAndSize = glm::vec4(block.GetMapPosition(), 160.0f, 160.0f);
				terrainShader->SetUniformValue("u_blockPositionAndSize", &mapPositionAndSize); // vs
				terrainShader->SetUniformValue("u_islandExtent", &islandExtent);               // vs
				const glm::vec4 u_skyAndBump = {desc.sky.GetCurrentSkyType(), desc.bumpMapStrength, desc.smallBumpMapStrength,
				                                0.0f};
				terrainShader->SetUniformValue("u_skyAndBump", &u_skyAndBump); // fs

				// clang-format off
				constexpr auto defaultState = 0u
					| BGFX_STATE_WRITE_MASK
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_BLEND_ALPHA
					| BGFX_STATE_MSAA
				;
				// clang-format on

				block.GetMesh().GetVertexBuffer().Bind();
				bgfx::setState(defaultState | (desc.cullBack ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW), 0);
				bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), terrainShader->GetRawHandle());
			}
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawModels
		                                                                               : Profiler::Stage::MainPassDrawModels);
		if (desc.drawEntities)
		{
			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = desc.viewId;
			submitDesc.program = objectShaderInstanced;
			submitDesc.state = 0u                           //
			                   | BGFX_STATE_WRITE_MASK      //
			                   | BGFX_STATE_DEPTH_TEST_LESS //
			                   | BGFX_STATE_MSAA            //
			    ;
			const auto& renderCtx = Locator::rendereringSystem::value().GetContext();

			// Instance meshes
			for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
			{
				auto mesh = meshManager.Handle(meshId);

				submitDesc.instanceBuffer = &renderCtx.instanceUniformBuffer;
				submitDesc.instanceStart = placers.offset;
				submitDesc.instanceCount = placers.count;
				if (mesh->IsBoned())
				{
					submitDesc.modelMatrices = mesh->GetBoneMatrices().data();
					submitDesc.matrixCount = static_cast<uint8_t>(mesh->GetBoneMatrices().size());
					// TODO(bwrsandman): Get animation frame instead of default
				}
				else
				{
					const static auto identity = glm::mat4(1.0f);
					submitDesc.modelMatrices = &identity;
					submitDesc.matrixCount = 1;
				}
				submitDesc.isSky = false;
				submitDesc.skyType = desc.sky.GetCurrentSkyType();
				submitDesc.morphWithTerrain = placers.morphWithTerrain;
				submitDesc.program = submitDesc.morphWithTerrain ? objectShaderHeightMapInstanced : objectShaderInstanced;

				// TODO(bwrsandman): choose the correct LOD
				DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
			}

			// Debug
			if (desc.viewId == graphics::RenderPass::Main)
			{
				for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
				{
					auto mesh = meshManager.Handle(meshId);
					if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
					{
						continue;
					}
				}
				if (renderCtx.boundingBox)
				{
					const auto boundBoxOffset = static_cast<uint32_t>(renderCtx.instanceUniforms.size() / 2);
					const auto boundBoxCount = static_cast<uint32_t>(renderCtx.instanceUniforms.size() / 2);
					renderCtx.boundingBox->GetVertexBuffer().Bind();
					bgfx::setInstanceDataBuffer(renderCtx.instanceUniformBuffer, boundBoxOffset, boundBoxCount);
					bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShaderInstanced->GetRawHandle());
				}
				if (renderCtx.footpaths)
				{
					renderCtx.footpaths->GetVertexBuffer().Bind();
					bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
				}
				if (renderCtx.streams)
				{
					renderCtx.streams->GetVertexBuffer().Bind();
					bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
				}
			}
		}

		{
			auto subSection =
			    desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSprites
			                                                                    : Profiler::Stage::MainPassDrawSprites);

			if (desc.drawSprites)
			{
				using namespace ecs::components;

				auto& registry = Game::Instance()->GetEntityRegistry();
				registry.Each<const Sprite, const Transform>(
				    [this, &spriteShader, &desc](const Sprite& sprite, const Transform& transform) {
					    glm::mat4 modelMatrix = glm::mat4(1.0f);
					    modelMatrix = glm::translate(modelMatrix, transform.position);
					    modelMatrix *= glm::mat4(transform.rotation);
					    modelMatrix = glm::scale(modelMatrix, transform.scale);

					    glm::vec4 u_sampleRect(sprite.uvExtent, sprite.uvMin);

					    bgfx::setTransform(glm::value_ptr(modelMatrix));
					    spriteShader->SetUniformValue("u_sampleRect", glm::value_ptr(u_sampleRect));
					    spriteShader->SetUniformValue("u_tint", glm::value_ptr(sprite.tint));
					    spriteShader->SetTextureSampler("s_diffuse", 0, sprite.texture);

					    _plane->GetVertexBuffer().Bind();

					    bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_BLEND_ALPHA);

					    bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), spriteShader->GetRawHandle());
				    });
			}
		}

		if (desc.drawTestModel)
		{
			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = desc.viewId;
			submitDesc.program = _shaderManager->GetShader("Object");
			// clang-format off
			submitDesc.state = 0u
				| BGFX_STATE_WRITE_MASK
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CCW
				| BGFX_STATE_MSAA
			;
			// clang-format on
			const auto& mesh = meshManager.Handle(entt::hashed_string("coffre"));
			const auto& testAnimation = Locator::resources::value().GetAnimations().Handle(entt::hashed_string("coffre"));
			const std::vector<uint32_t>& boneParents = mesh->GetBoneParents();
			auto bones = testAnimation->GetBoneMatrices(desc.time);
			for (uint32_t i = 0; i < bones.size(); ++i)
			{
				if (boneParents[i] != std::numeric_limits<uint32_t>::max())
				{
					bones[i] = bones[boneParents[i]] * bones[i];
				}
			}
			submitDesc.modelMatrices = bones.data();
			submitDesc.matrixCount = static_cast<uint8_t>(bones.size());
			submitDesc.isSky = false;
			submitDesc.skyType = desc.sky.GetCurrentSkyType();
			DrawMesh(*mesh, submitDesc, 0);
		}
	}

	{
		auto section =
		    desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawDebugCross
		                                                                    : Profiler::Stage::MainPassDrawDebugCross);
		if (desc.drawDebugCross)
		{
			bgfx::setTransform(glm::value_ptr(_debugCrossPose));
			_debugCross->GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
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
	if (desc.bgfxProfile)
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

void Renderer::RequestScreenshot(const std::filesystem::path& filepath)
{
	bgfx::FrameBufferHandle mainBackbuffer = BGFX_INVALID_HANDLE;
	bgfx::requestScreenShot(mainBackbuffer, filepath.string().c_str());
}
