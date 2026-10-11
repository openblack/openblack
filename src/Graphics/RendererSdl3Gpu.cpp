/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RendererSdl3Gpu.h"

#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <algorithm>
#include <array>
#include <limits>
#include <new>
#include <tuple>
#include <utility>
#include <vector>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_surface.h>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "3D/OceanInterface.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "Debug/DebugGuiInterface.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "FrameBuffer.h"
#include "GraphicsHandleSdl3Gpu.h"
#include "IndexBuffer.h"
#include "Locator.h"
#include "Mesh.h"
#include "Profiler.h"
#include "Resources/ResourcesInterface.h"
#include "Texture2d.h"
#include "VertexBuffer.h"
#include "generated/shaders/footprint.slang.spv.h"
#include "generated/shaders/line.slang.spv.h"
#include "generated/shaders/object.slang.spv.h"
#include "generated/shaders/sky.slang.spv.h"
#include "generated/shaders/sprite.slang.spv.h"
#include "generated/shaders/terrain.slang.spv.h"
#include "generated/shaders/vegetation.slang.spv.h"
#include "generated/shaders/water.slang.spv.h"

using namespace openblack::graphics;

namespace
{
enum class AdditiveBlend
{
	Disabled,
	Color,
	ColorAndAlpha,
};

struct MeshCamera
{
	glm::mat4 viewProjection;
	glm::vec4 islandExtent;
	glm::uvec4 offsets;
};

template <typename T, auto Release>
struct GpuResourceDeleter
{
	SDL_GPUDevice* device;
	void operator()(T* resource) const noexcept { Release(device, resource); }
};

template <typename T, auto Release>
using GpuResource = std::unique_ptr<T, GpuResourceDeleter<T, Release>>;

using TransferBuffer = GpuResource<SDL_GPUTransferBuffer, SDL_ReleaseGPUTransferBuffer>;
using GpuBuffer = GpuResource<SDL_GPUBuffer, SDL_ReleaseGPUBuffer>;
using GpuTexture = GpuResource<SDL_GPUTexture, SDL_ReleaseGPUTexture>;
using GpuFence = GpuResource<SDL_GPUFence, SDL_ReleaseGPUFence>;
using GpuSampler = GpuResource<SDL_GPUSampler, SDL_ReleaseGPUSampler>;

TransferBuffer CreateTransferBuffer(SDL_GPUDevice* device, std::span<const uint8_t> memory)
{
	const SDL_GPUTransferBufferCreateInfo info = {
	    .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	    .size = static_cast<uint32_t>(memory.size_bytes()),
	};
	TransferBuffer transfer {SDL_CreateGPUTransferBuffer(device, &info), {device}};
	if (!transfer)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create upload buffer: {}", SDL_GetError());
		return transfer;
	}
	auto* mapped = SDL_MapGPUTransferBuffer(device, transfer.get(), false);
	if (mapped == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to map upload buffer: {}", SDL_GetError());
		return {nullptr, {device}};
	}
	std::memcpy(mapped, memory.data(), memory.size_bytes());
	SDL_UnmapGPUTransferBuffer(device, transfer.get());
	return transfer;
}
} // namespace

void RendererSdl3Gpu::SdlGpuDeleter::operator()(SDL_GPUDevice* device) const noexcept
{
	if (device != nullptr)
	{
		if (window != nullptr)
		{
			SDL_ReleaseWindowFromGPUDevice(device, window);
		}
		SDL_DestroyGPUDevice(device);
	}
}

SDL_GPUGraphicsPipeline* CreatePipeline(SDL_GPUDevice* device, SDL_GPUTextureFormat targetFormat,
                                        const SDL_GPUShaderCreateInfo& vertexShaderCreateinfo,
                                        const SDL_GPUShaderCreateInfo& fragmentShaderCreateinfo,
                                        const SDL_GPUVertexInputState& vertexInputState,
                                        SDL_GPUTextureFormat depthStencilFormat = SDL_GPU_TEXTUREFORMAT_INVALID,
                                        bool alphaBlend = false, SDL_GPUCullMode cullMode = SDL_GPU_CULLMODE_NONE,
                                        bool depthWrite = true, AdditiveBlend additiveBlend = AdditiveBlend::Disabled,
                                        SDL_GPUPrimitiveType primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST)
{

	const SDL_GPUColorTargetDescription colorTargetDescriptions = {
	    .format = targetFormat,
	    .blend_state =
	        {
	            .src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA,
	            .dst_color_blendfactor = additiveBlend != AdditiveBlend::Disabled ? SDL_GPU_BLENDFACTOR_ONE
	                                                                              : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	            .color_blend_op = SDL_GPU_BLENDOP_ADD,
	            .src_alpha_blendfactor =
	                additiveBlend == AdditiveBlend::ColorAndAlpha ? SDL_GPU_BLENDFACTOR_SRC_ALPHA : SDL_GPU_BLENDFACTOR_ONE,
	            .dst_alpha_blendfactor = additiveBlend == AdditiveBlend::ColorAndAlpha
	                                         ? SDL_GPU_BLENDFACTOR_ONE
	                                         : SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	            .alpha_blend_op = SDL_GPU_BLENDOP_ADD,
	            .enable_blend = alphaBlend,
	            .enable_color_write_mask = false,
	        },
	};

	const GpuResource<SDL_GPUShader, SDL_ReleaseGPUShader> vs {SDL_CreateGPUShader(device, &vertexShaderCreateinfo), {device}};
	if (!vs)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create vertex shader: {}", SDL_GetError());
		return nullptr;
	}
	const GpuResource<SDL_GPUShader, SDL_ReleaseGPUShader> fs {SDL_CreateGPUShader(device, &fragmentShaderCreateinfo),
	                                                           {device}};
	if (!fs)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create fragment shader: {}", SDL_GetError());
		return nullptr;
	}
	const SDL_GPUGraphicsPipelineCreateInfo createInfo = {
	    .vertex_shader = vs.get(),
	    .fragment_shader = fs.get(),
	    .vertex_input_state = vertexInputState,
	    .primitive_type = primitiveType,
	    .rasterizer_state =
	        {
	            .cull_mode = cullMode,
	            .front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
	            .enable_depth_bias = false,
	            .enable_depth_clip = true,
	        },
	    .multisample_state =
	        {
	            .sample_count = SDL_GPU_SAMPLECOUNT_1,
	            .sample_mask = 0,
	            .enable_mask = false,
	        },
	    .depth_stencil_state =
	        {
	            .compare_op = SDL_GPU_COMPAREOP_GREATER,
	            .enable_depth_test = depthStencilFormat != SDL_GPU_TEXTUREFORMAT_INVALID,
	            .enable_depth_write = depthWrite && depthStencilFormat != SDL_GPU_TEXTUREFORMAT_INVALID,
	        },
	    .target_info =
	        {
	            .color_target_descriptions = &colorTargetDescriptions,
	            .num_color_targets = 1,
	            .depth_stencil_format = depthStencilFormat,
	            .has_depth_stencil_target = depthStencilFormat != SDL_GPU_TEXTUREFORMAT_INVALID,
	        },
	};
	auto* result = SDL_CreateGPUGraphicsPipeline(device, &createInfo);
	if (result == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create graphics pipeline: {}", SDL_GetError());
	}
	return result;
}

void RendererSdl3Gpu::Pipelines::Release(SDL_GPUDevice* device) noexcept
{
	const auto release = [device](SDL_GPUGraphicsPipeline*& pipeline) {
		if (pipeline != nullptr)
		{
			SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
			pipeline = nullptr;
		}
	};
	release(sky);
	release(water);
	release(terrain);
	release(sprites);
	release(footprints);
	release(lines);
	for (auto& pipeline : objects)
	{
		release(pipeline);
	}
	for (auto& pipeline : vegetation)
	{
		release(pipeline);
	}
}

std::unique_ptr<RendererInterface> RendererSdl3Gpu::Create(bool vsync) noexcept
{
	const bool debugMode =
#ifdef OPENBLACK_DEBUG
	    true;
#else
	    false;
#endif
	auto gpuDevice = std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter> {
	    SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, debugMode, "vulkan"), {}};
	if (gpuDevice == nullptr)
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to create SDL3 GPU device: {}", SDL_GetError());
		return nullptr;
	}

	const auto& window = Locator::windowing::value();

	if (!SDL_ClaimWindowForGPUDevice(gpuDevice.get(), reinterpret_cast<SDL_Window*>(window.GetHandle())))
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to claim window for GPU device: {}", SDL_GetError());
		return nullptr;
	}
	auto* sdlWindow = static_cast<SDL_Window*>(window.GetHandle());
	gpuDevice.get_deleter().window = sdlWindow;
	const auto presentMode =
	    !vsync && SDL_WindowSupportsGPUPresentMode(gpuDevice.get(), sdlWindow, SDL_GPU_PRESENTMODE_IMMEDIATE)
	        ? SDL_GPU_PRESENTMODE_IMMEDIATE
	        : SDL_GPU_PRESENTMODE_VSYNC;
	if (!SDL_SetGPUSwapchainParameters(gpuDevice.get(), sdlWindow, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, presentMode))
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to configure swapchain: {}", SDL_GetError());
		return nullptr;
	}

	std::array<Pipelines, static_cast<size_t>(RenderPass::_count)> pipelines {};
	const auto depthFormat = GetSupportedDepthStencilFormat(gpuDevice.get(), TextureFormat::Depth24Stencil8);
	const auto mainColorFormat = SDL_GetGPUSwapchainTextureFormat(gpuDevice.get(), sdlWindow);
	auto& main = pipelines[static_cast<size_t>(RenderPass::Main)];
	main = {
	    .sky = CreatePipeline(gpuDevice.get(), mainColorFormat, k_SkyVertexShaderCreateInfo, k_SkyFragmentShaderCreateInfo,
	                          k_SkyVertexInputStateByVertex, depthFormat),
	    .water = CreatePipeline(gpuDevice.get(), mainColorFormat, k_WaterVertexShaderCreateInfo,
	                            k_WaterFragmentShaderCreateInfo, k_WaterVertexInputStateByVertex, depthFormat),
	    .terrain =
	        CreatePipeline(gpuDevice.get(), mainColorFormat, k_TerrainVertexShaderCreateInfo, k_TerrainFragmentShaderCreateInfo,
	                       k_TerrainVertexInputStateByVertex, depthFormat, true, SDL_GPU_CULLMODE_BACK),
	    .sprites = CreatePipeline(gpuDevice.get(), mainColorFormat, k_SpriteVertexShaderCreateInfo,
	                              k_SpriteFragmentShaderCreateInfo, k_SpriteVertexInputStateByVertex, depthFormat, true,
	                              SDL_GPU_CULLMODE_NONE, false, AdditiveBlend::ColorAndAlpha),
	    .lines = CreatePipeline(gpuDevice.get(), mainColorFormat, k_LineVertexShaderCreateInfo, k_LineFragmentShaderCreateInfo,
	                            k_LineVertexInputStateByVertex, depthFormat, false, SDL_GPU_CULLMODE_NONE, true,
	                            AdditiveBlend::Disabled, SDL_GPU_PRIMITIVETYPE_LINELIST),
	};
	auto& reflection = pipelines[static_cast<size_t>(RenderPass::Reflection)];
	reflection.sky = CreatePipeline(gpuDevice.get(), SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, k_SkyVertexShaderCreateInfo,
	                                k_SkyFragmentShaderCreateInfo, k_SkyVertexInputStateByVertex, depthFormat);
	reflection.terrain = CreatePipeline(gpuDevice.get(), SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, k_TerrainVertexShaderCreateInfo,
	                                    k_TerrainFragmentShaderCreateInfo, k_TerrainVertexInputStateByVertex, depthFormat, true,
	                                    SDL_GPU_CULLMODE_FRONT);
	reflection.sprites = CreatePipeline(gpuDevice.get(), SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, k_SpriteVertexShaderCreateInfo,
	                                    k_SpriteFragmentShaderCreateInfo, k_SpriteVertexInputStateByVertex, depthFormat, true,
	                                    SDL_GPU_CULLMODE_NONE, false, AdditiveBlend::ColorAndAlpha);
	auto& footprints = pipelines[static_cast<size_t>(RenderPass::Footprint)].footprints;
	footprints = CreatePipeline(gpuDevice.get(), SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, k_FootprintVertexShaderCreateInfo,
	                            k_FootprintFragmentShaderCreateInfo, k_FootprintVertexInputStateByVertex,
	                            SDL_GPU_TEXTUREFORMAT_INVALID, true, SDL_GPU_CULLMODE_BACK, false);
	bool meshPipelinesValid = true;
	for (auto viewId : {RenderPass::Main, RenderPass::Reflection})
	{
		auto& pass = pipelines.at(static_cast<size_t>(viewId));
		const auto format = viewId == RenderPass::Main ? mainColorFormat : SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
		for (size_t blend = 0; blend < 3; ++blend)
		{
			for (size_t depthWrite = 0; depthWrite < 2; ++depthWrite)
			{
				auto*& pipeline = pass.objects.at(blend * 2 + depthWrite);
				pipeline =
				    CreatePipeline(gpuDevice.get(), format, k_ObjectVertexShaderCreateInfo, k_ObjectFragmentShaderCreateInfo,
				                   k_ObjectVertexInputStateByVertex, depthFormat, blend != 0, SDL_GPU_CULLMODE_NONE,
				                   depthWrite != 0, blend == 2 ? AdditiveBlend::Color : AdditiveBlend::Disabled);
				meshPipelinesValid &= pipeline != nullptr;
				auto*& treePipeline = pass.vegetation.at(blend * 2 + depthWrite);
				treePipeline = CreatePipeline(gpuDevice.get(), format, k_VegetationVertexShaderCreateInfo,
				                              k_VegetationFragmentShaderCreateInfo, k_VegetationVertexInputStateByVertex,
				                              depthFormat, blend != 0, SDL_GPU_CULLMODE_NONE, depthWrite != 0,
				                              blend == 2 ? AdditiveBlend::Color : AdditiveBlend::Disabled);
				meshPipelinesValid &= treePipeline != nullptr;
			}
		}
	}
	auto& viewer = pipelines[static_cast<size_t>(RenderPass::MeshViewer)];
	viewer.lines = CreatePipeline(gpuDevice.get(), SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, k_LineVertexShaderCreateInfo,
	                              k_LineFragmentShaderCreateInfo, k_LineVertexInputStateByVertex, depthFormat, false,
	                              SDL_GPU_CULLMODE_NONE, true, AdditiveBlend::Disabled, SDL_GPU_PRIMITIVETYPE_LINELIST);
	const std::array requiredPipelines {main.sky,       main.water,         main.terrain,       main.sprites, main.lines,
	                                    reflection.sky, reflection.terrain, reflection.sprites, footprints,   viewer.lines};
	if (std::ranges::any_of(requiredPipelines, [](const auto* pipeline) { return pipeline == nullptr; }) || !meshPipelinesValid)
	{
		for (auto& pass : pipelines)
		{
			pass.Release(gpuDevice.get());
		}
		return nullptr;
	}

	auto renderer = std::unique_ptr<RendererSdl3Gpu> {new (std::nothrow) RendererSdl3Gpu(std::move(gpuDevice), pipelines)};
	if (renderer == nullptr)
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to allocate RendererSdl3Gpu");
		return nullptr;
	}
	{
		struct Vertex
		{
			glm::vec2 position;
		};
		static constexpr std::array<Vertex, 6> k_Vertices {{{{-1, -1}}, {{1, 1}}, {{-1, 1}}, {{-1, -1}}, {{1, -1}}, {{1, 1}}}};
		VertexDecl decl;
		decl.emplace_back(VertexAttrib::Attribute::Position, 2, VertexAttrib::Type::Float);
		const auto bytes = std::span<const uint8_t> {reinterpret_cast<const uint8_t*>(k_Vertices.data()), sizeof(k_Vertices)};
		auto buffer = renderer->CreateVertexBuffer("Sprite quad", bytes, decl);
		if (!buffer)
		{
			return nullptr;
		}
		renderer->_spriteQuad = std::make_unique<Mesh>(std::move(buffer));
	}

	const std::array<uint8_t, 4> white {255, 255, 255, 255};
	renderer->_whiteTexture = renderer->CreateTexture2d("Untextured objects", white, {1, 1}, 1, TextureFormat::RGBA8,
	                                                    Wrapping::ClampEdge, Filter::Nearest);
	if (!renderer->_whiteTexture)
	{
		return nullptr;
	}
	return renderer;
}

RendererSdl3Gpu::RendererSdl3Gpu(std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter>&& device,
                                 const std::array<Pipelines, static_cast<size_t>(RenderPass::_count)>& pipelines) noexcept
    : _gpuDevice(std::move(device))
    , _pipelines(pipelines)
{
}

RendererSdl3Gpu::~RendererSdl3Gpu() noexcept
{
	if (_sceneMatrices != nullptr)
	{
		SDL_ReleaseGPUBuffer(_gpuDevice.get(), _sceneMatrices);
	}
	if (_depthStencil != nullptr)
	{
		SDL_ReleaseGPUTexture(_gpuDevice.get(), _depthStencil);
	}
	for (auto& pass : _pipelines)
	{
		pass.Release(_gpuDevice.get());
	}
}

bool RendererSdl3Gpu::ResizeDepthStencil(glm::uvec2 resolution) noexcept
{
	if (_depthStencil != nullptr && _depthStencilResolution == resolution)
	{
		return true;
	}
	const SDL_GPUTextureCreateInfo info {
	    .type = SDL_GPU_TEXTURETYPE_2D,
	    .format = GetSupportedDepthStencilFormat(_gpuDevice.get(), TextureFormat::Depth24Stencil8),
	    .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
	    .width = resolution.x,
	    .height = resolution.y,
	    .layer_count_or_depth = 1,
	    .num_levels = 1,
	    .sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	auto* texture = SDL_CreateGPUTexture(_gpuDevice.get(), &info);
	if (texture == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create main depth/stencil target: {}", SDL_GetError());
		return false;
	}
	SDL_SetGPUTextureName(_gpuDevice.get(), texture, "Main_depthStencil");
	if (_depthStencil != nullptr)
	{
		SDL_ReleaseGPUTexture(_gpuDevice.get(), _depthStencil);
	}
	_depthStencil = texture;
	_depthStencilResolution = resolution;
	return true;
}

void RendererSdl3Gpu::DrawScene(const DrawSceneDesc& drawDesc) noexcept
{
	auto& profiler = Locator::profiler::value();

	SDL_GPUCommandBuffer* cmdBufferMain = SDL_AcquireGPUCommandBuffer(_gpuDevice.get());
	if (cmdBufferMain == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to acquire command buffer: {}", SDL_GetError());
		return;
	}

	const auto& window = Locator::windowing::value();
	SDL_GPUTexture* swapchainTexture = nullptr;
	glm::uvec2 resolution {};
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBufferMain, reinterpret_cast<SDL_Window*>(window.GetHandle()),
	                                           &swapchainTexture, &resolution.x, &resolution.y))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to acquire swapchain texture: {}", SDL_GetError());
		SDL_CancelGPUCommandBuffer(cmdBufferMain);
		return;
	}
	// Minimized windows have no drawable swapchain texture. Submit the empty buffer
	// so it is retired without opening a render pass on a null target.
	if (swapchainTexture == nullptr)
	{
		SDL_SubmitGPUCommandBuffer(cmdBufferMain);
		return;
	}
	if (!ResizeDepthStencil(resolution))
	{
		SDL_SubmitGPUCommandBuffer(cmdBufferMain);
		return;
	}
	if ((drawDesc.drawEntities || drawDesc.drawIsland || drawDesc.drawVegetation || drawDesc.drawBoundingBoxes ||
	     drawDesc.drawFootpaths || drawDesc.drawStreams) &&
	    !UploadSceneMatrices(cmdBufferMain))
	{
		SDL_SubmitGPUCommandBuffer(cmdBufferMain);
		return;
	}
	DrawSceneDesc mainDesc = drawDesc;
	const auto& textures = Locator::resources::value().GetTextures();
	mainDesc.drawWater = drawDesc.drawWater && Locator::oceanSystem::has_value() &&
	                     textures.Contains(Locator::oceanSystem::value().GetDiffuseTexture()) &&
	                     textures.Contains(Locator::oceanSystem::value().GetAlphaTexture());
	if (mainDesc.drawIsland)
	{
		auto section = profiler.BeginScoped(Profiler::Stage::FootprintPass);
		const auto& island = Locator::terrainSystem::value();
		const auto& footprint = island.GetFootprintFramebuffer();
		const SDL_GPUColorTargetInfo target {
		    .texture = toSdl(footprint.colorAttachment),
		    .clear_color = {0, 0, 0, 0},
		    .load_op = SDL_GPU_LOADOP_CLEAR,
		    .store_op = SDL_GPU_STOREOP_STORE,
		    .cycle = true,
		};
		auto* pass = SDL_BeginGPURenderPass(cmdBufferMain, &target, 1, nullptr);
		if (pass == nullptr)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin footprint pass: {}", SDL_GetError());
			SDL_SubmitGPUCommandBuffer(cmdBufferMain);
			return;
		}
		const auto& context = Locator::rendereringSystem::value().GetContext();
		const auto& meshes = Locator::resources::value().GetMeshes();
		const auto viewProjection = island.GetOrthoProj() * island.GetOrthoView();
		auto* const pipeline = _pipelines[static_cast<size_t>(RenderPass::Footprint)].footprints;
		if (!context.instancedDrawDescs.empty() || !context.treeInstancedDrawDescs.empty())
		{
			SDL_BindGPUVertexStorageBuffers(pass, 0, &_sceneMatrices, 1);
		}
		const auto drawFootprints = [&](entt::id_type meshId, const auto& instances, uint32_t matrixOffset) {
			if (!meshes.Contains(meshId) || instances.count == 0)
			{
				return;
			}
			const auto mesh = meshes.Handle(meshId);
			if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
			{
				return;
			}
			const auto& decal = mesh->GetFootprints().front();
			if (!decal.texture || !decal.mesh || !decal.mesh->HasVertexBuffer())
			{
				return;
			}
			const SDL_GPUTextureSamplerBinding binding {toSdl(decal.texture->handle), toSdl(decal.texture->samplerHandle)};
			SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
			struct FootprintData
			{
				glm::mat4 viewProjection;
				glm::uvec4 offsets;
			};
			const FootprintData data {viewProjection, {matrixOffset + instances.offset, 0, 0, 0}};
			SDL_PushGPUVertexUniformData(cmdBufferMain, 0, &data, sizeof(data));
			DrawGeometry(pass, pipeline, *decal.mesh, instances.count);
		};
		for (const auto& [key, instances] : context.instancedDrawDescs)
		{
			drawFootprints(key.meshId, instances, 0);
		}
		for (const auto& [meshId, instances] : context.treeInstancedDrawDescs)
		{
			drawFootprints(meshId, instances, _treeMatrixOffset);
		}
		SDL_EndGPURenderPass(pass);
	}
	if (mainDesc.drawWater)
	{
		auto section = profiler.BeginScoped(Profiler::Stage::ReflectionPass);
		const auto reflectedCamera = drawDesc.camera->Reflect();
		DrawSceneDesc reflectionDesc = mainDesc;
		reflectionDesc.viewId = RenderPass::Reflection;
		reflectionDesc.camera = reflectedCamera.get();
		reflectionDesc.frameBuffer = &Locator::oceanSystem::value().GetReflectionFramebuffer();
		reflectionDesc.drawWater = false;
		reflectionDesc.drawBoundingBoxes = false;
		reflectionDesc.cullBack = true;
		if (!DrawPass(reflectionDesc, cmdBufferMain, nullptr))
		{
			SDL_SubmitGPUCommandBuffer(cmdBufferMain);
			return;
		}
	}
	GpuTexture screenshotTexture {nullptr, {_gpuDevice.get()}};
	TransferBuffer screenshotTransfer {nullptr, {_gpuDevice.get()}};
	SDL_GPUTextureFormat screenshotFormat = SDL_GPU_TEXTUREFORMAT_INVALID;
	// Swapchain textures are write-only. Capture from a matching render target.
	if (_screenshotPath)
	{
		screenshotFormat =
		    SDL_GetGPUSwapchainTextureFormat(_gpuDevice.get(), reinterpret_cast<SDL_Window*>(window.GetHandle()));
		const uint64_t size = static_cast<uint64_t>(resolution.x) * resolution.y * 4;
		if (size > std::numeric_limits<uint32_t>::max() || (screenshotFormat != SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM &&
		                                                    screenshotFormat != SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Unsupported screenshot format or dimensions");
			_screenshotPath.reset();
		}
		else
		{
			const SDL_GPUTextureCreateInfo textureInfo {
			    .type = SDL_GPU_TEXTURETYPE_2D,
			    .format = screenshotFormat,
			    .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
			    .width = resolution.x,
			    .height = resolution.y,
			    .layer_count_or_depth = 1,
			    .num_levels = 1,
			};
			const SDL_GPUTransferBufferCreateInfo transferInfo {.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD,
			                                                    .size = static_cast<uint32_t>(size)};
			screenshotTexture.reset(SDL_CreateGPUTexture(_gpuDevice.get(), &textureInfo));
			screenshotTransfer.reset(SDL_CreateGPUTransferBuffer(_gpuDevice.get(), &transferInfo));
			if (!screenshotTexture || !screenshotTransfer)
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to allocate screenshot buffers: {}", SDL_GetError());
				_screenshotPath.reset();
			}
		}
	}
	const bool capture = _screenshotPath.has_value();
	{
		auto section = profiler.BeginScoped(Profiler::Stage::MainPass);
		if (!DrawPass(mainDesc, cmdBufferMain, capture ? screenshotTexture.get() : swapchainTexture, _depthStencil))
		{
			SDL_SubmitGPUCommandBuffer(cmdBufferMain);
			return;
		}
	}

	if (!capture)
	{
		auto section = profiler.BeginScoped(Profiler::Stage::GuiDraw);
		Locator::debugGui::value().Draw(cmdBufferMain, swapchainTexture);
		if (!SDL_SubmitGPUCommandBuffer(cmdBufferMain))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to submit frame: {}", SDL_GetError());
		}
		return;
	}
	auto* copyPass = SDL_BeginGPUCopyPass(cmdBufferMain);
	if (copyPass == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin screenshot readback: {}", SDL_GetError());
		SDL_SubmitGPUCommandBuffer(cmdBufferMain);
		_screenshotPath.reset();
		return;
	}
	const SDL_GPUTextureLocation source {.texture = screenshotTexture.get()};
	const SDL_GPUTextureLocation destination {.texture = swapchainTexture};
	SDL_CopyGPUTextureToTexture(copyPass, &source, &destination, resolution.x, resolution.y, 1, false);
	const SDL_GPUTextureRegion region {.texture = screenshotTexture.get(), .w = resolution.x, .h = resolution.y, .d = 1};
	const SDL_GPUTextureTransferInfo download {.transfer_buffer = screenshotTransfer.get()};
	SDL_DownloadFromGPUTexture(copyPass, &region, &download);
	SDL_EndGPUCopyPass(copyPass);
	const GpuFence fence {SDL_SubmitGPUCommandBufferAndAcquireFence(cmdBufferMain), {_gpuDevice.get()}};
	auto* fenceHandle = fence.get();
	const auto path = std::move(*_screenshotPath);
	_screenshotPath.reset();
	if (!fence || !SDL_WaitForGPUFences(_gpuDevice.get(), true, &fenceHandle, 1))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to complete screenshot readback: {}", SDL_GetError());
		return;
	}
	auto* pixels = static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(_gpuDevice.get(), screenshotTransfer.get(), false));
	if (pixels == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to map screenshot pixels: {}", SDL_GetError());
		return;
	}
	// Screenshots are opaque regardless of the alpha written by scene materials.
	for (uint64_t i = 3; i < static_cast<uint64_t>(resolution.x) * resolution.y * 4; i += 4)
	{
		pixels[i] = 255;
	}
	const auto format =
	    screenshotFormat == SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM ? SDL_PIXELFORMAT_BGRA32 : SDL_PIXELFORMAT_RGBA32;
	std::unique_ptr<SDL_Surface, decltype(&SDL_DestroySurface)> surface {
	    SDL_CreateSurfaceFrom(static_cast<int>(resolution.x), static_cast<int>(resolution.y), format, pixels,
	                          static_cast<int>(resolution.x * 4)),
	    SDL_DestroySurface};
	const auto filename = path.string();
	const bool saved = surface && SDL_SavePNG(surface.get(), filename.c_str());
	if (!saved)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to save screenshot at {}: {}", filename, SDL_GetError());
	}
	surface.reset();
	SDL_UnmapGPUTransferBuffer(_gpuDevice.get(), screenshotTransfer.get());
	if (!saved)
	{
		return;
	}
	SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Screenshot ({}x{}) saved at {}", resolution.x, resolution.y, filename);
}

bool RendererSdl3Gpu::DrawPass(const DrawSceneDesc& desc, SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* defaultTarget,
                               SDL_GPUTexture* defaultDepthStencil) const noexcept
{
	auto& profiler = Locator::profiler::value();
	const auto& camera = *desc.camera;
	const auto& textures = Locator::resources::value().GetTextures();
	const auto& pipelines = _pipelines.at(static_cast<size_t>(desc.viewId));
	const bool isReflection = desc.viewId == RenderPass::Reflection;
	const auto* frameBuffer = desc.frameBuffer;
	const SDL_GPUColorTargetInfo colorTarget = {
	    .texture = frameBuffer != nullptr ? toSdl(frameBuffer->colorAttachment) : defaultTarget,
	    .clear_color = {0x27 / 255.0f, 0x46 / 255.0f, 0x59 / 255.0f, 1.0f},
	    .load_op = SDL_GPU_LOADOP_CLEAR,
	    .store_op = SDL_GPU_STOREOP_STORE,
	    .cycle = frameBuffer != nullptr,
	};
	SDL_GPUDepthStencilTargetInfo depthStencil {};
	auto* depthTexture = frameBuffer != nullptr ? toSdl(frameBuffer->depthStencilAttachment) : defaultDepthStencil;
	const bool hasDepthStencil = depthTexture != nullptr;
	if (hasDepthStencil)
	{
		depthStencil = {
		    .texture = depthTexture,
		    .clear_depth = 0.0f,
		    .load_op = SDL_GPU_LOADOP_CLEAR,
		    .store_op = SDL_GPU_STOREOP_STORE,
		    .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
		    .stencil_store_op = SDL_GPU_STOREOP_STORE,
		    .cycle = true,
		    .clear_stencil = 0,
		};
	}
	auto* renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, hasDepthStencil ? &depthStencil : nullptr);
	if (renderPass == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin {}: {}",
		                    k_RenderPassNames.at(static_cast<size_t>(desc.viewId)), SDL_GetError());
		return false;
	}

	auto view = camera.GetViewMatrix(Camera::Interpolation::Current);
	auto proj = camera.GetProjectionMatrix();
	auto viewProj = proj * view;

	{
		auto section =
		    profiler.BeginScoped(isReflection ? Profiler::Stage::ReflectionDrawSky : Profiler::Stage::MainPassDrawSky);
		if (desc.drawSky)
		{
			DrawSky(commandBuffer, renderPass, pipelines.sky, camera);
		}
	}

	{
		auto section =
		    profiler.BeginScoped(isReflection ? Profiler::Stage::ReflectionDrawWater : Profiler::Stage::MainPassDrawWater);
		if (desc.drawWater)
		{
			const auto& ocean = Locator::oceanSystem::value();
			const auto diffuse = textures.Handle(ocean.GetDiffuseTexture());
			const auto alpha = textures.Handle(ocean.GetAlphaTexture());
			const auto& reflection = ocean.GetReflectionFramebuffer();
			const std::array<SDL_GPUTextureSamplerBinding, 3> bindings {{
			    {toSdl(diffuse->handle), toSdl(diffuse->samplerHandle)},
			    {toSdl(alpha->handle), toSdl(alpha->samplerHandle)},
			    {toSdl(reflection.colorAttachment), toSdl(reflection.colorSampler)},
			}};
			SDL_BindGPUFragmentSamplers(renderPass, 0, bindings.data(), static_cast<uint32_t>(bindings.size()));
			struct WaterCamera
			{
				glm::mat4 view;
				glm::mat4 viewProjection;
			};
			const WaterCamera waterCamera {view, viewProj};
			const float skyType = Locator::skySystem::value().GetCurrentSkyType();
			SDL_PushGPUVertexUniformData(commandBuffer, 0, &waterCamera, sizeof(waterCamera));
			SDL_PushGPUFragmentUniformData(commandBuffer, 0, &skyType, sizeof(skyType));
			DrawGeometry(renderPass, pipelines.water, ocean.GetMesh());
		}
	}

	{
		auto section =
		    profiler.BeginScoped(isReflection ? Profiler::Stage::ReflectionDrawIsland : Profiler::Stage::MainPassDrawIsland);
		if (desc.drawIsland)
		{
			const auto& island = Locator::terrainSystem::value();
			const auto smallBump = textures.Handle(LandIslandInterface::k_SmallBumpTextureId);
			const auto& albedo = island.GetAlbedoArray();
			const auto& bump = island.GetBump();
			const auto& footprint = island.GetFootprintFramebuffer();
			const std::array<SDL_GPUTextureSamplerBinding, 4> bindings {{
			    {toSdl(albedo.handle), toSdl(albedo.samplerHandle)},
			    {toSdl(bump.handle), toSdl(bump.samplerHandle)},
			    {toSdl(smallBump->handle), toSdl(smallBump->samplerHandle)},
			    {toSdl(footprint.colorAttachment), toSdl(footprint.colorSampler)},
			}};
			SDL_BindGPUFragmentSamplers(renderPass, 0, bindings.data(), static_cast<uint32_t>(bindings.size()));
			const std::array<glm::mat4, 2> cameraData {view, viewProj};
			const glm::vec4 skyAndBump {Locator::skySystem::value().GetCurrentSkyType(), desc.bumpMapStrength,
			                            desc.smallBumpMapStrength, 0.0f};
			SDL_PushGPUVertexUniformData(commandBuffer, 0, cameraData.data(), sizeof(cameraData));
			SDL_PushGPUFragmentUniformData(commandBuffer, 0, &skyAndBump, sizeof(skyAndBump));
			struct BlockData
			{
				glm::vec4 positionAndSize;
				glm::vec4 islandExtent;
			};
			const auto extent = island.GetExtent();
			for (const auto& block : island.GetBlocks())
			{
				const BlockData data {glm::vec4(block.GetMapPosition(),
				                                LandIslandInterface::k_CellSize * LandIslandInterface::k_CellCount,
				                                LandIslandInterface::k_CellSize * LandIslandInterface::k_CellCount),
				                      glm::vec4(extent.minimum, extent.maximum)};
				SDL_PushGPUVertexUniformData(commandBuffer, 1, &data, sizeof(data));
				DrawGeometry(renderPass, pipelines.terrain, block.GetMesh());
			}
		}
	}

	const auto& context = Locator::rendereringSystem::value().GetContext();
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto drawMeshBatch = [&](entt::id_type meshId, const auto& instances, glm::uvec4 offsets, const auto& meshPipelines,
	                               bool vegetation) {
		if (instances.count == 0 || !meshes.Contains(meshId) || (!vegetation && !_entityBoneOffsets.contains(meshId)))
		{
			return;
		}
		const auto mesh = meshes.Handle(meshId);
		const auto& island = Locator::terrainSystem::value();
		const auto& heightMap = island.GetHeightMap();
		const auto extent = island.GetExtent();
		const SDL_GPUTextureSamplerBinding heightBinding {toSdl(heightMap.handle), toSdl(heightMap.samplerHandle)};
		SDL_BindGPUVertexSamplers(renderPass, 0, &heightBinding, 1);
		const std::array<SDL_GPUBuffer*, 2> buffers {_sceneMatrices, _sceneMatrices};
		SDL_BindGPUVertexStorageBuffers(renderPass, 0, buffers.data(), vegetation ? 2 : 1);
		if (!vegetation)
		{
			offsets.y = _entityBoneOffsets.at(meshId);
			offsets.z =
			    mesh->IsBoned() && !mesh->GetBoneMatrices().empty() ? static_cast<uint32_t>(mesh->GetBoneMatrices().size()) : 1;
		}
		const MeshCamera data {viewProj, glm::vec4(extent.minimum, extent.maximum), offsets};
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &data, sizeof(data));
		DrawMeshPrimitives(commandBuffer, renderPass, *mesh, meshPipelines, instances.count,
		                   std::numeric_limits<uint8_t>::max(), false);
	};

	{
		auto section =
		    profiler.BeginScoped(isReflection ? Profiler::Stage::ReflectionDrawModels : Profiler::Stage::MainPassDrawModels);
		if (desc.drawEntities)
		{
			for (const auto& [key, instances] : context.instancedDrawDescs)
			{
				drawMeshBatch(key.meshId, instances, {instances.offset, 0, 0, instances.morphWithTerrain ? 1u : 0u},
				              pipelines.objects, false);
			}
		}
	}
	{
		auto section = profiler.BeginScoped(isReflection ? Profiler::Stage::ReflectionDrawVegetation
		                                                 : Profiler::Stage::MainPassDrawVegetation);
		if (desc.drawVegetation)
		{
			for (const auto& [meshId, instances] : context.treeInstancedDrawDescs)
			{
				drawMeshBatch(meshId, instances,
				              {_treeMatrixOffset + instances.offset, _treeSwayOffset + instances.offset, 0, 0},
				              pipelines.vegetation, true);
			}
		}
	}
	if (desc.viewId == RenderPass::Main)
	{
		const auto drawLines = [&](const Mesh* mesh, uint32_t offset, uint32_t count) {
			if (mesh == nullptr || !mesh->HasVertexBuffer() || count == 0)
			{
				return;
			}
			struct LineData
			{
				glm::mat4 viewProjection;
				glm::uvec4 offsets;
			};
			const LineData data {viewProj, {offset, 0, 0, 0}};
			SDL_PushGPUVertexUniformData(commandBuffer, 0, &data, sizeof(data));
			SDL_BindGPUVertexStorageBuffers(renderPass, 0, &_sceneMatrices, 1);
			DrawGeometry(renderPass, pipelines.lines, *mesh, count);
		};
		if (desc.drawBoundingBoxes && context.hasBoundingBoxes)
		{
			const auto objectCount = static_cast<uint32_t>(context.instanceUniforms.size() / 2);
			drawLines(context.boundingBox.get(), objectCount, objectCount);
			drawLines(context.boundingBox.get(), _treeMatrixOffset + context.treeInstanceCount, context.treeInstanceCount);
		}
		if (desc.drawFootpaths)
		{
			drawLines(context.footpaths.get(), _identityMatrixOffset, 1);
		}
		if (desc.drawStreams)
		{
			drawLines(context.streams.get(), _identityMatrixOffset, 1);
		}
	}
	{
		auto section =
		    profiler.BeginScoped(isReflection ? Profiler::Stage::ReflectionDrawSprites : Profiler::Stage::MainPassDrawSprites);
		if (desc.drawSprites)
		{
			const auto inverseView = glm::inverse(view);
			std::map<uint64_t, SDL_GPUTextureSamplerBinding> bindings;
			desc.entities.Each<const ecs::components::Sprite, const ecs::components::Transform>(
			    [&](const ecs::components::Sprite& sprite, const ecs::components::Transform& transform) {
				    auto [binding, inserted] = bindings.try_emplace(sprite.texture.id);
				    if (inserted)
				    {
					    textures.Each([&](entt::id_type, const auto& resource) {
						    if (resource && resource->handle.id == sprite.texture.id)
						    {
							    binding->second = {toSdl(resource->handle), toSdl(resource->samplerHandle)};
						    }
					    });
				    }
				    if (binding->second.texture == nullptr || binding->second.sampler == nullptr)
				    {
					    return;
				    }
				    glm::mat4 model {1.0f};
				    model = glm::translate(model, transform.position);
				    model *= glm::mat4(transform.rotation);
				    model = glm::scale(model, transform.scale);
				    struct SpriteData
				    {
					    glm::mat4 viewProjection;
					    glm::mat4 model;
					    glm::mat4 inverseView;
					    glm::vec4 sampleRect;
				    };
				    const SpriteData data {viewProj, model, inverseView, glm::vec4(sprite.uvExtent, sprite.uvMin)};
				    SDL_PushGPUVertexUniformData(commandBuffer, 0, &data, sizeof(data));
				    SDL_PushGPUFragmentUniformData(commandBuffer, 0, &sprite.tint, sizeof(sprite.tint));
				    SDL_BindGPUFragmentSamplers(renderPass, 0, &binding->second, 1);
				    DrawGeometry(renderPass, pipelines.sprites, *_spriteQuad);
			    });
		}
	}

	SDL_EndGPURenderPass(renderPass);
	return true;
}

bool RendererSdl3Gpu::UploadSceneMatrices(SDL_GPUCommandBuffer* commandBuffer) noexcept
{
	const auto& context = Locator::rendereringSystem::value().GetContext();
	_entityBoneOffsets.clear();
	if (context.instancedDrawDescs.empty() && context.treeInstancedDrawDescs.empty() && !context.footpaths && !context.streams)
	{
		return true;
	}
	// Keep each instance array in prepared batch order, then append rest-pose bones.
	// The footprint, main and reflection passes share this upload.
	std::vector<glm::mat4> matrices = context.instanceUniforms;
	_treeMatrixOffset = static_cast<uint32_t>(matrices.size());
	for (const auto& instance : context.treeInstanceData)
	{
		matrices.push_back(instance.modelMatrix);
	}
	const auto identityOffset = static_cast<uint32_t>(matrices.size());
	_identityMatrixOffset = identityOffset;
	matrices.emplace_back(1.0f);
	const auto& meshes = Locator::resources::value().GetMeshes();
	for (const auto& [key, instances] : context.instancedDrawDescs)
	{
		if (_entityBoneOffsets.contains(key.meshId) || !meshes.Contains(key.meshId))
		{
			continue;
		}
		const auto mesh = meshes.Handle(key.meshId);
		const auto& bones = mesh->GetBoneMatrices();
		if (mesh->IsBoned() && !bones.empty())
		{
			_entityBoneOffsets.emplace(key.meshId, static_cast<uint32_t>(matrices.size()));
			matrices.insert(matrices.end(), bones.begin(), bones.end());
		}
		else
		{
			_entityBoneOffsets.emplace(key.meshId, identityOffset);
		}
	}
	const auto matrixBytes = matrices.size() * sizeof(glm::mat4);
	const auto swayBytes = context.treeInstanceData.size() * sizeof(glm::vec4);
	if (matrixBytes > std::numeric_limits<uint32_t>::max() || swayBytes > std::numeric_limits<uint32_t>::max() - matrixBytes)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Scene matrix buffer is too large");
		return false;
	}
	// Bind this storage buffer as matrices and as vec4s; sway data follows the matrices.
	_treeSwayOffset = static_cast<uint32_t>(matrixBytes / sizeof(glm::vec4));
	std::vector<uint8_t> memory(matrixBytes + swayBytes);
	std::memcpy(memory.data(), matrices.data(), matrixBytes);
	for (size_t i = 0; i < context.treeInstanceData.size(); ++i)
	{
		std::memcpy(memory.data() + matrixBytes + i * sizeof(glm::vec4), &context.treeInstanceData[i].swayParams,
		            sizeof(glm::vec4));
	}
	if (memory.size() > _sceneMatrixCapacity)
	{
		const SDL_GPUBufferCreateInfo info {.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ,
		                                    .size = static_cast<uint32_t>(memory.size())};
		auto* buffer = SDL_CreateGPUBuffer(_gpuDevice.get(), &info);
		if (buffer == nullptr)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create scene matrices: {}", SDL_GetError());
			return false;
		}
		if (_sceneMatrices != nullptr)
		{
			SDL_ReleaseGPUBuffer(_gpuDevice.get(), _sceneMatrices);
		}
		_sceneMatrices = buffer;
		_sceneMatrixCapacity = info.size;
		SDL_SetGPUBufferName(_gpuDevice.get(), buffer, "Scene instances, bones and tree sway");
	}
	auto transfer = CreateTransferBuffer(_gpuDevice.get(), memory);
	if (!transfer)
	{
		return false;
	}
	auto* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (copyPass == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin scene upload: {}", SDL_GetError());
		return false;
	}
	const SDL_GPUTransferBufferLocation source {.transfer_buffer = transfer.get()};
	const SDL_GPUBufferRegion destination {.buffer = _sceneMatrices, .size = static_cast<uint32_t>(memory.size())};
	// Cycle when prior frames still reference the buffer; all passes below use
	// this frame's matrices without overwriting an in-flight draw.
	SDL_UploadToGPUBuffer(copyPass, &source, &destination, true);
	SDL_EndGPUCopyPass(copyPass);
	return true;
}

void RendererSdl3Gpu::DrawSky(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                              SDL_GPUGraphicsPipeline* pipeline, const Camera& camera) const
{
	const auto& sky = Locator::skySystem::value();
	const auto viewProjection = camera.GetViewProjectionMatrix(Camera::Interpolation::Current);
	const std::array<float, 2> skyData {sky.GetCurrentSkyType(), Locator::config::value().skyAlignment + 1.0f};
	const auto& texture = sky.GetTexture();
	const SDL_GPUTextureSamplerBinding binding {toSdl(texture.handle), toSdl(texture.samplerHandle)};
	SDL_BindGPUFragmentSamplers(renderPass, 0, &binding, 1);
	SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewProjection, sizeof(viewProjection));
	SDL_PushGPUFragmentUniformData(commandBuffer, 0, skyData.data(), sizeof(skyData));
	DrawGeometry(renderPass, pipeline, sky.GetMesh().GetSubMeshes()[0]->GetMesh());
}

void RendererSdl3Gpu::DrawGeometry(SDL_GPURenderPass* renderPass, SDL_GPUGraphicsPipeline* pipeline, const Mesh& mesh,
                                   uint32_t instanceCount) const
{
	{
		const SDL_GPUBufferBinding bindings = {
		    .buffer = toSdl(mesh.GetVertexBuffer().handle),
		    .offset = 0,
		};
		SDL_BindGPUVertexBuffers(renderPass, 0, &bindings, 1);
	}
	if (mesh.IsIndexed())
	{
		const SDL_GPUBufferBinding bindings = {
		    .buffer = toSdl(mesh.GetIndexBuffer().handle),
		    .offset = 0,
		};
		SDL_BindGPUIndexBuffer(renderPass, &bindings,
		                       mesh.GetIndexBuffer().type == IndexBuffer::Type::Uint16 ? SDL_GPU_INDEXELEMENTSIZE_16BIT
		                                                                               : SDL_GPU_INDEXELEMENTSIZE_32BIT);
	}
	SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
	if (mesh.IsIndexed())
	{
		SDL_DrawGPUIndexedPrimitives(renderPass, mesh.GetIndexBuffer().count, instanceCount, 0, 0, 0);
	}
	else
	{
		SDL_DrawGPUPrimitives(renderPass, mesh.GetVertexBuffer().vertexCount, instanceCount, 0, 0);
	}
}

void RendererSdl3Gpu::Frame() noexcept
{
	// DrawScene submits and waits for an available swapchain image.
}

void RendererSdl3Gpu::RequestScreenshot(const std::filesystem::path& filepath) noexcept
{
	if (filepath.extension() != ".png")
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Screenshot output must be a PNG file: {}", filepath.string());
		return;
	}
	_screenshotPath = filepath;
}

[[nodiscard]] bool RendererSdl3Gpu::GetDebug() const noexcept
{
	// TODO(bwrsandman): Implement debug mode
	return false;
}

void RendererSdl3Gpu::SetDebug([[maybe_unused]] bool value) noexcept
{
	// TODO(bwrsandman): Implement debug mode
}

[[nodiscard]] bool RendererSdl3Gpu::GetProfile() const noexcept
{
	// TODO(bwrsandman): Implement profiling
	return false;
}

void RendererSdl3Gpu::SetProfile([[maybe_unused]] bool value) noexcept
{
	// TODO(bwrsandman): Implement profiling
}

void RendererSdl3Gpu::DrawMeshPrimitives(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass,
                                         const L3DMesh& mesh, const std::array<SDL_GPUGraphicsPipeline*, 6>& pipelines,
                                         uint32_t instanceCount, uint8_t subMeshIndex, bool drawAll) const
{
	const auto& textures = Locator::resources::value().GetTextures();
	for (size_t index = 0; const auto& subMesh : mesh.GetSubMeshes())
	{
		const auto currentIndex = index++;
		if (subMeshIndex != std::numeric_limits<uint8_t>::max() && currentIndex != subMeshIndex)
		{
			continue;
		}
		const auto flags = subMesh->GetFlags();
		if (!drawAll && (flags.isPhysics || flags.status != 0 || (flags.lodMask & 1) == 0))
		{
			continue;
		}
		const auto& geometry = subMesh->GetMesh();
		const SDL_GPUBufferBinding vertices {toSdl(geometry.GetVertexBuffer().handle), 0};
		const SDL_GPUBufferBinding indices {toSdl(geometry.GetIndexBuffer().handle), 0};
		SDL_BindGPUVertexBuffers(renderPass, 0, &vertices, 1);
		SDL_BindGPUIndexBuffer(renderPass, &indices, SDL_GPU_INDEXELEMENTSIZE_16BIT);
		for (const auto& primitive : subMesh->GetPrimitives())
		{
			const Texture2d* texture = _whiteTexture.get();
			if (const auto skin = mesh.GetSkins().find(primitive.skinID); skin != mesh.GetSkins().end())
			{
				texture = skin->second.get();
			}
			else if (textures.Contains(primitive.skinID))
			{
				texture = textures.Handle(primitive.skinID).operator->();
			}
			const SDL_GPUTextureSamplerBinding diffuse {toSdl(texture->handle), toSdl(texture->samplerHandle)};
			SDL_BindGPUFragmentSamplers(renderPass, 0, &diffuse, 1);
			const glm::vec4 skyAlpha {Locator::skySystem::value().GetCurrentSkyType(),
			                          primitive.thresholdAlpha ? primitive.alphaCutoutThreshold : 0.0f, 0, 0};
			SDL_PushGPUFragmentUniformData(commandBuffer, 0, &skyAlpha, sizeof(skyAlpha));
			const auto pipelineIndex = static_cast<size_t>(primitive.blend) * 2 + (primitive.depthWrite ? 1 : 0);
			SDL_BindGPUGraphicsPipeline(renderPass, pipelines.at(pipelineIndex));
			SDL_DrawGPUIndexedPrimitives(renderPass, primitive.indicesCount, instanceCount, primitive.indicesOffset, 0, 0);
		}
	}
}

void RendererSdl3Gpu::DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept
{
	if (desc.camera == nullptr || desc.frameBuffer == nullptr || !desc.frameBuffer->depthStencilFormat ||
	    desc.frameBuffer->colorFormat != TextureFormat::RGBA8 ||
	    toSdl(*desc.frameBuffer->depthStencilFormat) !=
	        GetSupportedDepthStencilFormat(_gpuDevice.get(), TextureFormat::Depth24Stencil8))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "DrawMesh requires a camera and an RGBA8 framebuffer with depth");
		return;
	}
	if (subMeshIndex != std::numeric_limits<uint8_t>::max() && subMeshIndex >= mesh.GetSubMeshes().size())
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Submesh {} is out of range for {}", subMeshIndex, mesh.GetDebugName());
		return;
	}
	std::vector<glm::mat4> matrices {glm::mat4(1.0f), glm::mat4(1.0f)};
	uint32_t boneCount = 1;
	if (mesh.IsBoned())
	{
		if (desc.modelMatrices != nullptr && desc.matrixCount > 0)
		{
			matrices.resize(1);
			matrices.insert(matrices.end(), desc.modelMatrices, desc.modelMatrices + desc.matrixCount);
			boneCount = desc.matrixCount;
		}
		else if (!mesh.GetBoneMatrices().empty())
		{
			matrices.resize(1);
			matrices.insert(matrices.end(), mesh.GetBoneMatrices().begin(), mesh.GetBoneMatrices().end());
			boneCount = static_cast<uint32_t>(mesh.GetBoneMatrices().size());
		}
	}
	else if (desc.modelMatrices != nullptr && desc.matrixCount > 0)
	{
		matrices[0] = desc.modelMatrices[0];
	}
	const auto boxOffset = static_cast<uint32_t>(matrices.size());
	if (desc.boundingBox != nullptr)
	{
		const auto box = mesh.GetBoundingBox();
		matrices.push_back(matrices[0] * glm::scale(glm::translate(glm::mat4(1.0f), box.Center()), box.Size()));
	}
	const auto byteCount = static_cast<uint32_t>(matrices.size() * sizeof(glm::mat4));
	const SDL_GPUBufferCreateInfo bufferInfo {.usage = SDL_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ, .size = byteCount};
	const GpuBuffer buffer {SDL_CreateGPUBuffer(_gpuDevice.get(), &bufferInfo), {_gpuDevice.get()}};
	auto transfer = CreateTransferBuffer(_gpuDevice.get(), {reinterpret_cast<const uint8_t*>(matrices.data()), byteCount});
	if (!buffer || !transfer)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to upload mesh matrices: {}", SDL_GetError());
		return;
	}
	auto* commandBuffer = SDL_AcquireGPUCommandBuffer(_gpuDevice.get());
	if (commandBuffer == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to acquire mesh command buffer: {}", SDL_GetError());
		return;
	}
	auto* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (copyPass == nullptr)
	{
		SDL_CancelGPUCommandBuffer(commandBuffer);
		return;
	}
	const SDL_GPUTransferBufferLocation source {.transfer_buffer = transfer.get()};
	const SDL_GPUBufferRegion destination {.buffer = buffer.get(), .size = byteCount};
	SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);
	SDL_EndGPUCopyPass(copyPass);
	const SDL_GPUColorTargetInfo colorTarget {
	    .texture = toSdl(desc.frameBuffer->colorAttachment),
	    .clear_color = {0x27 / 255.0f, 0x46 / 255.0f, 0x59 / 255.0f, 1.0f},
	    .load_op = SDL_GPU_LOADOP_CLEAR,
	    .store_op = SDL_GPU_STOREOP_STORE,
	    .cycle = true,
	};
	const SDL_GPUDepthStencilTargetInfo depthTarget {
	    .texture = toSdl(desc.frameBuffer->depthStencilAttachment),
	    .clear_depth = 0.0f,
	    .load_op = SDL_GPU_LOADOP_CLEAR,
	    .store_op = SDL_GPU_STOREOP_DONT_CARE,
	    .stencil_load_op = SDL_GPU_LOADOP_CLEAR,
	    .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
	    .cycle = true,
	};
	auto* pass = SDL_BeginGPURenderPass(commandBuffer, &colorTarget, 1, &depthTarget);
	if (pass == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin mesh pass: {}", SDL_GetError());
		SDL_CancelGPUCommandBuffer(commandBuffer);
		return;
	}
	// Reflection materials already target the same RGBA8/depth formats as the viewer.
	const auto& pipelines = _pipelines[static_cast<size_t>(RenderPass::Reflection)];
	const auto viewProjection = desc.camera->GetViewProjectionMatrix(Camera::Projection::ReversedZ);
	const auto& island = Locator::terrainSystem::value();
	const auto extent = island.GetExtent();
	const MeshCamera data {
	    viewProjection, glm::vec4(extent.minimum, extent.maximum), {0, 1, boneCount, desc.morphWithTerrain ? 1u : 0u}};
	SDL_PushGPUVertexUniformData(commandBuffer, 0, &data, sizeof(data));
	auto* storage = buffer.get();
	SDL_BindGPUVertexStorageBuffers(pass, 0, &storage, 1);
	const auto& heightMap = island.GetHeightMap();
	const SDL_GPUTextureSamplerBinding heightBinding {toSdl(heightMap.handle), toSdl(heightMap.samplerHandle)};
	SDL_BindGPUVertexSamplers(pass, 0, &heightBinding, 1);
	DrawMeshPrimitives(commandBuffer, pass, mesh, pipelines.objects, 1, subMeshIndex, desc.drawAll);
	if (desc.boundingBox != nullptr)
	{
		struct LineData
		{
			glm::mat4 viewProjection;
			glm::uvec4 offsets;
		};
		const LineData lineData {viewProjection, {boxOffset, 0, 0, 0}};
		SDL_PushGPUVertexUniformData(commandBuffer, 0, &lineData, sizeof(lineData));
		DrawGeometry(pass, _pipelines[static_cast<size_t>(RenderPass::MeshViewer)].lines, *desc.boundingBox);
	}
	SDL_EndGPURenderPass(pass);
	if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to submit mesh pass: {}", SDL_GetError());
	}
}

SDL_GPUBuffer* RendererSdl3Gpu::UploadBuffer(const std::string& name, std::span<const uint8_t> memory,
                                             bool indexBuffer) const noexcept
{
	const SDL_GPUBufferCreateInfo info = {
	    .usage = indexBuffer ? SDL_GPU_BUFFERUSAGE_INDEX : SDL_GPU_BUFFERUSAGE_VERTEX,
	    .size = static_cast<uint32_t>(memory.size_bytes()),
	};
	GpuBuffer buffer {SDL_CreateGPUBuffer(_gpuDevice.get(), &info), {_gpuDevice.get()}};
	if (!buffer)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create buffer {}: {}", name, SDL_GetError());
		return nullptr;
	}
	SDL_SetGPUBufferName(_gpuDevice.get(), buffer.get(), name.c_str());
	// TODO(bwrsandman): Maintain a transfer buffer to batch uploads.
	auto transfer = CreateTransferBuffer(_gpuDevice.get(), memory);
	if (!transfer)
	{
		return nullptr;
	}
	auto* commandBuffer = SDL_AcquireGPUCommandBuffer(_gpuDevice.get());
	if (commandBuffer == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to acquire upload command buffer: {}", SDL_GetError());
		return nullptr;
	}
	auto* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (copyPass == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin buffer upload: {}", SDL_GetError());
		SDL_CancelGPUCommandBuffer(commandBuffer);
		return nullptr;
	}
	const SDL_GPUTransferBufferLocation location = {.transfer_buffer = transfer.get(), .offset = 0};
	const SDL_GPUBufferRegion region = {
	    .buffer = buffer.get(), .offset = 0, .size = static_cast<uint32_t>(memory.size_bytes())};
	SDL_UploadToGPUBuffer(copyPass, &location, &region, false);
	SDL_EndGPUCopyPass(copyPass);
	if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to submit buffer upload: {}", SDL_GetError());
		return nullptr;
	}
	return buffer.release();
}

VertexBufferUniquePtr RendererSdl3Gpu::CreateVertexBuffer(std::string name, std::span<const uint8_t> memory,
                                                          VertexDecl decl) noexcept
{
	// Preserve the engine's packed vertex ABI: byte attributes occupy four bytes,
	// and short attributes occupy four or eight bytes.
	static constexpr std::array<std::array<uint32_t, 4>, 3> k_Strides = {{{4, 4, 4, 4}, {4, 4, 8, 8}, {4, 8, 12, 16}}};
	uint32_t stride = 0;
	for (const auto& attrib : decl)
	{
		if (attrib.num < 1 || attrib.num > 4 || static_cast<size_t>(attrib.type) >= k_Strides.size())
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Invalid vertex declaration for {}", name);
			return nullptr;
		}
		stride += k_Strides.at(static_cast<size_t>(attrib.type)).at(attrib.num - 1);
	}
	if (stride == 0 || memory.empty() || memory.size_bytes() > std::numeric_limits<uint32_t>::max() ||
	    memory.size_bytes() % stride != 0)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Invalid vertex data size for {}", name);
		return nullptr;
	}
	auto* buffer = UploadBuffer(name, memory, false);
	if (buffer == nullptr)
	{
		return nullptr;
	}
	auto* result = new (std::nothrow) VertexBuffer {std::move(name), static_cast<uint32_t>(memory.size_bytes() / stride),
	                                                stride, fromSdl<VertexBufferHandle>(buffer)};
	if (result == nullptr)
	{
		SDL_ReleaseGPUBuffer(_gpuDevice.get(), buffer);
		return nullptr;
	}
	return {result, [device = _gpuDevice.get()](VertexBuffer* vertexBuffer) {
		        SDL_ReleaseGPUBuffer(device, toSdl(vertexBuffer->handle));
		        delete vertexBuffer;
	        }};
}

IndexBufferUniquePtr RendererSdl3Gpu::CreateIndexBuffer(std::string name, std::span<const uint8_t> memory,
                                                        IndexBuffer::Type type) noexcept
{
	if (memory.empty() || memory.size_bytes() > std::numeric_limits<uint32_t>::max() ||
	    memory.size_bytes() % IndexBuffer::GetTypeSize(type) != 0)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Invalid index data size for {}", name);
		return nullptr;
	}
	auto* buffer = UploadBuffer(name, memory, true);
	if (buffer == nullptr)
	{
		return nullptr;
	}
	auto* result = new (std::nothrow)
	    IndexBuffer {std::move(name), static_cast<uint32_t>(memory.size_bytes() / IndexBuffer::GetTypeSize(type)), type,
	                 fromSdl<IndexBufferHandle>(buffer)};
	if (result == nullptr)
	{
		SDL_ReleaseGPUBuffer(_gpuDevice.get(), buffer);
		return nullptr;
	}
	return {result, [device = _gpuDevice.get()](IndexBuffer* indexBuffer) {
		        SDL_ReleaseGPUBuffer(device, toSdl(indexBuffer->handle));
		        delete indexBuffer;
	        }};
}

Texture2dUniquePtr RendererSdl3Gpu::CreateTexture2d(std::string name, std::span<const uint8_t> memory, glm::u16vec2 resolution,
                                                    uint16_t layers, TextureFormat format, Wrapping wrapping,
                                                    Filter filter) noexcept
{
	if (resolution.x == 0 || resolution.y == 0 || layers == 0)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Invalid texture dimensions for {}", name);
		return nullptr;
	}
	const auto sdlFormat = toSdl(format);
	const auto bytesPerLayer = SDL_CalculateGPUTextureFormatSize(sdlFormat, resolution.x, resolution.y, 1);
	const auto uploadSize = static_cast<uint64_t>(bytesPerLayer) * layers;
	if (bytesPerLayer == 0 || memory.size_bytes() < uploadSize || uploadSize > std::numeric_limits<uint32_t>::max())
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Invalid texture data size for {}", name);
		return nullptr;
	}
	// DDS inputs may contain trailing mip data; the current texture has only its base level.
	memory = memory.first(static_cast<size_t>(uploadSize));
	const SDL_GPUTextureCreateInfo info = {
	    .type = layers == 1 ? SDL_GPU_TEXTURETYPE_2D : SDL_GPU_TEXTURETYPE_2D_ARRAY,
	    .format = sdlFormat,
	    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
	    .width = resolution.x,
	    .height = resolution.y,
	    .layer_count_or_depth = layers,
	    .num_levels = 1,
	    .sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	GpuTexture texture {SDL_CreateGPUTexture(_gpuDevice.get(), &info), {_gpuDevice.get()}};
	if (!texture)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create texture {}: {}", name, SDL_GetError());
		return nullptr;
	}
	const SDL_GPUSamplerCreateInfo samplerInfo = {
	    .min_filter = toSdl(filter),
	    .mag_filter = toSdl(filter),
	    .address_mode_u = toSdl(wrapping),
	    .address_mode_v = toSdl(wrapping),
	    .address_mode_w = toSdl(wrapping),
	};
	GpuSampler sampler {SDL_CreateGPUSampler(_gpuDevice.get(), &samplerInfo), {_gpuDevice.get()}};
	if (!sampler)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create sampler for {}: {}", name, SDL_GetError());
		return nullptr;
	}
	SDL_SetGPUTextureName(_gpuDevice.get(), texture.get(), name.c_str());
	auto transfer = CreateTransferBuffer(_gpuDevice.get(), memory);
	if (!transfer)
	{
		return nullptr;
	}
	auto* commandBuffer = SDL_AcquireGPUCommandBuffer(_gpuDevice.get());
	if (commandBuffer == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to acquire texture upload command buffer: {}", SDL_GetError());
		return nullptr;
	}
	auto* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
	if (copyPass == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin texture upload: {}", SDL_GetError());
		SDL_CancelGPUCommandBuffer(commandBuffer);
		return nullptr;
	}
	SDL_GPUTextureTransferInfo location = {.transfer_buffer = transfer.get(), .offset = 0};
	SDL_GPUTextureRegion region = {.texture = texture.get(),
	                               .mip_level = 0,
	                               .layer = 0,
	                               .x = 0,
	                               .y = 0,
	                               .z = 0,
	                               .w = resolution.x,
	                               .h = resolution.y,
	                               .d = 1};
	for (uint16_t i = 0; i < layers; ++i)
	{
		location.offset = i * bytesPerLayer;
		region.layer = i;
		SDL_UploadToGPUTexture(copyPass, &location, &region, false);
	}
	SDL_EndGPUCopyPass(copyPass);
	if (!SDL_SubmitGPUCommandBuffer(commandBuffer))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to submit texture upload: {}", SDL_GetError());
		return nullptr;
	}
	auto* result = new (std::nothrow)
	    Texture2d {std::move(name), fromSdl(texture.get()), fromSdl(sampler.get()), resolution, layers, format};
	if (result == nullptr)
	{
		return nullptr;
	}
	std::ignore = texture.release();
	std::ignore = sampler.release();
	return {result, [device = _gpuDevice.get()](Texture2d* texture2d) {
		        SDL_ReleaseGPUSampler(device, toSdl(texture2d->samplerHandle));
		        SDL_ReleaseGPUTexture(device, toSdl(texture2d->handle));
		        delete texture2d;
	        }};
}

std::unique_ptr<FrameBuffer> RendererSdl3Gpu::CreateFrameBuffer(std::string name, glm::u16vec2 resolution,
                                                                TextureFormat colorFormat,
                                                                std::optional<TextureFormat> depthStencilFormat) noexcept
{
	if (resolution.x == 0 || resolution.y == 0)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Invalid render target dimensions for {}", name);
		return nullptr;
	}
	const SDL_GPUTextureCreateInfo info = {
	    .type = SDL_GPU_TEXTURETYPE_2D,
	    .format = toSdl(colorFormat),
	    .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
	    .width = resolution.x,
	    .height = resolution.y,
	    .layer_count_or_depth = 1,
	    .num_levels = 1,
	    .sample_count = SDL_GPU_SAMPLECOUNT_1,
	};
	GpuTexture texture {SDL_CreateGPUTexture(_gpuDevice.get(), &info), {_gpuDevice.get()}};
	if (!texture)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create render target {}: {}", name, SDL_GetError());
		return nullptr;
	}
	GpuTexture depthStencil {nullptr, {_gpuDevice.get()}};
	if (depthStencilFormat)
	{
		const auto format = GetSupportedDepthStencilFormat(_gpuDevice.get(), *depthStencilFormat);
		if (!SDL_GPUTextureSupportsFormat(_gpuDevice.get(), format, SDL_GPU_TEXTURETYPE_2D,
		                                  SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Unsupported depth/stencil format for {}", name);
			return nullptr;
		}
		auto depthInfo = info;
		depthInfo.format = format;
		depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
		depthStencil.reset(SDL_CreateGPUTexture(_gpuDevice.get(), &depthInfo));
		if (!depthStencil)
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create depth/stencil target {}: {}", name, SDL_GetError());
			return nullptr;
		}
		depthStencilFormat = fromSdl(format);
		SDL_SetGPUTextureName(_gpuDevice.get(), depthStencil.get(), (name + "_depthStencil").c_str());
	}
	const SDL_GPUSamplerCreateInfo samplerInfo = {
	    .min_filter = SDL_GPU_FILTER_LINEAR,
	    .mag_filter = SDL_GPU_FILTER_LINEAR,
	    .address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	    .address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	    .address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	};
	GpuSampler sampler {SDL_CreateGPUSampler(_gpuDevice.get(), &samplerInfo), {_gpuDevice.get()}};
	if (!sampler)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create render target sampler {}: {}", name, SDL_GetError());
		return nullptr;
	}
	SDL_SetGPUTextureName(_gpuDevice.get(), texture.get(), name.c_str());
	auto* color = texture.release();
	auto* colorSampler = sampler.release();
	auto* depth = depthStencil.release();
	return std::make_unique<FrameBuffer>(std::move(name), resolution, colorFormat, depthStencilFormat, fromSdl(color),
	                                     fromSdl(colorSampler), fromSdl(depth),
	                                     [device = _gpuDevice.get(), color, colorSampler, depth] {
		                                     SDL_ReleaseGPUSampler(device, colorSampler);
		                                     SDL_ReleaseGPUTexture(device, color);
		                                     if (depth != nullptr)
		                                     {
			                                     SDL_ReleaseGPUTexture(device, depth);
		                                     }
	                                     });
}
