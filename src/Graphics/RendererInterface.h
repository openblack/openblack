/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>
#include <memory>
#include <span>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "IndexBuffer.h"
#include "InstanceDesc.h"
#include "RenderPass.h"
#include "Texture2d.h"
#include "VertexBuffer.h"

#include "../EngineConfig.h"

namespace openblack
{
class Camera;
class Profiler;
class Sky;
class Ocean;
} // namespace openblack

namespace openblack::ecs
{
class Registry;
}

namespace openblack::graphics
{
class L3DMesh;
class FrameBuffer;
class ShaderManager;
class ShaderProgram;

class RendererInterface
{
public:
	struct DrawSceneDesc
	{
		const Camera* camera;
		const graphics::FrameBuffer* frameBuffer;
		const ecs::Registry& entities;
		uint32_t time;
		float timeOfDay;
		float bumpMapStrength;
		float smallBumpMapStrength;
		graphics::RenderPass viewId;
		bool drawSky;
		bool drawWater;
		bool drawIsland;
		bool drawEntities;
		bool drawSprites;
		bool drawVegetation;
		bool drawBoundingBoxes;
		bool cullBack;
		bool wireframe;
	};

	struct L3DMeshSubmitDesc
	{
		graphics::RenderPass viewId;
		const graphics::ShaderProgram* program;
		uint64_t state;
		uint32_t rgba;
		const glm::mat4* modelMatrices;
		uint8_t matrixCount;
		std::unique_ptr<const graphics::InstanceDesc> instanceDesc;
		uint32_t instanceStart;
		uint32_t instanceCount;
		bool isSky;
		bool drawAll; ///< For use in the mesh viewer
		bool morphWithTerrain;
	};

	static std::unique_ptr<RendererInterface> Create(GraphicsBackend backend, bool vsync) noexcept;

	virtual ~RendererInterface() noexcept = default;

	[[nodiscard]] virtual void* GetDevice() const noexcept = 0;
	virtual void ConfigureView(graphics::RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept = 0;
	virtual void Reset(glm::u16vec2 resolution) const noexcept = 0;
	virtual void DrawScene(const DrawSceneDesc& drawDesc) noexcept = 0;
	virtual void Frame() noexcept = 0;
	virtual void RequestScreenshot(const std::filesystem::path& filepath) noexcept = 0;
	[[nodiscard]] virtual bool GetDebug() const noexcept = 0;
	virtual void SetDebug(bool value) noexcept = 0;
	[[nodiscard]] virtual bool GetProfile() const noexcept = 0;
	virtual void SetProfile(bool value) noexcept = 0;

	// TODO: Remove this function. All renderables should be drawn through RenderingSystem with Components
	virtual void DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept = 0;
	// TODO: Should shader manager be available through Locator as a service?
	[[nodiscard]] virtual ShaderManager& GetShaderManager() const noexcept = 0;

	[[nodiscard]] virtual VertexBufferUniquePtr CreateVertexBuffer(std::string name, const void* memory,
	                                                               VertexDecl decl) noexcept = 0;
	[[nodiscard]] virtual VertexBufferUniquePtr CreateVertexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                               VertexDecl decl) noexcept = 0;
	[[nodiscard]] virtual IndexBufferUniquePtr CreateIndexBuffer(std::string name, const void* memory,
	                                                             IndexBuffer::Type type) noexcept = 0;
	[[nodiscard]] virtual IndexBufferUniquePtr CreateIndexBuffer(std::string name, std::span<const uint8_t> memory,
	                                                             IndexBuffer::Type type) noexcept = 0;
	[[nodiscard]] virtual Texture2dUniquePtr CreateTexture2d(std::string name, const void* memory, glm::u16vec2 resolution,
	                                                         uint16_t layers, TextureFormat format, Wrapping wrapping,
	                                                         Filter filter) noexcept = 0;
	[[nodiscard]] virtual Texture2dUniquePtr CreateTexture2d(std::string name, std::span<const uint8_t> memory, glm::u16vec2 resolution,
	                                                         uint16_t layers, TextureFormat format, Wrapping wrapping,
	                                                         Filter filter) noexcept = 0;

	virtual void Bind(const VertexBuffer& buffer) const noexcept = 0;
	virtual void Bind(const IndexBuffer& buffer, uint32_t count, uint32_t startIndex) const noexcept = 0;
};

} // namespace openblack::graphics
