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

#include "InstanceDesc.h"
#include "RenderPass.h"

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
		/// The hand and its shadow, hidden while a dialog shows its own pointer
		bool drawHand = true;
	};

	struct L3DMeshSubmitDesc
	{
		graphics::RenderPass viewId;
		const graphics::ShaderProgram* program;
		/// Draws the submeshes that have a lightmap, with it bound to s_lightmap, when set
		const graphics::ShaderProgram* lightmapProgram;
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
		/// Blend and write depth as each primitive's material says, instead of drawing it opaque
		bool useMaterialBlending;
		/// The table of joints the submeshes with joints turn by, about their pivots (the temple's doors)
		std::span<const glm::mat4> joints;
		/// Draws only the submeshes with joints
		bool onlyJoints;
		/// Leaves out the submeshes with joints while their joints don't turn them
		bool hideShutJoints;
		/// Culls the back faces of the primitives whose materials aren't two-sided, which the mirrored reflection pass
		/// sees from the other side
		bool useMaterialCulling;
		bool mirrored;
		/// The fraction of its depth the mesh is pushed back by
		float depthBias;
		/// How far the mesh's texture has slid across it
		glm::vec2 uvOffset;
		/// Textures some of the submeshes are drawn with in place of their skins, by submesh
		std::span<const std::pair<uint32_t, TextureHandle>> subMeshTextures;
		/// Colours added to some of the submeshes, by submesh, after everything else
		std::span<const std::pair<uint32_t, glm::vec3>> subMeshGlows;
	};

	static std::unique_ptr<RendererInterface> Create(GraphicsBackend backend, bool vsync) noexcept;

	virtual ~RendererInterface() noexcept = default;

	virtual void ConfigureView(graphics::RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept = 0;
	virtual void Reset(glm::u16vec2 resolution) const noexcept = 0;
	virtual void DrawScene(const DrawSceneDesc& drawDesc) const noexcept = 0;
	virtual void Frame() noexcept = 0;
	virtual void RequestScreenshot(const std::filesystem::path& filepath) noexcept = 0;
	[[nodiscard]] virtual bool GetDebug() const noexcept = 0;
	virtual void SetDebug(bool value) noexcept = 0;
	[[nodiscard]] virtual bool GetProfile() const noexcept = 0;
	virtual void SetProfile(bool value) noexcept = 0;

	// TODO: Remove this function. All renderables should be drawn through RenderingSystem with Components
	virtual void DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept = 0;
	// TODO: Should shader manager be available through Locator as a service?
	[[nodiscard]] virtual graphics::ShaderManager& GetShaderManager() const noexcept = 0;
};

} // namespace openblack::graphics
