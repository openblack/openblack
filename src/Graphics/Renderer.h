/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

#include <SDL.h>
#include <entt/core/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "3D/SkyDome.h"
#include "Graphics/HandShadow.h"
#include "Graphics/RenderPass.h"
#include "Graphics/RendererInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
class Camera;
class LandLightTable;
struct BgfxCallback;
class Game;

namespace ecs
{
class Registry;
}

namespace graphics
{
class FrameBuffer;
class L3DSubMesh;
class ShaderProgram;
class Mesh;

class Renderer final: public RendererInterface
{
public:
	Renderer(uint32_t bgfxReset, std::unique_ptr<BgfxCallback>&& bgfxCallback) noexcept;
	~Renderer() noexcept final;

	[[nodiscard]] ShaderManager& GetShaderManager() const noexcept final;

	void ConfigureView(RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept final;

	void DrawScene(const DrawSceneDesc& drawDesc) const noexcept final;
	void DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept final;
	void Frame() noexcept final;
	void RequestScreenshot(const std::filesystem::path& filepath) noexcept final;
	[[nodiscard]] bool GetDebug() const noexcept final { return _bgfxDebug; }
	void SetDebug(bool value) noexcept final { _bgfxDebug = value; }
	[[nodiscard]] bool GetProfile() const noexcept final { return _bgfxProfile; }
	void SetProfile(bool value) noexcept final { _bgfxProfile = value; }

	void Reset(glm::u16vec2 resolution) const noexcept final;

private:
	void DrawFootprintPass(const DrawSceneDesc& drawDesc) const;
	/// The rivers' beds or channels, a footprint for each stretch of river
	void DrawStreamFootprints(graphics::RenderPass viewId, entt::id_type meshId) const;
	void DrawLandAlphaPass(const DrawSceneDesc& drawDesc) const;
	/// The land's luminosity this frame: as it was laid, shaded by the clouds and lit by the lights at night
	void DrawLandLuminosityPass(const DrawSceneDesc& drawDesc) const;
	[[nodiscard]] const Texture2D& GetLandLuminosity() const;
	/// The land cells' colours this frame: as they were laid, with the frame's colour stamps (see land_colour_stamps)
	void DrawLandColourPass(const DrawSceneDesc& drawDesc) const;
	/// The sky's dome blended again where the sky has moved on, a band of rows a frame
	void DrawSkyDomePass(const DrawSceneDesc& drawDesc) const;
	[[nodiscard]] const Texture2D& GetLandColour() const;
	/// The sun in the sky, after the sky's dome
	void DrawSun(RenderPass viewId) const;
	/// The sun's glare over the finished view, dimmed by what hides the sun from the camera
	void DrawSunGlare(const Camera& camera) const;
	/// The puffs of mist, blended over the scene, the farthest first
	void DrawMists(const DrawSceneDesc& desc) const;
	/// The moon and its glow in the sky, after the sky's dome
	void DrawMoon(RenderPass viewId) const;
	/// A mesh of the sky's, in a colour, with a texture and that texture's alpha
	struct CelestialDraw
	{
		entt::id_type meshId;
		entt::id_type textureId;
		entt::id_type alphaTextureId;
		glm::mat4 model;
		glm::vec4 colour;
		/// See vs_celestial
		glm::vec4 celestial;
		uint64_t state;
	};
	void DrawCelestialMesh(RenderPass viewId, const CelestialDraw& draw) const;
	/// The hand's glow on the water at night, under the sea
	void DrawHandWaterGlow(const DrawSceneDesc& desc) const;
	/// The sea's rows, ripple, period and colour for the camera
	void SetSeaUniforms(const ShaderProgram& waterShader, const Camera& camera) const;
	/// Casts the shadows of the island's trees, rocks and buildings onto it (see ObjectShadows)
	void DrawObjectShadowPass(const DrawSceneDesc& drawDesc) const;
	/// Draws the hand's silhouette for its shadow, or clears the shadow when there is none
	void DrawHandShadowPass(const DrawSceneDesc& drawDesc) const;
	/// Draws a submesh, with a texture in place of its skins when given one
	void DrawSubMesh(const L3DMesh& mesh, const L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc, bool preserveState,
	                 const TextureHandle* texture = nullptr, glm::vec3 glow = glm::vec3(0.0f)) const;
	void DrawPass(const DrawSceneDesc& desc) const;
	/// The beams of the temple's spot lights and the light its windows shed, which the game draws in its
	/// rooms but not in the reflection of the main room
	void DrawLightBeams(const DrawSceneDesc& desc) const;
	/// The temple's domes of mist, which the game draws facing the camera with a frame of the smoke texture
	void DrawMistDomes(const DrawSceneDesc& desc) const;
	/// The text the temple's rooms write in the world this frame: the signs' labels and the scroll the camera is close to
	void DrawTempleText(const DrawSceneDesc& desc) const;
	/// The temple's map: the land seen from above, unlit, once a visit to the temple
	void DrawTempleMapPass() const;
	/// The main room's pool: its water twice, turned an eighth apart, each shimmering as the other fades
	void DrawTemplePool(const DrawSceneDesc& desc) const;
	/// The temple's map: the island in relief over the main room's pool
	void DrawTempleMap(const DrawSceneDesc& desc) const;
	/// A black floor under the whole temple, which the cracks the rooms are modelled with show instead of the sky
	void DrawTempleUnderside(const DrawSceneDesc& desc) const;
	/// The markers on the temple's map, on soft glows that show through anything
	void DrawTempleMapMarkers(const DrawSceneDesc& desc) const;
	/// The creature's room's belts and medals
	void DrawCaveTrophies(const DrawSceneDesc& desc) const;

	std::unique_ptr<ShaderManager> _shaderManager;
	std::unique_ptr<BgfxCallback> _bgfxCallback;
	uint32_t _bgfxReset;
	bool _bgfxDebug = false;
	bool _bgfxProfile = false;

	std::unique_ptr<FrameBuffer> _handShadowFrameBuffer;
	/// Where the objects' shadows cover the island, laid out as its footprints are and made for the island's size
	mutable std::unique_ptr<FrameBuffer> _objectShadowFrameBuffer;
	/// The land seen from above for the temple's map, and the visit to the temple it was drawn for
	mutable std::unique_ptr<FrameBuffer> _templeMapFrameBuffer;
	mutable std::optional<uint32_t> _templeMapVisit;
	/// The hand's shadow of the frame being drawn
	mutable std::optional<HandShadow> _handShadow;
	/// The land's light this frame, and the 256 by 1 texture the terrain reads it from
	mutable std::unique_ptr<LandLightTable> _landLightTable;
	/// The sea's ripple step, 0 to 15, moving on each frame the sea's rows are drawn while the game's time goes on
	mutable uint8_t _seaRippleStep {0};
	/// The land's luminosity this frame, sized to the land's
	mutable std::unique_ptr<FrameBuffer> _landLuminosityFrameBuffer;
	/// What shades the land this frame: the clouds' shadows and the lights at night
	mutable std::unique_ptr<FrameBuffer> _landShadeFrameBuffer;
	/// The land cells' colours this frame, sized to the land's
	mutable std::unique_ptr<FrameBuffer> _landColourFrameBuffer;
	/// The sky's dome for each alignment, one above the other, kept from frame to frame
	mutable std::unique_ptr<FrameBuffer> _skyDomeFrameBuffer;
	/// The glow lightning stamps on the land, made the first time it is drawn
	mutable std::optional<TextureHandle> _lightningGlowTexture;
	/// How strongly the sun glares, 0 to 255, easing towards how much of the sun shows
	mutable float _sunGlare {0.0f};
	mutable std::optional<TextureHandle> _landLightTexture;
	/// u_haze and u_hazeColour of the frame's distance haze, off until the land's light is built
	mutable std::array<glm::vec4, 2> _haze {};
	/// The colours the sky's dome is drawn in this frame
	mutable sky_dome::Tint _skyTint;
	/// The overcast at the camera this frame, which dims the sun and the moon
	mutable float _overcast {0.0f};
	/// icons.raw with iconsa.raw's alpha, which the creature's room's belts and medals are drawn with, once loaded
	mutable std::optional<TextureHandle> _iconsTexture;
	mutable bool _iconsLoaded {false};
	/// Sampled by the primitives without a skin, as Direct3D's texture stages read white with no texture set
	std::optional<TextureHandle> _whiteTexture;
	/// u_modelLight: where the game's model light is this frame, and its ambient
	mutable glm::vec4 _modelLight {0.0f};
	/// What the land's light is scaled by for the trees this frame, of 1
	mutable float _treeBrightness {1.0f};
	/// u_modelLight: the game's model light this frame
	[[nodiscard]] glm::vec4 GetModelLight() const;
	/// Rebuilds the land's light for this frame from the palette; the texture of it
	TextureHandle UpdateLandLight() const;
	/// The land's light, or a texture to bind in its place before it is first built
	[[nodiscard]] TextureHandle GetLandLightTexture() const;
	std::unique_ptr<Mesh> _plane;
};
} // namespace graphics
} // namespace openblack
