/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "Graphics/RendererInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

struct SDL_GPUDevice;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPURenderPass;
struct SDL_GPUTexture;
struct SDL_GPUBuffer;
struct SDL_Window;

namespace openblack::graphics
{
class L3DSubMesh;

class RendererSdl3Gpu final: public RendererInterface
{
public:
	struct Pipelines
	{
		SDL_GPUGraphicsPipeline* sky;
	};

	static std::unique_ptr<RendererInterface> Create(bool vsync) noexcept;

	~RendererSdl3Gpu() noexcept final;

	[[nodiscard]] void* GetDevice() const noexcept final { return _gpuDevice.get(); }

	void ConfigureView(RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept final;

	void Reset(glm::u16vec2 resolution) const noexcept final;
	void DrawScene(const DrawSceneDesc& drawDesc) noexcept final;
	void Frame() noexcept final;
	void RequestScreenshot(const std::filesystem::path& filepath) noexcept final;
	[[nodiscard]] bool GetDebug() const noexcept final;
	void SetDebug(bool value) noexcept final;
	[[nodiscard]] bool GetProfile() const noexcept final;
	void SetProfile(bool value) noexcept final;

	void DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept final;
	[[nodiscard]] ShaderManager& GetShaderManager() const noexcept final;

	[[nodiscard]] VertexBufferUniquePtr CreateVertexBuffer(std::string name, const void* memory,
	                                                       VertexDecl decl) noexcept final;
	[[nodiscard]] VertexBufferUniquePtr CreateVertexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                       VertexDecl decl) noexcept final;
	[[nodiscard]] IndexBufferUniquePtr CreateIndexBuffer(std::string name, const void* memory,
	                                                     IndexBuffer::Type type) noexcept final;
	[[nodiscard]] IndexBufferUniquePtr CreateIndexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                     IndexBuffer::Type type) noexcept final;
	[[nodiscard]] Texture2dUniquePtr CreateTexture2d(std::string name, const void* memory, glm::u16vec2 resolution,
	                                                 uint16_t layers, TextureFormat format, Wrapping wrapping,
	                                                 Filter filter) noexcept final;
	[[nodiscard]] Texture2dUniquePtr CreateTexture2d(std::string name, std::span<const uint8_t> memory, glm::u16vec2 resolution,
	                                                 uint16_t layers, TextureFormat format, Wrapping wrapping,
	                                                 Filter filter) noexcept final;

	void Bind(const VertexBuffer& buffer) const noexcept final;
	void Bind(const IndexBuffer& buffer, uint32_t count, uint32_t startIndex) const noexcept final;

private:
	struct SdlGpuDeleter
	{
		SDL_Window* window = nullptr;
		void operator()(SDL_GPUDevice* device) const noexcept;
	};

	[[nodiscard]] SDL_GPUBuffer* UploadBuffer(const std::string& name, std::span<const uint8_t> memory,
	                                          bool indexBuffer) const noexcept;
	void DrawSubMesh(SDL_GPURenderPass* renderPass, SDL_GPUGraphicsPipeline* pipeline, const L3DSubMesh& subMesh) const;

	RendererSdl3Gpu(std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter>&& device, Pipelines&& pipelines) noexcept;
	std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter> _gpuDevice;
	Pipelines _pipelines;
};
} // namespace openblack::graphics
