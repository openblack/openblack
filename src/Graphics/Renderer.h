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
#include <unordered_map>
#include <utility>
#include <vector>

#include <SDL.h>
#include <entt/core/fwd.hpp>
#include <entt/entity/fwd.hpp>
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "3D/SkyDome.h"
#include "Graphics/CreatureShadow.h"
#include "Graphics/HandShadow.h"
#include "Graphics/RenderPass.h"
#include "Graphics/RendererInterface.h"
#include "Particles/ParticleDrawFrame.h"

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
class MorphStreamLayouts;
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
	/// The creatures' footprints laid over the land, blended before the rest of what blends, in the main view and the
	/// sea's reflection
	void DrawCreatureFootprints(const DrawSceneDesc& desc) const;
	/// The villagers' ground blobs, in the main view
	void DrawGroundBlobs(const DrawSceneDesc& desc) const;
	/// The rain about the camera, each block's in its place among what blends, in the main view
	void DrawRain(const DrawSceneDesc& desc) const;
	/// The rings on the water where things splashed, added over the land and the sea
	void DrawWaterRings(const DrawSceneDesc& desc) const;
	/// The snow falling about the camera where it snows, each quarter of a land block in its place among what blends
	void DrawSnowfall(const DrawSceneDesc& desc) const;
	/// The smoke from the homes' chimneys, each in its place among what blends, in the main view
	void DrawChimneySmoke(const DrawSceneDesc& desc) const;
	/// Gathers what the particle effects draw this frame, once for every pass
	void CollectParticles() const;
	/// The particle effects in the order the camera of the pass draws them, among the other things that blend: runs of
	/// sprites that share a sheet as one instanced draw, ribbons, models and mists
	void DrawParticles(const DrawSceneDesc& desc) const;
	/// A particle's mist, in the translucent pass at a place among what blends
	void DrawParticleMist(const DrawSceneDesc& desc, const particles::draw::MistDraw& mist, uint32_t depth) const;
	/// A particle's model, in the translucent pass at a place among what blends
	void DrawParticleMesh(const DrawSceneDesc& desc, const particles::draw::MeshDraw& mesh, uint32_t depth) const;
	/// A frame of a particle light map as a texture of its colours, made when first stamped
	[[nodiscard]] const Texture2D* ParticleLightMap(entt::id_type bitmap, int frame) const;
	/// The border of the players' influence, in the main view
	void DrawInfluenceBorder(const DrawSceneDesc& desc) const;
	/// The ripples the hand makes crossing a border, each in its place among what blends, in the main view
	void DrawInfluenceRipples(const DrawSceneDesc& desc) const;
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
	/// Draws the silhouettes of the creatures nearest the camera from the light, for their shadows
	void DrawCreatureShadowPass(const DrawSceneDesc& drawDesc) const;
	/// Hands the creatures' shadows to a program that takes them, or none where it isn't to receive them
	void SetCreatureShadowUniforms(const ShaderProgram& program, bool receives) const;
	/// The uniforms the draws of meshes set, in the order of their names in the renderer
	enum class MeshUniform : uint8_t
	{
		DepthBias,
		Tint,
		Glow,
		Darkening,
		MorphWeights,
		VertexBlend,
		UvOffset,
		SeaClip,
		Snow,
		SnowDepth,
		SnowTexture,
		SnowAlpha,
		Window,
		Diffuse,
		Lightmap,
		Environment,
		Heightmap,
		IslandExtent,
		LandLight,
		LandLuminosity,
		LandLightTexture,
		LandColour,
		Haze,
		HazeColour,
		ModelLight,
		CreatureShadowInfo,
		CreatureShadows,
		CreatureShadowMatrix,
		CreatureShadow,
		SkyAlphaThreshold,

		_count
	};
	static constexpr std::array<std::string_view, static_cast<size_t>(MeshUniform::_count)> k_MeshUniformNames {
	    "u_depthBias",            //
	    "u_tint",                 //
	    "u_glow",                 //
	    "u_darkening",            //
	    "u_morphWeights",         //
	    "u_vertexBlend",          //
	    "u_uvOffset",             //
	    "u_seaClip",              //
	    "u_snow",                 //
	    "s_snowDepth",            //
	    "s_snow",                 //
	    "s_snowAlpha",            //
	    "u_window",               //
	    "s_diffuse",              //
	    "s_lightmap",             //
	    "s_environment",          //
	    "s_heightmap",            //
	    "u_islandExtent",         //
	    "u_landLight",            //
	    "s_landLuminosity",       //
	    "s_landLight",            //
	    "s_landColour",           //
	    "u_haze",                 //
	    "u_hazeColour",           //
	    "u_modelLight",           //
	    "u_creatureShadowInfo",   //
	    "s_creatureShadows",      //
	    "u_creatureShadowMatrix", //
	    "u_creatureShadow",       //
	    "u_skyAlphaThreshold",    //
	};
	using MeshUniforms = std::array<std::optional<UniformHandle>, static_cast<size_t>(MeshUniform::_count)>;
	/// A program's handles of the mesh uniforms it has, looked up by name the first time it draws a mesh
	[[nodiscard]] const MeshUniforms& MeshUniformsOf(const ShaderProgram& program) const;
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
	/// A creature's eyes and eyelids, after its body
	void DrawCreatureEyes(const DrawSceneDesc& desc, entt::entity entity, const L3DMeshSubmitDesc& bodyDesc) const;
	/// A creature's strands of hair, as ribbons facing the camera blended over the scene
	void DrawCreatureHair(const DrawSceneDesc& desc, entt::entity entity) const;
	/// The leashes' ropes, each a ribbon lit by the land beneath it, and their shadows on the land
	void DrawLeashes(const DrawSceneDesc& desc) const;
	/// Takes up the creatures' skins where they have been painted again, before their bodies are drawn with them; drops
	/// those of creatures no longer on the land
	void UploadCreatureSkins(const DrawSceneDesc& drawDesc) const;
	/// The hand's skins as blended for its player's alignment (see components::HandMorph), taken up when they change
	void UploadHandSkins() const;
	/// What the hand's base mesh is pulled towards for its player's alignment, none while it shows the base
	[[nodiscard]] std::optional<L3DMeshSubmitDesc::MorphTargets> HandMorphTargets() const;

	std::unique_ptr<ShaderManager> _shaderManager;
	std::unique_ptr<BgfxCallback> _bgfxCallback;
	uint32_t _bgfxReset;
	bool _bgfxDebug = false;
	bool _bgfxProfile = false;

	std::unique_ptr<FrameBuffer> _handShadowFrameBuffer;
	/// How a creature's variant meshes are read to blend its body
	std::unique_ptr<MorphStreamLayouts> _morphStreamLayouts;
	/// Where the objects' shadows cover the island, laid out as its footprints are and made for the island's size
	mutable std::unique_ptr<FrameBuffer> _objectShadowFrameBuffer;
	/// The land seen from above for the temple's map, and the visit to the temple it was drawn for
	mutable std::unique_ptr<FrameBuffer> _templeMapFrameBuffer;
	mutable std::optional<uint32_t> _templeMapVisit;
	/// The hand's shadow of the frame being drawn
	mutable std::optional<HandShadow> _handShadow;
	/// The creatures' silhouettes, a cell each, and the shadows of the frame being drawn
	std::unique_ptr<FrameBuffer> _creatureShadowFrameBuffer;
	mutable std::array<glm::mat4, CreatureShadow::k_MaxShadows> _creatureShadowMatrices {};
	/// x: how much a fully covered texel darkens, y: where along the light the shadow starts
	mutable std::array<glm::vec4, CreatureShadow::k_MaxShadows> _creatureShadowParameters {};
	mutable uint8_t _creatureShadowCount {0};
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
	/// The mesh uniforms of each program that has drawn a mesh; the programs live as long as the renderer
	mutable std::unordered_map<const ShaderProgram*, MeshUniforms> _meshUniforms;
	/// The snow's textures, looked up once a frame, null until they are loaded
	mutable const Texture2D* _snowTexture {nullptr};
	mutable const Texture2D* _snowAlphaTexture {nullptr};
	/// How deep the snow lies over the island, refreshed when it changes
	mutable std::unique_ptr<Texture2D> _snowDepth;
	mutable std::optional<uint32_t> _snowRevision;
	/// u_haze and u_hazeColour of the frame's distance haze, off until the land's light is built
	mutable std::array<glm::vec4, 2> _haze {};
	/// The colours the sky's dome is drawn in this frame
	mutable sky_dome::Tint _skyTint;
	/// The overcast at the camera this frame, which dims the sun and the moon
	mutable float _overcast {0.0f};
	/// icons.raw with iconsa.raw's alpha, which the creature's room's belts and medals are drawn with, once loaded
	mutable std::optional<TextureHandle> _iconsTexture;
	mutable bool _iconsLoaded {false};
	/// A creature's skins as painted (see components::CreatureSkin), one texture each
	struct CreatureSkins
	{
		/// The painting the textures hold
		uint32_t revision {0};
		std::vector<std::unique_ptr<Texture2D>> textures;
		/// The painted skins the body is drawn with in place of its base mesh's, by skin id
		std::vector<std::pair<uint32_t, const Texture2D*>> drawn;
	};
	mutable std::unordered_map<entt::entity, CreatureSkins> _creatureSkins;
	/// Whether running out of textures for the creatures' skins has been logged
	mutable bool _warnedOutOfSkins {false};
	/// The hand's skins as blended for its player's alignment, one texture each, and the blending they hold
	mutable std::vector<std::unique_ptr<Texture2D>> _handSkinTextures;
	mutable std::vector<std::pair<uint32_t, const Texture2D*>> _handSkins;
	mutable std::optional<uint32_t> _handSkinRevision;
	/// The creatures drawn this frame: all of them, or the nearest the camera when there are more than the backend can
	/// upload the bones of
	struct DrawnCreature
	{
		entt::entity entity;
		uint32_t instance;
	};
	mutable std::vector<DrawnCreature> _drawnCreatures;
	mutable bool _warnedCreatureCap {false};
	/// Found by trial in a debug build, whose backends check the uploads: Vulkan overflowed with 65 creatures drawn and
	/// Direct3D 12 with 850, and both held with 50 and 600
	static constexpr size_t k_MaxCreaturesDrawnVulkan = 40;
	static constexpr size_t k_MaxCreaturesDrawnDirect3D12 = 500;
	void SelectDrawnCreatures(const DrawSceneDesc& drawDesc) const;
	[[nodiscard]] static size_t MaxCreaturesDrawn();
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
	/// What the particle effects draw this frame, and the order of the pass being drawn, kept for their room
	mutable particles::draw::Frame _particleFrame;
	mutable std::vector<particles::draw::Command> _particleCommands;
	mutable std::vector<uint32_t> _particleSpriteOrder;
	/// The particle light maps' frames stamped so far, by light map and frame
	mutable std::unordered_map<uint64_t, std::unique_ptr<Texture2D>> _particleLightMaps;
};
} // namespace graphics
} // namespace openblack
