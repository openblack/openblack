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

#include <array>
#include <limits>
#include <new>
#include <tuple>
#include <utility>
#include <vector>

#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_stdinc.h>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "Debug/DebugGuiInterface.h"
#include "GraphicsHandleSdl3Gpu.h"
#include "IndexBuffer.h"
#include "Locator.h"
#include "Mesh.h"
#include "Profiler.h"
#include "Texture2d.h"
#include "VertexBuffer.h"
#include "generated/shaders/sky.slang.spv.h"

using namespace openblack::graphics;

namespace
{
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
                                        const SDL_GPUVertexInputState& vertexInputState)
{

	const SDL_GPUColorTargetDescription colorTargetDescriptions = {
	    .format = targetFormat,
	    .blend_state =
	        {
	            .enable_blend = false,
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
	    .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
	    .rasterizer_state =
	        {
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
	            .enable_depth_write = false,
	        },
	    .target_info =
	        {
	            .color_target_descriptions = &colorTargetDescriptions,
	            .num_color_targets = 1,
	            .has_depth_stencil_target = false,
	        },
	};
	auto* result = SDL_CreateGPUGraphicsPipeline(device, &createInfo);
	if (result == nullptr)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to create graphics pipeline: {}", SDL_GetError());
	}
	return result;
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

	Pipelines pipelines = {
	    .sky = CreatePipeline(gpuDevice.get(), SDL_GetGPUSwapchainTextureFormat(gpuDevice.get(), sdlWindow),
	                          k_SkyVertexShaderCreateInfo, k_SkyFragmentShaderCreateInfo, k_SkyVertexInputStateByVertex),
	};
	if (!pipelines.sky)
	{
		return nullptr;
	}

	return std::unique_ptr<RendererInterface> {new RendererSdl3Gpu(std::move(gpuDevice), std::move(pipelines))};
}

RendererSdl3Gpu::RendererSdl3Gpu(std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter>&& device, Pipelines&& pipelines) noexcept
    : _gpuDevice(std::move(device))
    , _pipelines(pipelines)
{
}

RendererSdl3Gpu::~RendererSdl3Gpu() noexcept
{
	SDL_ReleaseGPUGraphicsPipeline(_gpuDevice.get(), _pipelines.sky);
}

void RendererSdl3Gpu::ConfigureView(RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept
{
	// TODO
}

void RendererSdl3Gpu::Reset(glm::u16vec2 resolution) const noexcept
{
	// TODO
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
	if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmdBufferMain, reinterpret_cast<SDL_Window*>(window.GetHandle()),
	                                           &swapchainTexture, nullptr, nullptr))
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
	SDL_GPUColorTargetInfo colorTargetMain = {.texture = swapchainTexture,
	                                          .clear_color = {.r = 0x27 / static_cast<float>(0xff),
	                                                          .g = 0x46 / static_cast<float>(0xff),
	                                                          .b = 0x59 / static_cast<float>(0xff),
	                                                          .a = 1.0f},
	                                          .load_op = SDL_GPU_LOADOP_CLEAR,
	                                          .store_op = SDL_GPU_STOREOP_STORE,
	                                          .cycle = false};

	auto* renderPassMain = SDL_BeginGPURenderPass(cmdBufferMain, &colorTargetMain, 1, nullptr);
	if (!renderPassMain)
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to begin render pass: {}", SDL_GetError());
		SDL_SubmitGPUCommandBuffer(cmdBufferMain);
		return;
	}

	const auto& camera = *drawDesc.camera;
	auto view = camera.GetViewMatrix(Camera::Interpolation::Current);
	auto proj = camera.GetProjectionMatrix();
	auto viewProj = proj * view;

	{
		auto section = profiler.BeginScoped(Profiler::Stage::MainPassDrawSky);
		if (drawDesc.drawSky)
		{
			const auto& sky = Locator::skySystem::value();
			const auto skyType = sky.GetCurrentSkyType();
			struct Sky
			{
				float timeOfDay;
				float alignment;
			};
			Sky skyData = {
			    .timeOfDay = skyType,
			    .alignment = Locator::config::value().skyAlignment + 1.0f,
			};

			auto& skyMesh = Locator::skySystem::value().GetMesh();
			const auto& subMesh = *skyMesh.GetSubMeshes()[0].get();

			{
				auto& skyTexture = sky.GetTexture();
				SDL_GPUTextureSamplerBinding bindings = {
				    .texture = toSdl(skyTexture.handle),
				    .sampler = toSdl(skyTexture.samplerHandle),
				};
				SDL_BindGPUFragmentSamplers(renderPassMain, 0, &bindings, 1);
			}
			SDL_PushGPUVertexUniformData(cmdBufferMain, 0, &viewProj, sizeof(viewProj));
			SDL_PushGPUFragmentUniformData(cmdBufferMain, 0, &skyData, sizeof(skyData));
			DrawSubMesh(renderPassMain, _pipelines.sky, subMesh);
		}
	}

	{
		auto section = profiler.BeginScoped(Profiler::Stage::ReflectionDrawWater);
		if (drawDesc.drawWater)
		{
			assert(false);
		}
	}

	{
		auto section = profiler.BeginScoped(Profiler::Stage::MainPassDrawIsland);
		if (drawDesc.drawIsland)
		{
			assert(false);
		}
	}

	{
		auto section = profiler.BeginScoped(Profiler::Stage::MainPassDrawModels);
		if (drawDesc.drawEntities)
		{
			assert(false);
		}
	}

	SDL_EndGPURenderPass(renderPassMain);

	{
		auto section = profiler.BeginScoped(Profiler::Stage::GuiDraw);
		// TODO:
		// const bool screenshotThisFrame = _requestScreenshot.has_value() && _requestScreenshot->first == _frameCount;
		// // Skip drawing Debug UI for screenshots
		// if (screenshotThisFrame)
		// {
		// 	SPDLOG_LOGGER_INFO(spdlog::get("game"), "Requesting a screenshot at frame {}...", _frameCount);
		// 	Locator::rendererInterface::value().RequestScreenshot(_requestScreenshot->second);
		// }
		// else
		{
			Locator::debugGui::value().Draw(cmdBufferMain, swapchainTexture);
		}
	}

	if (!SDL_SubmitGPUCommandBuffer(cmdBufferMain))
	{
		SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to submit frame: {}", SDL_GetError());
	}
}

void RendererSdl3Gpu::DrawSubMesh(SDL_GPURenderPass* renderPass, SDL_GPUGraphicsPipeline* pipeline,
                                  const L3DSubMesh& subMesh) const
{
	const uint32_t numInstances = 1;
	auto& mesh = subMesh.GetMesh();
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
		SDL_DrawGPUIndexedPrimitives(renderPass, mesh.GetIndexBuffer().count, numInstances, 0, 0, 0);
	}
	else
	{
		SDL_DrawGPUPrimitives(renderPass, mesh.GetVertexBuffer().vertexCount, numInstances, 0, 0);
	}
}

void RendererSdl3Gpu::Frame() noexcept
{
	// DrawScene submits and waits for an available swapchain image.
}

void RendererSdl3Gpu::RequestScreenshot(const std::filesystem::path& filepath) noexcept
{
	// TODO
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

void RendererSdl3Gpu::DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept
{
	// TODO
}

[[nodiscard]] ShaderManager& RendererSdl3Gpu::GetShaderManager() const noexcept
{
	SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "The bgfx shader manager is unavailable in the SDL GPU renderer");
	std::abort();
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

VertexBufferUniquePtr RendererSdl3Gpu::CreateVertexBuffer(std::string name, const void*, VertexDecl) noexcept
{
	SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Buffer {} requires a byte span for SDL GPU uploads", name);
	return nullptr;
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
	return {new VertexBuffer {std::move(name),
	                          static_cast<uint32_t>(memory.size_bytes() / stride),
	                          stride,
	                          fromSdl<VertexBufferHandle>(buffer),
	                          {}},
	        [device = _gpuDevice.get()](VertexBuffer* vertexBuffer) {
		        SDL_ReleaseGPUBuffer(device, toSdl(vertexBuffer->handle));
		        delete vertexBuffer;
	        }};
}

IndexBufferUniquePtr RendererSdl3Gpu::CreateIndexBuffer(std::string name, const void*, IndexBuffer::Type) noexcept
{
	SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Buffer {} requires a byte span for SDL GPU uploads", name);
	return nullptr;
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

Texture2dUniquePtr RendererSdl3Gpu::CreateTexture2d(std::string name, const void*, glm::u16vec2, uint16_t, TextureFormat,
                                                    Wrapping, Filter) noexcept
{
	SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Texture {} requires a byte span for SDL GPU uploads", name);
	return nullptr;
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

void RendererSdl3Gpu::Bind(const VertexBuffer& buffer) const noexcept
{
	// TODO
}

void RendererSdl3Gpu::Bind(const IndexBuffer& buffer, uint32_t count, uint32_t startIndex) const noexcept
{
	// TODO
}
