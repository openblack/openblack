/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <map>

#include <entt/fwd.hpp>

#include "Graphics/RendererInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

struct SDL_GPUDevice;
struct SDL_GPUCommandBuffer;
struct SDL_GPUGraphicsPipeline;
struct SDL_GPUSampler;
struct SDL_GPURenderPass;
struct SDL_GPUTexture;
struct SDL_GPUBuffer;
struct SDL_Window;

namespace openblack::graphics
{
class L3DSubMesh;
class Mesh;

class RendererSdl3Gpu final: public RendererInterface
{
public:
	struct Pipelines
	{
		SDL_GPUGraphicsPipeline* sky;
		SDL_GPUGraphicsPipeline* water;
		SDL_GPUGraphicsPipeline* terrain;
		SDL_GPUGraphicsPipeline* sprites;
		SDL_GPUGraphicsPipeline* footprints;
		SDL_GPUGraphicsPipeline* lines;
		std::array<SDL_GPUGraphicsPipeline*, 6> objects {}; // blend mode and depth writes
		std::array<SDL_GPUGraphicsPipeline*, 6> vegetation {};

		void Release(SDL_GPUDevice* device) noexcept;
	};

	static std::unique_ptr<RendererInterface> Create(bool vsync) noexcept;

	~RendererSdl3Gpu() noexcept final;

	[[nodiscard]] void* GetDevice() const noexcept final { return _gpuDevice.get(); }

	void DrawScene(const DrawSceneDesc& drawDesc) noexcept final;
	void Frame() noexcept final;
	void RequestScreenshot(const std::filesystem::path& filepath) noexcept final;
	[[nodiscard]] bool GetDebug() const noexcept final;
	void SetDebug(bool value) noexcept final;
	[[nodiscard]] bool GetProfile() const noexcept final;
	void SetProfile(bool value) noexcept final;

	void DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept final;

	[[nodiscard]] VertexBufferUniquePtr CreateVertexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                       VertexDecl decl) noexcept final;
	[[nodiscard]] IndexBufferUniquePtr CreateIndexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                     IndexBuffer::Type type) noexcept final;
	[[nodiscard]] Texture2dUniquePtr CreateTexture2d(std::string name, std::span<const uint8_t> memory, glm::u16vec2 resolution,
	                                                 uint16_t layers, TextureFormat format, Wrapping wrapping,
	                                                 Filter filter) noexcept final;

	[[nodiscard]] std::unique_ptr<FrameBuffer>
	CreateFrameBuffer(std::string name, glm::u16vec2 resolution, TextureFormat colorFormat,
	                  std::optional<TextureFormat> depthStencilFormat) noexcept final;

private:
	struct SdlGpuDeleter
	{
		SDL_Window* window = nullptr;
		void operator()(SDL_GPUDevice* device) const noexcept;
	};

	[[nodiscard]] SDL_GPUBuffer* UploadBuffer(const std::string& name, std::span<const uint8_t> memory,
	                                          bool indexBuffer) const noexcept;
	[[nodiscard]] bool ResizeDepthStencil(glm::uvec2 resolution) noexcept;
	void DrawGeometry(SDL_GPURenderPass* renderPass, SDL_GPUGraphicsPipeline* pipeline, const Mesh& mesh,
	                  uint32_t instanceCount = 1) const;
	void DrawMeshPrimitives(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, const L3DMesh& mesh,
	                        const std::array<SDL_GPUGraphicsPipeline*, 6>& pipelines, uint32_t instanceCount,
	                        uint8_t subMeshIndex, bool drawAll) const;
	[[nodiscard]] bool UploadSceneMatrices(SDL_GPUCommandBuffer* commandBuffer) noexcept;
	void DrawSky(SDL_GPUCommandBuffer* commandBuffer, SDL_GPURenderPass* renderPass, SDL_GPUGraphicsPipeline* pipeline,
	             const Camera& camera) const;
	[[nodiscard]] bool DrawPass(const DrawSceneDesc& desc, SDL_GPUCommandBuffer* commandBuffer, SDL_GPUTexture* defaultTarget,
	                            SDL_GPUTexture* defaultDepthStencil = nullptr) const noexcept;

	RendererSdl3Gpu(std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter>&& device,
	                const std::array<Pipelines, static_cast<size_t>(RenderPass::_count)>& pipelines) noexcept;
	std::unique_ptr<SDL_GPUDevice, SdlGpuDeleter> _gpuDevice;
	std::array<Pipelines, static_cast<size_t>(RenderPass::_count)> _pipelines;
	std::optional<std::filesystem::path> _screenshotPath;
	SDL_GPUBuffer* _sceneMatrices {};
	std::unique_ptr<Mesh> _spriteQuad;
	uint32_t _sceneMatrixCapacity {};
	uint32_t _treeMatrixOffset {};
	uint32_t _treeSwayOffset {};
	uint32_t _identityMatrixOffset {};
	std::map<entt::id_type, uint32_t> _entityBoneOffsets;
	Texture2dUniquePtr _whiteTexture;
	SDL_GPUTexture* _depthStencil {};
	glm::uvec2 _depthStencilResolution {};
};
} // namespace openblack::graphics
