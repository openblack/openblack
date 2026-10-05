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
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

#include "Graphics/HandShadow.h"
#include "Graphics/RenderPass.h"
#include "Graphics/RendererInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

namespace openblack
{
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
	/// Casts the shadows of the island's trees, rocks and buildings onto it (see ObjectShadows)
	void DrawObjectShadowPass(const DrawSceneDesc& drawDesc) const;
	/// u_handLight of the land: where HandLight's map lies and how strongly it lights, loading the map the first time
	[[nodiscard]] glm::vec4 GetHandLight(const DrawSceneDesc& drawDesc) const;
	/// Draws the hand's silhouette for its shadow, or clears the shadow when there is none
	void DrawHandShadowPass(const DrawSceneDesc& drawDesc) const;
	/// Draws a submesh, with a texture in place of its skins when given one
	void DrawSubMesh(const L3DMesh& mesh, const L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc, bool preserveState,
	                 const TextureHandle* texture = nullptr, glm::vec3 glow = glm::vec3(0.0f)) const;
	void DrawPass(const DrawSceneDesc& desc) const;
	/// The beams of the temple's spot lights and the light its windows shed, which InnerRoom::DrawGlow draws in its
	/// rooms but not in the reflection of the main room
	void DrawLightBeams(const DrawSceneDesc& desc) const;
	/// The temple's domes of mist, which LH3DMist draws facing the camera with a frame of the smoke texture
	void DrawMistDomes(const DrawSceneDesc& desc) const;
	/// The text the temple's rooms write in the world this frame: the signs' labels and the scroll the camera is close to
	void DrawTempleText(const DrawSceneDesc& desc) const;
	/// MiniMap::BuildMapTex: the land seen from above, unlit, once a visit to the temple
	void DrawTempleMapPass(const DrawSceneDesc& desc) const;
	/// WorldRoom::Draw's pool: its water twice, turned an eighth apart, each shimmering as the other fades
	void DrawTemplePool(const DrawSceneDesc& desc) const;
	/// MiniMap's draw: the island in relief over the main room's pool
	void DrawTempleMap(const DrawSceneDesc& desc) const;
	/// A black floor under the whole temple, which the cracks the rooms are modelled with show instead of the sky
	void DrawTempleUnderside(const DrawSceneDesc& desc) const;
	/// WorldRoom::DrawAdditional: the markers on the map, on soft glows that show through anything
	void DrawTempleMapMarkers(const DrawSceneDesc& desc) const;

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
	/// HandLight's map, loaded with the first land drawn, empty when the game has none
	mutable std::optional<TextureHandle> _handLightTexture;
	mutable bool _handLightLoaded {false};
	/// Sampled by the primitives without a skin, as Direct3D's texture stages read white with no texture set
	std::optional<TextureHandle> _whiteTexture;
	/// u_handLight of the pass being drawn, without strength outside of the scene's passes
	mutable glm::vec4 _handLight {0.0f};
	/// HandLight's map, or a texture to bind in its place when there is none
	[[nodiscard]] TextureHandle GetHandLightTexture() const;
	std::unique_ptr<Mesh> _plane;
};
} // namespace graphics
} // namespace openblack
