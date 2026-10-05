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
		/// Where the mesh comes in a pass sorted by depth, the greatest first
		uint32_t sortDepth;
		/// How far the mesh's texture has slid across it
		glm::vec2 uvOffset;
		/// A texture every primitive with a skin is drawn with in place of it, when set, as the game gives the creature
		/// room's icons
		const TextureHandle* skinTexture;
		/// The environment map added to the mesh where its program takes one (s_environment)
		const Texture2D* environment;
		/// Textures some of the submeshes are drawn with in place of their skins, by submesh
		std::span<const std::pair<uint32_t, TextureHandle>> subMeshTextures;
		/// Colours added to some of the submeshes, by submesh, after everything else
		std::span<const std::pair<uint32_t, glm::vec3>> subMeshGlows;
		/// Submeshes left undrawn
		std::span<const uint32_t> hiddenSubMeshes;
		/// A colour the mesh is drawn in, where its program takes one: lit when w is 0, otherwise unlit with its alpha by w
		glm::vec4 tint {1.0f, 1.0f, 1.0f, 0.0f};
		/// The temple's light, which the lightmapped submeshes are multiplied by, and which is added to every submesh
		/// that doesn't glow, as the game adds the vertices' specular
		glm::vec3 lightMultiply {1.0f};
		/// How much brighter than the land's light where it stands the mesh is, at most white: the god hand is half as
		/// bright again
		float landLightScale {1.0f};
		/// The mesh isn't shaded by the sun, only coloured by the land's light where it stands
		bool unlit {false};
		glm::vec3 lightAdd {0.0f};
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
