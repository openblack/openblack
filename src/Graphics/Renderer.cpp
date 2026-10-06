/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <cstring>

#include <algorithm>
#include <chrono>
#include <limits>
#include <map>
#include <memory>
#include <span>
#define LOCATOR_IMPLEMENTATIONS

#include <cstdint>

#include <LNDFile.h>
#include <SDL_video.h>
#include <bgfx/platform.h>
#include <bimg/bimg.h>
#include <bx/file.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/ChimneySmoke.h"
#include "3D/Clouds.h"
#include "3D/DayNightClock.h"
#include "3D/InfluenceCircle.h"
#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandBlock.h"
#include "3D/LandColourStamps.h"
#include "3D/LandIslandInterface.h"
#include "3D/LandLightFrame.h"
#include "3D/LandLightTable.h"
#include "3D/Lightning.h"
#include "3D/Mists.h"
#include "3D/OceanInterface.h"
#include "3D/OrientedText.h"
#include "3D/Rain.h"
#include "3D/SkyInterface.h"
#include "3D/SnowCover.h"
#include "3D/TempleDoors.h"
#include "3D/TempleInteriorInterface.h"
#include "3D/TempleMap.h"
#include "3D/VillageLights.h"
#include "3D/WaterRings.h"
#include "Camera/Camera.h"
#include "Common/CrashHandler.h"
#include "Creature/CreatureHair.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreatureSkin.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/ChimneySmoke.h"
#include "ECS/Components/Cloud.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureHair.h"
#include "ECS/Components/CreatureSkin.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/LightBeam.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mist.h"
#include "ECS/Components/MistDome.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Stream.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/VillageLight.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/Weather.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/CreatureHairSystemInterface.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "ECS/Systems/FootprintSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/RainSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "ECS/Systems/SnowSystemInterface.h"
#include "ECS/Systems/SnowfallSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "EngineConfig.h"
#include "FileSystem/FileSystemInterface.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/DetailLevel.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/GroundBlobs.h"
#include "Graphics/HandLight.h"
#include "Graphics/HandWaterGlow.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/LightBeams.h"
#include "Graphics/ModelLight.h"
#include "Graphics/Moon.h"
#include "Graphics/ObjectShadows.h"
#include "Graphics/Primitive.h"
#include "Graphics/SeaRows.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/Sun.h"
#include "Graphics/TreeBrightness.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/ZSort.h"
#include "Locator.h"
#include "Profiler.h"
#include "Renderer.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "Windowing/WindowingInterface.h"
#include "entt/locator/locator.hpp"

using namespace openblack;
using namespace openblack::graphics;
using namespace openblack::ecs::systems;

namespace openblack::graphics
{
/// The layouts a creature's variant meshes are read with as the second to fourth vertex streams: their positions and
/// normals as the attributes the morphing vertex shader takes them in, the rest of each vertex skipped
class MorphStreamLayouts
{
public:
	MorphStreamLayouts()
	{
		constexpr std::array<std::pair<bgfx::Attrib::Enum, bgfx::Attrib::Enum>, 3> k_Attributes {{
		    {bgfx::Attrib::Tangent, bgfx::Attrib::Bitangent},
		    {bgfx::Attrib::Color1, bgfx::Attrib::Color2},
		    {bgfx::Attrib::Color3, bgfx::Attrib::Weight},
		}};
		// As L3DSubMesh packs a vertex: position, texture coordinates, normal and bone indices
		constexpr uint8_t k_TexCoordBytes = 2 * sizeof(float);
		constexpr uint8_t k_IndicesBytes = 4 * sizeof(int16_t);
		for (size_t axis = 0; axis < k_Attributes.size(); ++axis)
		{
			bgfx::VertexLayout layout;
			layout.begin()
			    .add(k_Attributes.at(axis).first, 3, bgfx::AttribType::Float)
			    .skip(k_TexCoordBytes)
			    .add(k_Attributes.at(axis).second, 3, bgfx::AttribType::Float)
			    .skip(k_IndicesBytes)
			    .end();
			_layouts.at(axis) = fromBgfx(bgfx::createVertexLayout(layout));
		}
	}
	~MorphStreamLayouts()
	{
		for (const auto& layout : _layouts)
		{
			if (bgfx::isValid(toBgfx(layout)))
			{
				bgfx::destroy(toBgfx(layout));
			}
		}
	}
	MorphStreamLayouts(const MorphStreamLayouts&) = delete;
	MorphStreamLayouts& operator=(const MorphStreamLayouts&) = delete;
	MorphStreamLayouts(MorphStreamLayouts&&) = delete;
	MorphStreamLayouts& operator=(MorphStreamLayouts&&) = delete;

	[[nodiscard]] VertexLayoutHandle Get(size_t axis) const { return _layouts.at(axis); }

private:
	std::array<VertexLayoutHandle, 3> _layouts {};
};
} // namespace openblack::graphics

namespace openblack
{
// clang-format off
constexpr auto k_BgfxDefaultStateInvertedZ = 0 \
                                     | BGFX_STATE_WRITE_RGB \
                                     | BGFX_STATE_WRITE_A \
                                     | BGFX_STATE_WRITE_Z \
                                     | BGFX_STATE_CULL_CW \
                                     | BGFX_STATE_DEPTH_TEST_GREATER \
                                     | BGFX_STATE_MSAA;
// clang-format on

namespace
{
/// How deep the snow lies over the island, as a texture the shaders read point by point, refreshed when the snow
/// changes; none without snow
const Texture2D* SnowDepth(std::unique_ptr<Texture2D>& texture, std::optional<uint32_t>& revision)
{
	if (!Locator::snowSystem::has_value())
	{
		return nullptr;
	}
	const auto& snow = Locator::snowSystem::value();
	if (!texture)
	{
		texture = std::make_unique<Texture2D>("SnowDepth");
		texture->Create(snow_cover::k_GridSize, snow_cover::k_GridSize, 1, TextureFormat::R32F, Wrapping::ClampEdge,
		                Filter::Nearest, nullptr);
		revision.reset();
	}
	if (revision != snow.GetRevision())
	{
		const auto depths = snow.GetDepths();
		texture->Update(depths.data(), static_cast<uint32_t>(depths.size_bytes()));
		revision = snow.GetRevision();
	}
	return texture.get();
}

/// The pass what blends in a scene goes to: its own pass after the scene's, but in the temple the scene's own, which
/// draws everything in the order it comes
RenderPass TranslucentView(RenderPass scene)
{
	return Locator::temple::has_value() && Locator::temple::value().Active() ? scene : TranslucentPassOf(scene);
}
} // namespace

/// How far back, as a fraction of their depth, the temple's rooms the player isn't in are drawn: a few millimetres at the
/// doorways, past the rounding of the copies of their arches
constexpr float k_OtherTempleRoomDepthBias = 5e-5f;

struct BgfxCallback: public bgfx::CallbackI
{
	constexpr static std::array<std::string_view, bgfx::Fatal::Count> k_CodeLookup = {
	    "DebugCheck",            //
	    "InvalidShader",         //
	    "UnableToInitialize",    //
	    "UnableToCreateTexture", //
	    "DeviceLost",            //
	};

	~BgfxCallback() override = default;

	void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) override
	{
		const auto* codeStr = k_CodeLookup.at(code).data();
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "bgfx: {}:{}: FATAL ({}): {}", filePath, line, codeStr, str);

#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_CRITICAL
		spdlog::get("graphics")
		    ->log(spdlog::source_loc {filePath, line, SPDLOG_FUNCTION}, spdlog::level::critical, "FATAL ({}): {}", codeStr,
		          str);
#endif

		// Must terminate, continuing will cause crash anyway. This can come from bgfx's render thread, where nothing
		// would catch an exception, so it is reported and the game exits from here.
		if (crash_handler::IsInstalled())
		{
			crash_handler::ReportFatal(crash_report::CrashKind::GraphicsFatal, str, filePath, line,
			                           std::string("bgfx ") + codeStr);
		}
		throw std::runtime_error(std::string("bgfx: ") + filePath + ":" + std::to_string(line) + ": FATAL (" + codeStr +
		                         "): " + str);
	}

	void traceVargs([[maybe_unused]] const char* filePath, [[maybe_unused]] uint16_t line, const char* format,
	                va_list argList) override
	{
		std::array<char, 0x2000> temp;
		char* out = temp.data();
		int32_t len = vsnprintf(out, temp.size(), format, argList);
		if (static_cast<int32_t>(temp.size()) < len)
		{
			out = reinterpret_cast<char*>(alloca(len + 1));
			len = vsnprintf(out, len, format, argList);
		}
		if (len > 0)
		{
			out[len] = '\0';
			if (len > 0 && out[len - 1] == '\n')
			{
				out[len - 1] = '\0';
			}
// TODO(bwrsandman): change level to trace
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_DEBUG
			spdlog::get("graphics")->log(spdlog::source_loc {filePath, line, SPDLOG_FUNCTION}, spdlog::level::debug, out);
#endif
		}
		else
		{
#if SPDLOG_ACTIVE_LEVEL <= SPDLOG_LEVEL_ERROR
			spdlog::get("graphics")
			    ->log(spdlog::source_loc {filePath, line, SPDLOG_FUNCTION}, spdlog::level::err,
			          "bgfx: failed to format message: {}", format);
#endif
		}
	}
	void profilerBegin([[maybe_unused]] const char* name, [[maybe_unused]] uint32_t abgr, [[maybe_unused]] const char* filePath,
	                   [[maybe_unused]] uint16_t line) override
	{
	}
	void profilerBeginLiteral([[maybe_unused]] const char* name, [[maybe_unused]] uint32_t abgr,
	                          [[maybe_unused]] const char* filePath, [[maybe_unused]] uint16_t line) override
	{
	}
	void profilerEnd() override {}
	// Reading and writing to shader cache
	uint32_t cacheReadSize([[maybe_unused]] uint64_t id) override { return 0; }
	bool cacheRead([[maybe_unused]] uint64_t id, [[maybe_unused]] void* data, [[maybe_unused]] uint32_t size) override
	{
		return false;
	}
	void cacheWrite([[maybe_unused]] uint64_t id, [[maybe_unused]] const void* data, [[maybe_unused]] uint32_t size) override {}
	// Saving a screenshot
	void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, const void* data,
	                [[maybe_unused]] uint32_t size, bool yflip) override
	{
		SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Taking a screenshot...");

		const auto ext = std::filesystem::path(filePath).extension();
		if (std::filesystem::path(filePath).extension() == ".png")
		{
			bx::FileWriter writer;
			bx::Error err;
			if (bx::open(&writer, filePath, false, &err))
			{
				// Strip out alpha for screenshot
				std::vector<uint32_t> noAlpha;
				noAlpha.resize(size / sizeof(noAlpha[0]), 0);
				memcpy(noAlpha.data(), data, size);
				for (uint32_t y = 0; y < height; ++y)
				{
					for (uint32_t x = 0; x < width; ++x)
					{
						noAlpha[x + pitch / sizeof(noAlpha[0]) * y] |= 0xFF000000;
					}
				}

				bimg::imageWritePng(&writer, width, height, pitch, noAlpha.data(), bimg::TextureFormat::BGRA8, yflip, &err);
				bx::close(&writer);
				SPDLOG_LOGGER_INFO(spdlog::get("graphics"), "Screenshot ({}x{}) saved at {}", width, height, filePath);
			}
			else
			{
				SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Failed to save Screenshot ({}x{}) at {}: {}", width, height,
				                    filePath, std::string(err.getMessage().getCPtr(), err.getMessage().getLength()));
			}
		}
		else
		{
			SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: {} screenshot ({}x{}) requested at {}", ext.string(),
			                   width, height, filePath);
		}
	}
	// Saving a video
	void captureBegin(uint32_t width, uint32_t height, [[maybe_unused]] uint32_t pitch,
	                  [[maybe_unused]] bgfx::TextureFormat::Enum format, [[maybe_unused]] bool yflip) override
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: Video Capture Begin ({}x{}) requested", width, height);
	}
	void captureEnd() override { SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: Video Capture End requested"); }
	void captureFrame([[maybe_unused]] const void* data, [[maybe_unused]] uint32_t size) override
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Not Implemented: Video Capture Frame requested");
	}
};

} // namespace openblack

std::unique_ptr<RendererInterface> RendererInterface::Create(GraphicsBackend backend, bool vsync) noexcept
{
	bgfx::Init init {};
	switch (backend)
	{
	case GraphicsBackend::Noop:
		init.type = bgfx::RendererType::Noop;
		break;
	case GraphicsBackend::Direct3D12:
		init.type = bgfx::RendererType::Direct3D12;
		break;
	case GraphicsBackend::Metal:
		init.type = bgfx::RendererType::Metal;
		break;
	case GraphicsBackend::Vulkan:
		init.type = bgfx::RendererType::Vulkan;
		break;
	default:
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Got impossible graphics backend.");
		return nullptr;
	}

	// Get render area size
	glm::uvec2 drawableSize;
	if (backend != GraphicsBackend::Noop)
	{
		const auto& window = Locator::windowing::value();

		drawableSize = static_cast<glm::uvec2>(window.GetSize());
		init.resolution.width = static_cast<uint32_t>(drawableSize.x);
		init.resolution.height = static_cast<uint32_t>(drawableSize.y);

		// Get Native Handles from SDL window
		const auto handles = window.GetNativeHandles();
		init.platformData.nwh = handles.nativeWindow;
		init.platformData.ndt = handles.nativeDisplay;
	}

	uint32_t bgfxReset = BGFX_RESET_NONE;
	auto bgfxCallback = std::make_unique<BgfxCallback>();
	if (vsync)
	{
		bgfxReset |= BGFX_RESET_VSYNC;
	}
	init.resolution.reset = bgfxReset;
	init.callback = dynamic_cast<bgfx::CallbackI*>(bgfxCallback.get());

	if (!bgfx::init(init))
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Failed to initialize bgfx.");
		return nullptr;
	}

	const bgfx::Caps* caps = bgfx::getCaps();
	if ((caps->supported & BGFX_CAPS_TEXTURE_2D_ARRAY) == 0 || caps->limits.maxTextureLayers < 9)
	{
		SPDLOG_LOGGER_CRITICAL(spdlog::get("graphics"), "Graphics device must support texture layers.");
		return nullptr;
	}

	return std::make_unique<Renderer>(bgfxReset, std::move(bgfxCallback));
}

Renderer::Renderer(uint32_t bgfxReset, std::unique_ptr<BgfxCallback>&& bgfxCallback) noexcept
    : _shaderManager(std::make_unique<ShaderManager>())
    , _bgfxCallback(std::move(bgfxCallback))
    , _bgfxReset(bgfxReset)
{
	_shaderManager->LoadShaders();
	_plane = Primitive::CreatePlane();
	_morphStreamLayouts = std::make_unique<MorphStreamLayouts>();
	{
		constexpr uint32_t k_White = 0xFFFFFFFF;
		_whiteTexture = fromBgfx(bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE,
		                                               bgfx::copy(&k_White, sizeof(k_White))));
		bgfx::setName(toBgfx(*_whiteTexture), "White");
	}
	// The game rasterises shadows with eight coverage samples per texel, multisampling gives the same soft edges
	_handShadowFrameBuffer =
	    std::make_unique<FrameBuffer>("Hand Shadow", HandShadow::k_TextureSize, HandShadow::k_TextureSize, TextureFormat::R8,
	                                  std::nullopt, static_cast<uint8_t>(8), Wrapping::ClampEdge);
	// The creatures' silhouettes side by side, with the same soft edges
	_creatureShadowFrameBuffer = std::make_unique<FrameBuffer>(
	    "Creature Shadows", static_cast<uint16_t>(CreatureShadow::k_CellSize * CreatureShadow::k_MaxShadows),
	    CreatureShadow::k_CellSize, TextureFormat::R8, std::nullopt, static_cast<uint8_t>(8), Wrapping::ClampEdge);

	// give debug names to views
	// TODO (#749) use std::views::enumerate
	for (bgfx::ViewId i = 0; const auto& name : k_RenderPassNames)
	{
		bgfx::setViewName(i, name.data());
		++i;
	}
}

Renderer::~Renderer() noexcept
{
	_creatureSkins.clear();
	_snowDepth.reset();
	_plane.reset();
	_morphStreamLayouts.reset();
	_handShadowFrameBuffer.reset();
	_creatureShadowFrameBuffer.reset();
	_objectShadowFrameBuffer.reset();
	_templeMapFrameBuffer.reset();
	if (_landLightTexture)
	{
		bgfx::destroy(toBgfx(*_landLightTexture));
	}
	if (_iconsTexture)
	{
		bgfx::destroy(toBgfx(*_iconsTexture));
	}
	if (_whiteTexture)
	{
		bgfx::destroy(toBgfx(*_whiteTexture));
	}
	_landLuminosityFrameBuffer.reset();
	_landShadeFrameBuffer.reset();
	_landColourFrameBuffer.reset();
	_skyDomeFrameBuffer.reset();
	if (_lightningGlowTexture)
	{
		bgfx::destroy(toBgfx(*_lightningGlowTexture));
	}
	_shaderManager.reset();
	bgfx::frame();
	bgfx::shutdown();
}

void Renderer::ConfigureView(graphics::RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept
{
	// A scene with a sky of its own is cleared by its sky's pass, which comes first, and drawn over it
	const auto skyId = static_cast<bgfx::ViewId>(SkyPassOf(viewId));
	bgfx::setViewClear(skyId, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 0.0f, 0);
	bgfx::setViewRect(skyId, 0, 0, resolution.x, resolution.y);
	if (HasSkyPass(viewId))
	{
		bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_NONE);
		bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, resolution.x, resolution.y);
	}
	// And what blends in it is drawn over it after
	if (const auto translucentId = TranslucentPassOf(viewId); translucentId != viewId)
	{
		bgfx::setViewClear(static_cast<bgfx::ViewId>(translucentId), BGFX_CLEAR_NONE);
		bgfx::setViewRect(static_cast<bgfx::ViewId>(translucentId), 0, 0, resolution.x, resolution.y);
	}
}

void Renderer::Reset(glm::u16vec2 resolution) const noexcept
{
	bgfx::reset(resolution.x, resolution.y, _bgfxReset);
}

graphics::ShaderManager& Renderer::GetShaderManager() const noexcept
{
	return *_shaderManager;
}

const Texture2D* GetTexture(uint32_t skinID, const std::unordered_map<SkinId, std::unique_ptr<graphics::Texture2D>>& meshSkins)
{
	const auto& textureManager = Locator::resources::value().GetTextures();

	const Texture2D* texture = nullptr;

	if (skinID != 0xFFFFFFFF)
	{
		if (meshSkins.find(skinID) != meshSkins.end())
		{
			texture = meshSkins.at(skinID).get();
		}
		else if (textureManager.Contains(skinID))
		{
			texture = &*textureManager.Handle(skinID);
		}
		else
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("graphics"), "Could not find the texture");
		}
	}

	return texture;
}

namespace
{
/// The land's light a creature, its eyes and its hair take: halved in the sea's reflection, as the game halves every
/// level of the land's light while it draws the reflection
float CreatureLandLightScale(RenderPass viewId)
{
	constexpr float k_ReflectionLight = 0.5f;
	return viewId == RenderPass::Reflection || viewId == RenderPass::ReflectionTranslucent ? k_ReflectionLight : 1.0f;
}

/// Binds the submeshes of a creature's variant meshes that match one of its base mesh as the second to fourth vertex
/// streams; the base's own where a variant's doesn't match
void BindMorphTargets(const L3DMesh& mesh, const L3DSubMesh& subMesh,
                      const RendererInterface::L3DMeshSubmitDesc::MorphTargets& targets, const MorphStreamLayouts& layouts)
{
	const auto& subMeshes = mesh.GetSubMeshes();
	const auto found = std::ranges::find_if(subMeshes, [&subMesh](const auto& other) { return other.get() == &subMesh; });
	const auto index = static_cast<size_t>(std::distance(subMeshes.begin(), found));
	const auto& base = subMesh.GetMesh().GetVertexBuffer();
	for (size_t axis = 0; axis < targets.meshes.size(); ++axis)
	{
		const auto* target = targets.meshes.at(axis);
		const auto* buffer = &base;
		if (target != nullptr && index < target->GetSubMeshes().size())
		{
			const auto& candidate = target->GetSubMeshes()[index]->GetMesh().GetVertexBuffer();
			if (candidate.GetCount() == base.GetCount() && candidate.GetStrideBytes() == base.GetStrideBytes())
			{
				buffer = &candidate;
			}
		}
		buffer->BindStream(static_cast<uint8_t>(axis + 1), layouts.Get(axis));
	}
}

/// Binds every vertex's position and bone in the submesh and in its matches of the creature's variant meshes, for the
/// vertex shader to blend the seams with; the base's own where a variant's doesn't match. Off where the submesh has no
/// blends, or where the blends are turned off.
void BindBlendSources(const L3DMesh& mesh, const L3DSubMesh& subMesh,
                      const RendererInterface::L3DMeshSubmitDesc::MorphTargets& targets, const ShaderProgram& program)
{
	const auto* base = subMesh.GetBlendSource();
	const bool blended = targets.blendSeams && subMesh.HasBlends() && base != nullptr;
	const auto width = base != nullptr ? static_cast<float>(base->GetResolution().x) : 1.0f;
	const glm::vec4 u_vertexBlend {blended ? 1.0f : 0.0f, width, 0.0f, 0.0f};
	program.SetUniformValue("u_vertexBlend", &u_vertexBlend);
	if (!blended)
	{
		return;
	}
	const auto& subMeshes = mesh.GetSubMeshes();
	const auto found = std::ranges::find_if(subMeshes, [&subMesh](const auto& other) { return other.get() == &subMesh; });
	const auto index = static_cast<size_t>(std::distance(subMeshes.begin(), found));
	constexpr std::array<std::pair<const char*, uint8_t>, 3> k_Samplers {{
	    {"s_blendEvilGood", 3},
	    {"s_blendThinFat", 4},
	    {"s_blendWeakStrong", 9},
	}};
	program.SetTextureSampler("s_blendBase", 2, *base);
	for (size_t axis = 0; axis < targets.meshes.size(); ++axis)
	{
		const auto* target = targets.meshes.at(axis);
		const auto* source = base;
		if (target != nullptr && index < target->GetSubMeshes().size())
		{
			const auto* candidate = target->GetSubMeshes()[index]->GetBlendSource();
			if (candidate != nullptr && candidate->GetResolution() == base->GetResolution())
			{
				source = candidate;
			}
		}
		program.SetTextureSampler(k_Samplers.at(axis).first, k_Samplers.at(axis).second, *source);
	}
}
} // namespace

void Renderer::DrawSubMesh(const graphics::L3DMesh& mesh, const graphics::L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc,
                           bool preserveState, const TextureHandle* subMeshTexture, glm::vec3 glow) const
{
	assert(&subMesh.GetMesh());
	// We don't draw physics meshes, we haven't implemented statuses (building and graves) and modern GPUs can handle high
	// lod. Windows fail the game's lod test unless their house lights them, which the object shader decides.
	const bool window = subMesh.GetFlags().isWindow;
	// A lit window is a faint glow, blended by its texture's alpha as its material says, over the wall behind it
	const bool materialBlending = desc.useMaterialBlending || window;
	const auto viewId = window ? TranslucentView(desc.viewId) : desc.viewId;
	if (!desc.drawAll &&
	    (subMesh.IsPhysics() || subMesh.GetFlags().status != 0 || (!window && (subMesh.GetFlags().lodMask & 1) != 1)))
	{
		return;
	}

	const auto& island = Locator::terrainSystem::value();

	auto extent = island.GetExtent();
	auto islandExtent = glm::vec4(extent.minimum, extent.maximum);
	const auto& heightMap = island.GetHeightMap();

	auto const& skins = mesh.GetSkins();
	// The game draws the submeshes with a lightmap through it and the others as they are
	const auto lightmapSkinID = subMesh.GetLightmapSkinID();
	const Texture2D* lightmap = lightmapSkinID.has_value() ? GetTexture(*lightmapSkinID, skins) : nullptr;
	const auto* program = desc.lightmapProgram != nullptr && lightmap != nullptr ? desc.lightmapProgram : desc.program;
	if (desc.onlyJoints && !subMesh.GetJoint().has_value())
	{
		return;
	}
	if (const auto& joint = subMesh.GetJoint();
	    desc.hideShutJoints && joint.has_value() &&
	    (joint->index >= desc.joints.size() || desc.joints[joint->index] == glm::mat4(1.0f)))
	{
		return;
	}

	// The game turns a submesh with a joint about its pivot by its matrix of the table, before the mesh's own matrix
	const auto* modelMatrices = desc.modelMatrices;
	glm::mat4 jointModel;
	if (const auto& joint = subMesh.GetJoint();
	    joint.has_value() && joint->index < desc.joints.size() && modelMatrices != nullptr && desc.matrixCount == 1)
	{
		jointModel = *modelMatrices * glm::translate(glm::mat4(1.0f), joint->pivot) * desc.joints[joint->index] *
		             glm::translate(glm::mat4(1.0f), -joint->pivot);
		modelMatrices = &jointModel;
	}
	// A creature's body takes its blended skins in place of its base mesh's
	const auto skinOf = [&desc, &skins](uint32_t skinID) -> const Texture2D* {
		if (desc.morphTargets != nullptr)
		{
			const auto& blended = desc.morphTargets->skins;
			const auto found = std::ranges::find_if(blended, [skinID](const auto& skin) { return skin.first == skinID; });
			if (found != blended.end())
			{
				return found->second;
			}
		}
		return GetTexture(skinID, skins);
	};
	bool lastPreserveState = false;
	const auto& primitives = subMesh.GetPrimitives();
	for (auto it = primitives.begin(); it != primitives.end(); ++it)
	{
		const auto& prim = *it;

		const bool hasNext = std::next(it) != primitives.end();

		const Texture2D* texture = skinOf(prim.skinID);
		const Texture2D* nextTexture = !hasNext ? nullptr : skinOf(std::next(it)->skinID);

		// Primitives drawn with their own material's blending can't share render state, nor can a submesh with a texture
		// of its own
		const bool primitivePreserveState = !materialBlending && subMeshTexture == nullptr && desc.subMeshGlows.empty() &&
		                                    texture != nullptr && texture == nextTexture && (preserveState || hasNext);

		uint32_t skip = Mesh::SkipState::SkipNone;
		if (!lastPreserveState)
		{
			if (modelMatrices != nullptr && desc.matrixCount > 0)
			{
				bgfx::setTransform(modelMatrices, desc.matrixCount);
			}
			if (program->HasUniform("u_depthBias"))
			{
				const glm::vec4 u_depthBias {desc.depthBias, 0.0f, 0.0f, 0.0f};
				program->SetUniformValue("u_depthBias", &u_depthBias);
			}
			if (program->HasUniform("u_tint"))
			{
				program->SetUniformValue("u_tint", &desc.tint);
			}
			if (program->HasUniform("u_glow"))
			{
				// A control's glow takes the place of the light's colour added
				const glm::vec4 u_glow {glow != glm::vec3(0.0f) ? glow : desc.lightAdd, 0.0f};
				program->SetUniformValue("u_glow", &u_glow);
			}
			if (program->HasUniform("u_darkening"))
			{
				const glm::vec4 u_darkening {1.0f - desc.lightMultiply, 0.0f};
				program->SetUniformValue("u_darkening", &u_darkening);
			}
			if (desc.morphTargets != nullptr && program->HasUniform("u_morphWeights"))
			{
				const glm::vec4 u_morphWeights {desc.morphTargets->weights, 0.0f};
				program->SetUniformValue("u_morphWeights", &u_morphWeights);
			}
			if (desc.morphTargets != nullptr && program->HasUniform("u_vertexBlend"))
			{
				BindBlendSources(mesh, subMesh, *desc.morphTargets, *program);
			}
			if (program->HasUniform("u_uvOffset"))
			{
				const glm::vec4 u_uvOffset {desc.uvOffset, 0.0f, 0.0f};
				program->SetUniformValue("u_uvOffset", &u_uvOffset);
			}
			if (program->HasUniform("u_seaClip"))
			{
				// The sea mirrors only what stands above it
				const bool reflection =
				    desc.viewId == RenderPass::Reflection || desc.viewId == RenderPass::ReflectionTranslucent;
				const glm::vec4 u_seaClip {reflection ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
				program->SetUniformValue("u_seaClip", &u_seaClip);
			}
			if (program->HasUniform("u_snow"))
			{
				// Snow shows where the primitive writes its depth, as the game draws it over the object at the same depth
				const auto& textures = Locator::resources::value().GetTextures();
				const auto* depth = desc.snow ? SnowDepth(_snowDepth, _snowRevision) : nullptr;
				const bool snowed = depth != nullptr && prim.depthWrite && textures.Contains(snow_cover::k_TextureId.value()) &&
				                    textures.Contains(snow_cover::k_AlphaTextureId.value());
				const glm::vec4 u_snow {snowed ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
				program->SetUniformValue("u_snow", &u_snow);
				if (snowed)
				{
					program->SetTextureSampler("s_snowDepth", 11, *depth);
					program->SetTextureSampler("s_snow", 12, *textures.Handle(snow_cover::k_TextureId.value()));
					program->SetTextureSampler("s_snowAlpha", 13, *textures.Handle(snow_cover::k_AlphaTextureId.value()));
				}
				// Without snow its samplers get the program's white defaults when submitted
			}
			if (program->HasUniform("u_window"))
			{
				// Window submeshes are lit by their houses at night
				glm::vec4 u_window {subMesh.GetFlags().isWindow ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
				if (Locator::skySystem::has_value())
				{
					const auto& clock = Locator::skySystem::value().GetClock();
					u_window.y = clock.GetVisualTime();
					u_window.z = clock.IsVisualNight() ? 1.0f : 0.0f;
				}
				program->SetUniformValue("u_window", &u_window);
			}
			if (program->HasUniform("s_diffuse"))
			{
				// A primitive without a skin would otherwise sample whichever texture the draw before it left bound,
				// changing as bgfx orders the draws. The sky has none either, but binds the sky's texture for it.
				if (subMeshTexture != nullptr)
				{
					program->SetTextureSampler("s_diffuse", 0, *subMeshTexture);
				}
				else if (desc.skinTexture != nullptr && prim.skinID != 0xFFFFFFFF)
				{
					program->SetTextureSampler("s_diffuse", 0, *desc.skinTexture);
				}
				else if (texture != nullptr)
				{
					program->SetTextureSampler("s_diffuse", 0, *texture);
				}
				else if (!desc.isSky && _whiteTexture)
				{
					program->SetTextureSampler("s_diffuse", 0, *_whiteTexture);
				}
			}
			if (program == desc.lightmapProgram)
			{
				program->SetTextureSampler("s_lightmap", 3, *lightmap);
			}
			if (desc.environment != nullptr && program->HasUniform("s_environment"))
			{
				program->SetTextureSampler("s_environment", 5, *desc.environment);
			}
			if (desc.morphWithTerrain)
			{
				program->SetTextureSampler("s_heightmap", 1, heightMap);   // vs
				program->SetUniformValue("u_islandExtent", &islandExtent); // vs
			}
			if (program->HasUniform("u_landLight"))
			{
				// Objects in the world take the colour of the land's light where they stand; the sky and the temple's
				// insides have lights of their own
				const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
				const bool landLit = !desc.isSky && !desc.drawAll && !inTemple && _landLightTexture.has_value();
				const glm::vec4 u_landLight {landLit ? 1.0f : 0.0f, desc.landLightScale, desc.unlit ? 1.0f : 0.0f, 0.0f};
				program->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
				program->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
				program->SetTextureSampler("s_landColour", 8, GetLandColour());
				program->SetUniformValue("u_islandExtent", &islandExtent);
				program->SetUniformValue("u_landLight", &u_landLight);
			}
			if (program->HasUniform("u_haze"))
			{
				// The distance haze is the world's, not the sky's or the temple's
				const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
				const bool hazed = !desc.isSky && !desc.drawAll && !inTemple;
				const auto u_haze = hazed ? _haze[0] : glm::vec4(0.0f);
				program->SetUniformValue("u_haze", &u_haze);
				program->SetUniformValue("u_hazeColour", &_haze[1]);
			}
			if (program->HasUniform("u_modelLight"))
			{
				program->SetUniformValue("u_modelLight", &_modelLight);
			}
			if (program->HasUniform("u_creatureShadowInfo"))
			{
				// They fall on what stands on the land, not on the creatures, nor in the reflection
				const bool mainView = desc.viewId == RenderPass::Main || desc.viewId == RenderPass::Translucent;
				const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
				SetCreatureShadowUniforms(*program, desc.creatureShadows && mainView && !inTemple && !desc.isSky &&
				                                        !desc.drawAll && desc.morphTargets == nullptr);
			}
			if (!desc.isSky && program->HasUniform("u_skyAlphaThreshold"))
			{
				const glm::vec4 u_skyAlphaThreshold = {
				    Locator::skySystem::value().GetCurrentSkyType(),
				    prim.thresholdAlpha ? prim.alphaCutoutThreshold : 0.0f,
				    0.0f,
				    0.0f,
				};
				program->SetUniformValue("u_skyAlphaThreshold", &u_skyAlphaThreshold);
			}
		}
		else
		{
			skip |= Mesh::SkipState::SkipRenderState;
			skip |= Mesh::SkipState::SkipVertexBuffer;
		}

		{
			if (desc.instanceDesc != nullptr && (skip & Mesh::SkipState::SkipInstanceBuffer) == 0)
			{
				bgfx::setInstanceDataBuffer(toBgfx(desc.instanceDesc->GetRawHandle()), desc.instanceDesc->GetStart(),
				                            desc.instanceDesc->GetCount());
			}
			if (subMesh.GetMesh().IsIndexed() && (skip & Mesh::SkipState::SkipIndexBuffer) == 0)
			{
				subMesh.GetMesh().GetIndexBuffer().Bind(prim.indicesCount, prim.indicesOffset);
			}
			if ((skip & Mesh::SkipState::SkipVertexBuffer) == 0)
			{
				subMesh.GetMesh().GetVertexBuffer().Bind();
				if (desc.morphTargets != nullptr && _morphStreamLayouts)
				{
					BindMorphTargets(mesh, subMesh, *desc.morphTargets, *_morphStreamLayouts);
				}
			}
			if ((skip & Mesh::SkipState::SkipRenderState) == 0)
			{
				auto state = desc.state;
				if (desc.useMaterialCulling)
				{
					// L3D meshes face clockwise
					state &= ~BGFX_STATE_CULL_MASK;
					if (!prim.twoSided)
					{
						state |= desc.mirrored ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW;
					}
				}
				if (materialBlending)
				{
					using BlendMode = decltype(prim.blend);
					switch (prim.blend)
					{
					case BlendMode::Standard:
						state |= BGFX_STATE_BLEND_ALPHA;
						break;
					case BlendMode::Additive:
						state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
						break;
					case BlendMode::JustZ:
						state |= BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ZERO, BGFX_STATE_BLEND_ONE);
						break;
					case BlendMode::Disabled:
						break;
					}
					if (!prim.depthWrite)
					{
						state &= ~BGFX_STATE_WRITE_Z;
					}
				}
				bgfx::setState(state, desc.rgba);
			}

			program->Submit(static_cast<bgfx::ViewId>(viewId), desc.sortDepth,
			                primitivePreserveState ? BGFX_DISCARD_NONE : BGFX_DISCARD_ALL);
		}
		lastPreserveState = primitivePreserveState;
	}
}

void Renderer::DrawTempleText(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::temple::has_value() || !Locator::temple::value().Active())
	{
		return;
	}
	const auto& temple = Locator::temple::value();
	const auto& vertices = temple.GetText();
	const auto* texture = temple.GetTextTexture();
	if (vertices.empty() || texture == nullptr)
	{
		return;
	}
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	static_assert(sizeof(OrientedTextVertex) == (3 + 2 + 1) * sizeof(float));
	const auto count = static_cast<uint32_t>(vertices.size());
	if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, count, layout);
	std::memcpy(buffer.data, vertices.data(), count * sizeof(OrientedTextVertex));

	// The game draws the glyphs blended by their coverage, tested against the room's depth
	const auto* shader = _shaderManager->GetShader("Text3D");
	const auto model = glm::translate(glm::mat4(1.0f), temple.GetPosition());
	bgfx::setTransform(glm::value_ptr(model));
	bgfx::setVertexBuffer(0, &buffer);
	shader->SetTextureSampler("s_texture", 0, *texture);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
	               BGFX_STATE_BLEND_ALPHA);
	shader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawTemplePool(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::temple::has_value() || !Locator::temple::value().Active())
	{
		return;
	}
	const auto& temple = Locator::temple::value();
	const auto pool = entt::hashed_string("temple/interior/mainwater_l3d").value();
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!temple.IsRoomDrawn(TempleRoom::Main) || !meshes.Contains(pool))
	{
		return;
	}
	// The water's alpha swells and ebbs with the sine of the time, and the second layer's is what the first's lacks
	const float time = temple.GetPoolTime();
	const auto alpha = static_cast<int32_t>((std::sin(time) * 32.0f) + 128.0f);
	// The layers take 13 sixteenths of the temple's light, and of its colour added
	const float k_Light = 13.0f / 16.0f;
	const auto& light = temple.GetLight();
	struct Layer
	{
		glm::vec3 position;
		float yaw;
		int32_t alpha;
		glm::vec2 uvOffset;
	};
	const std::array layers = {
	    Layer {glm::vec3(0.0f), 0.0f, alpha, glm::vec2(-0.01f, 0.007f) * time},
	    Layer {glm::vec3(0.0f, 0.05f, 0.0f), glm::quarter_pi<float>(), 255 - alpha, glm::vec2(0.01f, 0.005f) * time},
	};
	// Under the water the room's reflection shows, where the floor leaves it uncovered
	{
		const auto model = glm::translate(glm::mat4(1.0f), temple.GetPosition());
		const auto* reflection = _shaderManager->GetShader("Reflection");
		L3DMeshSubmitDesc submitDesc = {};
		submitDesc.viewId = desc.viewId;
		submitDesc.program = reflection;
		// Beneath the first layer, at its height, so it leaves the depth to the layers
		submitDesc.state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA;
		submitDesc.useMaterialCulling = true;
		submitDesc.modelMatrices = &model;
		submitDesc.matrixCount = 1;
		// Each submesh's draw lets go of the textures bound for it
		const auto& mesh = *meshes.Handle(pool);
		for (uint8_t subMesh = 0; subMesh < mesh.GetNumSubMeshes(); ++subMesh)
		{
			reflection->SetTextureSampler("s_reflection", 4,
			                              Locator::oceanSystem::value().GetReflectionFramebuffer().GetColorAttachment());
			DrawMesh(mesh, submitDesc, subMesh);
		}
	}
	for (const auto& layer : layers)
	{
		const auto model = glm::translate(glm::mat4(1.0f), temple.GetPosition() + layer.position) *
		                   glm::rotate(glm::mat4(1.0f), layer.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		L3DMeshSubmitDesc submitDesc = {};
		submitDesc.viewId = desc.viewId;
		// In their colour alone, without the pool's lightmap: through it, the water is far darker than the game's
		// TODO(raffclar): confirm how the game lights an object given a colour
		submitDesc.program = _shaderManager->GetShader("Object");
		// Each primitive blended, culled and writing depth as its material says, as the room's meshes are
		submitDesc.state = k_BgfxDefaultStateInvertedZ;
		submitDesc.useMaterialBlending = true;
		submitDesc.useMaterialCulling = true;
		submitDesc.modelMatrices = &model;
		submitDesc.matrixCount = 1;
		submitDesc.uvOffset = layer.uvOffset;
		submitDesc.tint = glm::vec4(light.multiply * k_Light, static_cast<float>(layer.alpha) / 255.0f);
		submitDesc.lightAdd = light.add * k_Light;
		DrawMesh(*meshes.Handle(pool), submitDesc, std::numeric_limits<uint8_t>::max());
	}
}

void Renderer::DrawTempleMapPass() const
{
	// The view keeps its clear from one frame to the next, so it is only touched to draw the land afresh
	const auto viewId = static_cast<bgfx::ViewId>(RenderPass::TempleMap);
	if (!Locator::temple::has_value() || !Locator::temple::value().Active() || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto visit = Locator::temple::value().GetVisits();
	if (_templeMapVisit == visit)
	{
		return;
	}
	_templeMapVisit = visit;

	// 8 texels a block, as the game's pages of the land's textures give the map, over the 32 by 32 blocks there can be
	constexpr uint16_t k_Size = 256;
	if (!_templeMapFrameBuffer)
	{
		_templeMapFrameBuffer = std::make_unique<FrameBuffer>("Temple Map", k_Size, k_Size, TextureFormat::RGBA8, std::nullopt,
		                                                      static_cast<uint8_t>(1), Wrapping::ClampEdge);
	}
	_templeMapFrameBuffer->Bind(RenderPass::TempleMap);
	bgfx::setViewRect(viewId, 0, 0, k_Size, k_Size);
	bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR, 0x00000000);

	// From above, x across the texture and z down it, as the map's texture coordinates run. The terrain's shader pulls
	// vertices at sea level towards the view's origin, so the view is centred on the texture to keep that small.
	constexpr float k_Half = TempleMap::k_TextureSpan * 0.5f;
	const auto view = glm::translate(glm::mat4(1.0f), glm::vec3(-k_Half, 0.0f, -k_Half));
	const float down = bgfx::getCaps()->originBottomLeft ? 1.0f : -1.0f;
	auto projection = glm::mat4(0.0f);
	projection[0][0] = 1.0f / k_Half;
	projection[2][1] = down / k_Half;
	projection[3][2] = 0.5f;
	projection[3][3] = 1.0f;
	bgfx::setViewTransform(viewId, glm::value_ptr(view), glm::value_ptr(projection));
	bgfx::touch(viewId);

	const auto& island = Locator::terrainSystem::value();
	const auto* terrainShader = _shaderManager->GetShader("Terrain");
	const auto islandExtent = glm::vec4(island.GetExtent().minimum, island.GetExtent().maximum);
	auto smallBump = Locator::resources::value().GetTextures().Handle(LandIslandInterface::k_SmallBumpTextureId);
	// The land's own textures, bumped, with the footprints and the objects' shadows on them
	const glm::vec4 u_skyAndBump = {0.0f, 0.0f, 0.0f, 1.0f};
	auto u_objectShadows = glm::vec4(0.0f);
	if (_objectShadowFrameBuffer)
	{
		uint16_t width = 0;
		uint16_t height = 0;
		_objectShadowFrameBuffer->GetSize(width, height);
		u_objectShadows = glm::vec4(ObjectShadows::k_MaxDarkness, 1.0f / static_cast<float>(std::max<uint16_t>(width, 1)),
		                            1.0f / static_cast<float>(std::max<uint16_t>(height, 1)), 0.0f);
	}
	const auto noHandShadow = glm::mat4(0.0f);
	const auto noHand = glm::vec4(0.0f);
	terrainShader->SetTextureSampler("s0_blockTextures", 0, island.GetBlockTextures());
	terrainShader->SetTextureSampler("s9_landLuminosity", 9, GetLandLuminosity());
	terrainShader->SetTextureSampler("s10_landColour", 10, GetLandColour());
	terrainShader->SetTextureSampler("s2_smallBump", 2, *smallBump);
	terrainShader->SetTextureSampler("s3_footprints", 3, island.GetFootprintFramebuffer().GetColorAttachment());
	terrainShader->SetTextureSampler("s4_handShadow", 4, _handShadowFrameBuffer->GetColorAttachment());
	terrainShader->SetTextureSampler("s5_objectShadows", 5,
	                                 _objectShadowFrameBuffer ? _objectShadowFrameBuffer->GetColorAttachment()
	                                                          : island.GetFootprintFramebuffer().GetColorAttachment());
	terrainShader->SetTextureSampler("s7_landLight", 7, GetLandLightTexture());
	// The temple's map of the land has no haze
	const auto noHaze = glm::vec4(0.0f);
	terrainShader->SetUniformValue("u_haze", &noHaze);
	terrainShader->SetUniformValue("u_hazeColour", &noHaze);
	terrainShader->SetUniformValue("u_skyAndBump", &u_skyAndBump);
	terrainShader->SetUniformValue("u_objectShadows", &u_objectShadows);
	terrainShader->SetUniformValue("u_islandExtent", &islandExtent);
	terrainShader->SetUniformValue("u_handShadowMatrix", &noHandShadow);
	terrainShader->SetUniformValue("u_handShadow", &noHand);
	for (size_t i = 0; const auto& block : island.GetBlocks())
	{
		const glm::vec4 mapPositionAndSize = glm::vec4(block.GetMapPosition(), 160.0f, 160.0f);
		terrainShader->SetUniformValue("u_blockPositionAndSize", &mapPositionAndSize);
		const glm::vec4 u_block {static_cast<float>(i++), 0.0f, 0.0f, 0.0f};
		terrainShader->SetUniformValue("u_block", &u_block);
		block.BindVertices();
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		// The textures stay bound from one block to the next
		terrainShader->Submit(viewId, 0,
		                      BGFX_DISCARD_INSTANCE_DATA | BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_TRANSFORM |
		                          BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_STATE);
	}
	_shaderManager->DiscardBindings();
}

void Renderer::DrawTempleUnderside(const DrawSceneDesc& desc) const
{
	// The rooms' meshes don't quite meet everywhere: the sills of the main room's doorways stand a hundredth of a unit off
	// their doors' frames, and their edges have vertices of others part way along them. The game draws the sky
	// behind the temple, so it shows through the cracks there too, as specks of the outside. Looking down through them,
	// this shows black instead. The creature's room goes deepest, to 70.5 below the temple.
	constexpr float k_Depth = -75.0f;
	constexpr float k_Extent = 10000.0f;
	if (desc.viewId != RenderPass::Main || !_whiteTexture || !Locator::temple::has_value() ||
	    !Locator::temple::value().Active())
	{
		return;
	}
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	constexpr uint32_t k_Black = 0xFF000000;
	const std::array<OrientedTextVertex, 6> vertices {{
	    {{-k_Extent, k_Depth, -k_Extent}, {0.0f, 0.0f}, k_Black},
	    {{k_Extent, k_Depth, -k_Extent}, {1.0f, 0.0f}, k_Black},
	    {{k_Extent, k_Depth, k_Extent}, {1.0f, 1.0f}, k_Black},
	    {{-k_Extent, k_Depth, -k_Extent}, {0.0f, 0.0f}, k_Black},
	    {{k_Extent, k_Depth, k_Extent}, {1.0f, 1.0f}, k_Black},
	    {{-k_Extent, k_Depth, k_Extent}, {0.0f, 1.0f}, k_Black},
	}};
	const auto count = static_cast<uint32_t>(vertices.size());
	if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, count, layout);
	std::memcpy(buffer.data, vertices.data(), count * sizeof(OrientedTextVertex));

	const auto* shader = _shaderManager->GetShader("Text3D");
	const auto model = glm::translate(glm::mat4(1.0f), Locator::temple::value().GetPosition());
	bgfx::setTransform(glm::value_ptr(model));
	bgfx::setVertexBuffer(0, &buffer);
	shader->SetTextureSampler("s_texture", 0, *_whiteTexture);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER |
	               BGFX_STATE_MSAA);
	shader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawTempleMap(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !_templeMapFrameBuffer || !Locator::temple::has_value() ||
	    !Locator::temple::value().Active())
	{
		return;
	}
	const auto& temple = Locator::temple::value();
	const auto& vertices = temple.GetMap();
	if (vertices.empty())
	{
		return;
	}
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto count = static_cast<uint32_t>(vertices.size());
	if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, count, layout);
	std::memcpy(buffer.data, vertices.data(), count * sizeof(OrientedTextVertex));

	// The land's texture by the vertices' colours, blended by their alpha
	const auto* shader = _shaderManager->GetShader("Text3D");
	const auto model = glm::translate(glm::mat4(1.0f), temple.GetPosition());
	bgfx::setTransform(glm::value_ptr(model));
	bgfx::setVertexBuffer(0, &buffer);
	shader->SetTextureSampler("s_texture", 0, _templeMapFrameBuffer->GetColorAttachment());
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER |
	               BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA);
	shader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawTempleMapMarkers(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::temple::has_value() || !Locator::temple::value().Active())
	{
		return;
	}
	const auto& temple = Locator::temple::value();
	const auto& markers = temple.GetMapMarkers();
	if (markers.empty())
	{
		return;
	}
	const auto origin = temple.GetPosition();

	// First a glow under each marker, the room's light glow drawn over everything: 1.3 across, a tenth above the marker
	const auto& textures = Locator::resources::value().GetTextures();
	const auto atmos = entt::hashed_string("raw/ATMOS");
	const auto atmosAlpha = entt::hashed_string("raw/ATMOSA");
	if (textures.Contains(atmos.value()) && textures.Contains(atmosAlpha.value()))
	{
		const auto* spriteShader = _shaderManager->GetShader("Sprite");
		constexpr float k_GlowSize = 1.3f;
		constexpr glm::vec4 k_GlowColour {0x61 / 255.0f, 0x6E / 255.0f, 0x7C / 255.0f, 1.0f};
		// The glow is frame 22 of the atmosphere texture's 8 by 8 frames, facing the camera and adding to what is behind
		const glm::vec4 u_sampleRect {1.0f / 8.0f, 1.0f / 8.0f, 6.0f / 8.0f, 2.0f / 8.0f};
		const glm::vec4 u_spriteParams {1.0f, 1.0f, 1.0f, 0.0f};
		for (const auto& marker : markers)
		{
			const auto model = glm::scale(
			    glm::translate(glm::mat4(1.0f), origin + marker.position + glm::vec3(0.0f, 0.1f, 0.0f)), glm::vec3(k_GlowSize));
			bgfx::setTransform(glm::value_ptr(model));
			spriteShader->SetUniformValue("u_sampleRect", glm::value_ptr(u_sampleRect));
			spriteShader->SetUniformValue("u_spriteParams", glm::value_ptr(u_spriteParams));
			spriteShader->SetUniformValue("u_tint", glm::value_ptr(k_GlowColour));
			spriteShader->SetTextureSampler("s_diffuse", 0, *textures.Handle(atmos));
			spriteShader->SetTextureSampler("s_alpha", 1, *textures.Handle(atmosAlpha));
			_plane->GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
			               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE));
			spriteShader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
		}
	}

	// Then the markers, a twentieth of their size, turning, in their colours
	constexpr std::array<entt::id_type, 3> k_Icons = {
	    entt::hashed_string("temple/icons/I_citadel_on_map"),
	    entt::hashed_string("temple/icons/I_creature_on_map"),
	    entt::hashed_string("temple/icons/I_challenge_on_map"),
	};
	constexpr float k_MarkerScale = 0.05f;
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto turn = glm::rotate(glm::mat4(1.0f), temple.GetMapMarkerTurn(), glm::vec3(0.0f, 1.0f, 0.0f));
	for (const auto& marker : markers)
	{
		const auto icon = k_Icons.at(static_cast<size_t>(marker.kind));
		if (!meshes.Contains(icon))
		{
			continue;
		}
		// The game puts the markers in the temple's light
		const auto model = glm::translate(glm::mat4(1.0f), origin + marker.position) * turn *
		                   glm::scale(glm::mat4(1.0f), glm::vec3(k_MarkerScale));
		L3DMeshSubmitDesc submitDesc = {};
		submitDesc.viewId = desc.viewId;
		submitDesc.program = _shaderManager->GetShader("Object");
		submitDesc.state = k_BgfxDefaultStateInvertedZ;
		submitDesc.modelMatrices = &model;
		submitDesc.matrixCount = 1;
		submitDesc.tint = glm::vec4(temple.GetLight().Colour(glm::vec3(marker.colour) / 255.0f), 1.0f);
		submitDesc.lightAdd = temple.GetLight().add;
		DrawMesh(*meshes.Handle(icon), submitDesc, std::numeric_limits<uint8_t>::max());
	}
}

void Renderer::DrawCaveTrophies(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::temple::has_value() || !Locator::temple::value().Active())
	{
		return;
	}
	const auto& trophies = Locator::temple::value().GetCaveTrophies();
	if (trophies.empty())
	{
		return;
	}
	// The game gives every material of the icons the one texture, with the alpha read from beside it
	if (!_iconsLoaded)
	{
		_iconsLoaded = true;
		constexpr uint16_t k_Size = 256;
		constexpr size_t k_Pixels = static_cast<size_t>(k_Size) * k_Size;
		auto& fileSystem = Locator::filesystem::value();
		const auto colourPath = fileSystem.GetPath<filesystem::Path::Textures>() / "icons.raw";
		const auto alphaPath = fileSystem.GetPath<filesystem::Path::Textures>() / "iconsa.raw";
		if (fileSystem.Exists(colourPath) && fileSystem.Exists(alphaPath))
		{
			const auto colour = fileSystem.ReadAll(colourPath);
			const auto alpha = fileSystem.ReadAll(alphaPath);
			if (colour.size() == k_Pixels * 3 && alpha.size() == k_Pixels)
			{
				const auto* memory = bgfx::alloc(static_cast<uint32_t>(k_Pixels * 4));
				for (size_t i = 0; i < k_Pixels; ++i)
				{
					memory->data[(i * 4) + 0] = colour[(i * 3) + 0];
					memory->data[(i * 4) + 1] = colour[(i * 3) + 1];
					memory->data[(i * 4) + 2] = colour[(i * 3) + 2];
					memory->data[(i * 4) + 3] = alpha[i];
				}
				_iconsTexture =
				    fromBgfx(bgfx::createTexture2D(k_Size, k_Size, false, 1, bgfx::TextureFormat::RGBA8, 0, memory));
				bgfx::setName(toBgfx(*_iconsTexture), "Icons");
			}
		}
	}
	if (!_iconsTexture)
	{
		return;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& textures = Locator::resources::value().GetTextures();
	const auto envmap = entt::hashed_string("raw/envmap").value();
	const auto* environment = textures.Contains(envmap) ? &*textures.Handle(envmap) : nullptr;
	for (const auto& trophy : trophies)
	{
		if (!meshes.Contains(trophy.mesh))
		{
			continue;
		}
		const auto colour = glm::vec3((trophy.colour >> 16) & 0xFF, (trophy.colour >> 8) & 0xFF, trophy.colour & 0xFF);
		L3DMeshSubmitDesc submitDesc = {};
		submitDesc.viewId = desc.viewId;
		// The creature's room draws the belts and the medals past wood environment-mapped, adding the game's first
		// environment map, envmap.raw
		const bool environmentMapped = trophy.environmentMapped && environment != nullptr;
		submitDesc.program = _shaderManager->GetShader(environmentMapped ? "ObjectEnvironment" : "Object");
		submitDesc.environment = environmentMapped ? environment : nullptr;
		submitDesc.state = k_BgfxDefaultStateInvertedZ;
		submitDesc.modelMatrices = &trophy.model;
		// Their materials are two sided
		submitDesc.useMaterialCulling = true;
		submitDesc.matrixCount = 1;
		submitDesc.skinTexture = &*_iconsTexture;
		submitDesc.useMaterialBlending = true;
		// The colour multiplies what lights them: the medals come out the mid grey of the game's at 0x80. The game
		// puts it in the temple's light.
		const auto& light = Locator::temple::value().GetLight();
		submitDesc.tint = glm::vec4(light.Colour(colour / 255.0f), 0.0f);
		submitDesc.lightAdd = light.add;
		DrawMesh(*meshes.Handle(trophy.mesh), submitDesc, std::numeric_limits<uint8_t>::max());
	}
}

void Renderer::DrawCreatureEyes(const DrawSceneDesc& desc, entt::entity entity, const L3DMeshSubmitDesc& bodyDesc) const
{
	using ecs::components::CreatureEyes;
	const auto* eyes = desc.entities.TryGet<const CreatureEyes>(entity);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (eyes == nullptr || !meshes.Contains(CreatureEyes::k_EyeballMeshId) || !meshes.Contains(CreatureEyes::k_EyelidMeshId))
	{
		return;
	}
	const auto eyeball = meshes.Handle(CreatureEyes::k_EyeballMeshId);
	const auto eyelid = meshes.Handle(CreatureEyes::k_EyelidMeshId);

	// Each eye is drawn on its own, in the creature's light: its eyeball, then its eyelid in the colour of the skin under it
	L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = bodyDesc.viewId;
	submitDesc.program = _shaderManager->GetShader("Object");
	submitDesc.state = bodyDesc.state;
	submitDesc.matrixCount = 1;
	submitDesc.lightMultiply = bodyDesc.lightMultiply;
	submitDesc.lightAdd = bodyDesc.lightAdd;
	submitDesc.landLightScale = bodyDesc.landLightScale;
	submitDesc.mirrored = bodyDesc.mirrored;
	submitDesc.creatureShadows = false;
	submitDesc.sortDepth = bodyDesc.sortDepth;
	const auto tint = glm::vec4(bodyDesc.tint.r, bodyDesc.tint.g, bodyDesc.tint.b, 0.0f);
	for (const auto& eye : eyes->drawn)
	{
		if (eye.eyeball.has_value())
		{
			submitDesc.modelMatrices = &*eye.eyeball;
			submitDesc.tint = tint;
			DrawMesh(*eyeball, submitDesc, std::numeric_limits<uint8_t>::max());
		}
		if (eye.eyelid.has_value())
		{
			submitDesc.modelMatrices = &*eye.eyelid;
			submitDesc.tint = tint * glm::vec4(eyes->lidColour, 1.0f);
			DrawMesh(*eyelid, submitDesc, std::numeric_limits<uint8_t>::max());
		}
	}
}

void Renderer::DrawCreatureHair(const DrawSceneDesc& desc, entt::entity entity) const
{
	using ecs::components::CreatureHair;
	const auto* hair = desc.entities.TryGet<const CreatureHair>(entity);
	const auto& textures = Locator::resources::value().GetTextures();
	if (hair == nullptr || !Locator::creatureHairSystem::value().IsShown())
	{
		return;
	}
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	static const auto k_Layout = [] {
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
		    .end();
		return layout;
	}();
	const bool hasTexture = textures.Contains(CreatureHair::k_TextureId) && textures.Contains(CreatureHair::k_AlphaTextureId);
	const auto* transform = desc.entities.TryGet<const ecs::components::Transform>(entity);
	if (transform == nullptr)
	{
		return;
	}
	const auto eye = desc.camera->GetOrigin();
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentPassOf(desc.viewId));
	const auto* program = _shaderManager->GetShader("CreatureHair");
	// The hair takes the land's light and colour under the creature, and the haze at it, as its body does
	const auto extent = Locator::terrainSystem::value().GetExtent();
	const auto islandExtent = glm::vec4(extent.minimum, extent.maximum);
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	const bool landLit = !inTemple && _landLightTexture.has_value();
	const glm::vec4 u_landLight {landLit ? 1.0f : 0.0f, CreatureLandLightScale(desc.viewId), 0.0f, 0.0f};
	const glm::vec4 u_hairOrigin {transform->position, 0.0f};
	const auto u_haze = inTemple ? glm::vec4(0.0f) : _haze[0];
	std::vector<creature_hair::RibbonVertex> ribbon;
	for (const auto& group : hair->groups)
	{
		// Every strand of a group in one draw, each a strip of quads between its points
		uint32_t vertexCount = 0;
		uint32_t indexCount = 0;
		for (const auto& strand : group.strands)
		{
			if (strand.positions.size() >= 2)
			{
				vertexCount += static_cast<uint32_t>(strand.positions.size() * 2);
				indexCount += static_cast<uint32_t>((strand.positions.size() - 1) * 6);
			}
		}
		const bool textured = group.textured && hasTexture;
		if (vertexCount == 0 || vertexCount > std::numeric_limits<uint16_t>::max() ||
		    (!textured && !_whiteTexture.has_value()) ||
		    bgfx::getAvailTransientVertexBuffer(vertexCount, k_Layout) < vertexCount ||
		    bgfx::getAvailTransientIndexBuffer(indexCount) < indexCount)
		{
			continue;
		}
		bgfx::TransientVertexBuffer vertexBuffer;
		bgfx::TransientIndexBuffer indexBuffer;
		bgfx::allocTransientVertexBuffer(&vertexBuffer, vertexCount, k_Layout);
		bgfx::allocTransientIndexBuffer(&indexBuffer, indexCount);
		const auto vertices = std::span(reinterpret_cast<Vertex*>(vertexBuffer.data), vertexCount);
		const auto indices = std::span(reinterpret_cast<uint16_t*>(indexBuffer.data), indexCount);
		// The texture multiplies the strands' colour, which the shader lights
		const auto colour = glm::clamp(group.colour, 0, 255);
		const auto abgr = 0xFF000000u | (static_cast<uint32_t>(colour.b) << 16u) | (static_cast<uint32_t>(colour.g) << 8u) |
		                  static_cast<uint32_t>(colour.r);
		size_t vertex = 0;
		size_t index = 0;
		glm::vec3 middle(0.0f);
		for (const auto& strand : group.strands)
		{
			if (strand.positions.size() < 2)
			{
				continue;
			}
			ribbon.resize(strand.positions.size() * 2);
			creature_hair::BuildRibbon(strand.positions, eye, group.halfWidth, ribbon);
			const auto first = static_cast<uint16_t>(vertex);
			for (const auto& corner : ribbon)
			{
				vertices[vertex++] = {corner.position, corner.uv, abgr};
			}
			for (uint16_t i = 0; i + 1 < static_cast<uint16_t>(strand.positions.size()); ++i)
			{
				const auto a = static_cast<uint16_t>(first + (i * 2));
				for (const auto offset : {0, 1, 2, 2, 1, 3})
				{
					indices[index++] = static_cast<uint16_t>(a + offset);
				}
			}
			middle += strand.positions.front();
		}
		middle /= static_cast<float>(group.strands.size());
		// Without the texture a strand is a solid ribbon in its colour
		if (textured)
		{
			program->SetTextureSampler("s_diffuse", 0, *textures.Handle(CreatureHair::k_TextureId));
			program->SetTextureSampler("s_alpha", 1, *textures.Handle(CreatureHair::k_AlphaTextureId));
		}
		else
		{
			program->SetTextureSampler("s_diffuse", 0, *_whiteTexture);
			program->SetTextureSampler("s_alpha", 1, *_whiteTexture);
		}
		program->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
		program->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
		program->SetTextureSampler("s_landColour", 8, GetLandColour());
		program->SetUniformValue("u_islandExtent", &islandExtent);
		program->SetUniformValue("u_landLight", &u_landLight);
		program->SetUniformValue("u_hairOrigin", &u_hairOrigin);
		program->SetUniformValue("u_haze", &u_haze);
		program->SetUniformValue("u_hazeColour", &_haze[1]);
		bgfx::setVertexBuffer(0, &vertexBuffer);
		bgfx::setIndexBuffer(&indexBuffer);
		// Blended over what is behind by the texture's alpha, both sides, tested against depth but leaving none
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
		               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA));
		program->Submit(viewId, zsort::Depth(middle, eye));
	}
}

void Renderer::DrawMistDomes(const DrawSceneDesc& desc) const
{
	if (desc.viewId == RenderPass::Reflection || !Locator::temple::has_value() || !Locator::temple::value().Active())
	{
		return;
	}
	using namespace ecs::components;
	const auto& temple = Locator::temple::value();
	const auto* shader = _shaderManager->GetShader("Beam");
	auto& registry = Locator::entitiesRegistry::value();
	const auto& textures = Locator::resources::value().GetTextures();
	const auto smoke = entt::hashed_string("raw/smoke");
	const auto smokeAlpha = entt::hashed_string("raw/smokea");
	if (!textures.Contains(smoke.value()) || !textures.Contains(smokeAlpha.value()))
	{
		return;
	}

	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	// The smoke's material: blended by alpha, without writing depth, from both sides
	constexpr uint64_t k_State = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
	                             BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_INV_SRC_ALPHA);
	// It turns to the camera as the camera is turned
	const auto facing = glm::mat4(glm::mat3(Locator::camera::value().GetRotationMatrix()));

	registry.Each<const MistDome, const Transform, const TempleInteriorPart>(
	    [&](const MistDome& mist, const Transform& transform, const TempleInteriorPart& part) {
		    if (mist.dome == nullptr || !temple.IsRoomDrawn(part.room))
		    {
			    return;
		    }
		    const auto vertexCount = static_cast<uint32_t>(mist.dome->vertices.size());
		    const auto indexCount = static_cast<uint32_t>(mist.dome->indices.size());
		    if (vertexCount == 0 || indexCount == 0 || bgfx::getAvailTransientVertexBuffer(vertexCount, layout) < vertexCount ||
		        bgfx::getAvailTransientIndexBuffer(indexCount) < indexCount)
		    {
			    return;
		    }
		    bgfx::TransientVertexBuffer vertices;
		    bgfx::TransientIndexBuffer indices;
		    bgfx::allocTransientVertexBuffer(&vertices, vertexCount, layout);
		    bgfx::allocTransientIndexBuffer(&indices, indexCount);
		    auto* out = reinterpret_cast<BeamVertex*>(vertices.data);
		    const auto colour = glm::u8vec4(glm::clamp(mist.colour, 0.0f, 1.0f) * 255.0f);
		    for (uint32_t i = 0; i < vertexCount; ++i)
		    {
			    out[i] = mist.dome->vertices[i];
			    out[i].colour = colour;
			    out[i].uv += mist.uvOffset;
		    }
		    std::memcpy(indices.data, mist.dome->indices.data(), indexCount * sizeof(uint16_t));

		    const glm::vec4 u_beamParams {1.0f, 0.0f, 0.0f, 0.0f};
		    shader->SetTextureSampler("s_diffuse", 0, *textures.Handle(smoke));
		    shader->SetTextureSampler("s_alpha", 1, *textures.Handle(smokeAlpha));
		    shader->SetUniformValue("u_beamParams", &u_beamParams);
		    const auto model = glm::translate(glm::mat4(1.0f), transform.position) * facing * glm::scale(transform.scale);
		    bgfx::setTransform(glm::value_ptr(model));
		    bgfx::setVertexBuffer(0, &vertices);
		    bgfx::setIndexBuffer(&indices);
		    bgfx::setState(k_State);
		    shader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
	    });
}

void Renderer::DrawLightBeams(const DrawSceneDesc& desc) const
{
	if (desc.viewId == RenderPass::Reflection || !Locator::temple::has_value() || !Locator::temple::value().Active())
	{
		return;
	}
	using namespace ecs::components;
	const auto& temple = Locator::temple::value();
	const auto inDrawnRoom = [&temple](TempleRoom room) { return temple.IsRoomDrawn(room); };
	const auto* beamShader = _shaderManager->GetShader("Beam");
	auto& registry = Locator::entitiesRegistry::value();
	auto& resources = Locator::resources::value();

	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	static_assert(sizeof(BeamVertex) == 6 * sizeof(float));

	// Added by alpha, without writing depth, from both sides
	constexpr uint64_t k_State = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
	                             BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);
	const auto submit = [&](const BeamMesh& mesh, const glm::mat4& model, const Texture2D& texture, const Texture2D* alpha) {
		const auto vertexCount = static_cast<uint32_t>(mesh.vertices.size());
		const auto indexCount = static_cast<uint32_t>(mesh.indices.size());
		if (vertexCount == 0 || indexCount == 0 || bgfx::getAvailTransientVertexBuffer(vertexCount, layout) < vertexCount ||
		    bgfx::getAvailTransientIndexBuffer(indexCount) < indexCount)
		{
			return;
		}
		bgfx::TransientVertexBuffer vertices;
		bgfx::TransientIndexBuffer indices;
		bgfx::allocTransientVertexBuffer(&vertices, vertexCount, layout);
		bgfx::allocTransientIndexBuffer(&indices, indexCount);
		std::memcpy(vertices.data, mesh.vertices.data(), vertexCount * sizeof(BeamVertex));
		std::memcpy(indices.data, mesh.indices.data(), indexCount * sizeof(uint16_t));

		const glm::vec4 u_beamParams {alpha != nullptr ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
		beamShader->SetTextureSampler("s_diffuse", 0, texture);
		beamShader->SetTextureSampler("s_alpha", 1, alpha != nullptr ? *alpha : texture);
		beamShader->SetUniformValue("u_beamParams", &u_beamParams);
		bgfx::setTransform(glm::value_ptr(model));
		bgfx::setVertexBuffer(0, &vertices);
		bgfx::setIndexBuffer(&indices);
		bgfx::setState(k_State);
		beamShader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
	};

	// The spot lights' cones, with the atmosphere texture. Each room's lights drift on together, a step for each cone
	// drawn, so a room of more cones drifts faster.
	std::map<TempleRoom, uint32_t> coneCounts;
	registry.Each<const LightBeam, const TempleInteriorPart>(
	    [&coneCounts](const LightBeam& /*unused*/, const TempleInteriorPart& part) { coneCounts[part.room]++; });
	BeamMesh cones;
	registry.Each<const LightBeam, const TempleInteriorPart>(
	    [&cones, &coneCounts, &inDrawnRoom, &desc](const LightBeam& beam, const TempleInteriorPart& part) {
		    if (inDrawnRoom(part.room))
		    {
			    const float seconds = static_cast<float>(desc.time) * 0.001f;
			    AppendCone(beam.cone, seconds * static_cast<float>(coneCounts[part.room]) * 0.1f, cones);
		    }
	    });
	const auto atmos = resources.GetTextures().Handle(entt::hashed_string("raw/ATMOS"));
	const auto atmosAlpha = resources.GetTextures().Handle(entt::hashed_string("raw/ATMOSA"));
	if (atmos && atmosAlpha)
	{
		submit(cones, glm::mat4(1.0f), *atmos, &*atmosAlpha);
	}

	// The light the rooms' windows shed, with the windows' textures
	registry.Each<const ecs::components::Mesh, const Transform, const TempleInteriorPart>(
	    [&](const ecs::components::Mesh& mesh, const Transform& transform, const TempleInteriorPart& part) {
		    if (part.mesh != TempleInteriorMesh::Room || !inDrawnRoom(part.room))
		    {
			    return;
		    }
		    const auto l3dMesh = resources.GetMeshes().Handle(mesh.id);
		    const auto model = glm::translate(transform.position) * glm::mat4(transform.rotation) * glm::scale(transform.scale);
		    for (const auto& volumeLight : l3dMesh->GetVolumeLights())
		    {
			    if (const auto* texture = GetTexture(volumeLight.skinID, l3dMesh->GetSkins()); texture != nullptr)
			    {
				    submit(volumeLight.mesh, model, *texture, nullptr);
			    }
		    }
	    });
}

void Renderer::DrawMesh(const graphics::L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const noexcept
{
	if (mesh.GetNumSubMeshes() == 0)
	{
		SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "Mesh {} has no submeshes to draw", mesh.GetDebugName());
		return;
	}

	const auto& subMeshes = mesh.GetSubMeshes();

	if (subMeshIndex != std::numeric_limits<uint8_t>::max())
	{
		if (subMeshIndex >= mesh.GetNumSubMeshes())
		{
			SPDLOG_LOGGER_WARN(spdlog::get("graphics"), "tried to draw submesh out of range ({}/{})", subMeshIndex,
			                   mesh.GetNumSubMeshes());
		}

		DrawSubMesh(mesh, *subMeshes[subMeshIndex], desc, false);
		return;
	}

	// The state carries on from one submesh to the next, up to the last drawn
	const auto isDrawn = [&desc](uint32_t index) {
		return std::ranges::find(desc.hiddenSubMeshes, index) == desc.hiddenSubMeshes.end();
	};
	auto lastDrawn = static_cast<uint32_t>(subMeshes.size());
	for (auto index = static_cast<uint32_t>(subMeshes.size()); index > 0; --index)
	{
		if (isDrawn(index - 1))
		{
			lastDrawn = index - 1;
			break;
		}
	}
	for (auto it = subMeshes.begin(); it != subMeshes.end(); ++it)
	{
		const L3DSubMesh& subMesh = **it;
		const auto index = static_cast<uint32_t>(std::distance(subMeshes.begin(), it));
		if (!isDrawn(index))
		{
			continue;
		}
		const auto found = std::ranges::find(desc.subMeshTextures, index, &std::pair<uint32_t, TextureHandle>::first);
		const auto glow = std::ranges::find(desc.subMeshGlows, index, &std::pair<uint32_t, glm::vec3>::first);
		DrawSubMesh(mesh, subMesh, desc, index != lastDrawn, found != desc.subMeshTextures.end() ? &found->second : nullptr,
		            glow != desc.subMeshGlows.end() ? glow->second : glm::vec3(0.0f));
	}
}

void Renderer::DrawFootprintPass(const DrawSceneDesc& drawDesc) const
{
	const auto viewId = graphics::RenderPass::Footprint;
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::FootprintPass);
	if (drawDesc.drawIsland)
	{
		const auto& island = Locator::terrainSystem::value();
		island.GetFootprintFramebuffer().Bind(viewId);
		// The island's own size, as each land loaded has its own
		uint16_t width = 0;
		uint16_t height = 0;
		island.GetFootprintFramebuffer().GetSize(width, height);
		bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, width, height);
		bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_COLOR, 0x00000000);

		// This dummy draw call is here to make sure that view is cleared if no
		// other draw calls are submitted to view
		bgfx::touch(static_cast<bgfx::ViewId>(viewId));

		// _shaderManager->SetCamera(viewId, *drawDesc.camera); // TODO

		auto view = island.GetOrthoView();
		auto proj = island.GetOrthoProj();
		bgfx::setViewTransform(static_cast<bgfx::ViewId>(viewId), &view, &proj);

		const auto& meshManager = Locator::resources::value().GetMeshes();
		const auto& renderCtx = Locator::rendereringSystem::value().GetContext();
		const auto* footprintShaderInstanced = _shaderManager->GetShader("FootprintInstanced");
		for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
		{
			auto mesh = meshManager.Handle(meshId);
			if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
			{
				continue;
			}
			const auto& footprint = mesh->GetFootprints()[0];
			footprintShaderInstanced->SetTextureSampler("s_footprint", 0, *footprint.texture);
			footprint.mesh->GetVertexBuffer().Bind();
			bgfx::setInstanceDataBuffer(toBgfx(renderCtx.instanceUniformBuffer), placers.offset, placers.count);
			const uint64_t state = 0u                       //
			                       | BGFX_STATE_WRITE_RGB   //
			                       | BGFX_STATE_WRITE_A     //
			                       | BGFX_STATE_BLEND_ALPHA //
			                       | BGFX_STATE_CULL_CW     //
			                       | BGFX_STATE_MSAA;
			bgfx::setState(state);
			footprintShaderInstanced->Submit(static_cast<bgfx::ViewId>(viewId));
		}

		for (const auto& [meshId, placers] : renderCtx.treeInstancedDrawDescs)
		{
			auto mesh = meshManager.Handle(meshId);
			if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
			{
				continue;
			}
			const auto& footprint = mesh->GetFootprints()[0];
			footprintShaderInstanced->SetTextureSampler("s_footprint", 0, *footprint.texture);
			footprint.mesh->GetVertexBuffer().Bind();
			bgfx::setInstanceDataBuffer(toBgfx(renderCtx.treeInstanceUniformBuffer), placers.offset, placers.count);
			const uint64_t state = 0u                       //
			                       | BGFX_STATE_WRITE_RGB   //
			                       | BGFX_STATE_WRITE_A     //
			                       | BGFX_STATE_BLEND_ALPHA //
			                       | BGFX_STATE_CULL_CW     //
			                       | BGFX_STATE_MSAA;
			bgfx::setState(state);
			footprintShaderInstanced->Submit(static_cast<bgfx::ViewId>(viewId));
		}

		// The rivers' beds are laid after the land's other footprints, blended into its colour like them
		DrawStreamFootprints(viewId, ecs::components::StreamSegment::k_BedMeshId);
	}
}

void Renderer::DrawStreamFootprints(RenderPass viewId, entt::id_type meshId) const
{
	const auto& segments = Locator::rendereringSystem::value().GetContext().streamSegments;
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (segments.empty() || !meshes.Contains(meshId))
	{
		return;
	}
	const auto mesh = meshes.Handle(meshId);
	if (mesh->GetFootprints().empty())
	{
		return;
	}
	const auto count = static_cast<uint32_t>(segments.size());
	constexpr uint16_t k_Stride = sizeof(glm::mat4);
	if (bgfx::getAvailInstanceDataBuffer(count, k_Stride) < count)
	{
		return;
	}
	bgfx::InstanceDataBuffer instances;
	bgfx::allocInstanceDataBuffer(&instances, count, k_Stride);
	std::memcpy(instances.data, segments.data(), segments.size() * sizeof(glm::mat4));

	const auto& footprint = mesh->GetFootprints()[0];
	const bool channel = meshId == ecs::components::StreamSegment::k_ChannelMeshId;
	const auto* program = _shaderManager->GetShader(channel ? "LandAlphaInstanced" : "FootprintInstanced");
	program->SetTextureSampler("s_footprint", 0, *footprint.texture);
	if (channel)
	{
		const auto size = footprint.texture->GetResolution();
		const glm::vec4 u_footprintSize {size.x, size.y, 0.0f, 0.0f};
		program->SetUniformValue("u_footprintSize", &u_footprintSize);
	}
	footprint.mesh->GetVertexBuffer().Bind();
	bgfx::setInstanceDataBuffer(&instances);
	// A channel leaves the land's alpha at the lower of the two
	const uint64_t state = channel ? BGFX_STATE_WRITE_R | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) |
	                                     BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MIN)
	                               : BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA;
	bgfx::setState(state);
	program->Submit(static_cast<bgfx::ViewId>(viewId));
}

void Renderer::SetSeaUniforms(const ShaderProgram& waterShader, const Camera& camera) const
{
	const auto& config = Locator::config::value();
	const float waterTiling = detail_level::WaterTiling(config.detailLevel);
	const bool still = waterTiling == 0.0f;

	glm::vec4 u_seaParams {still ? sea_rows::k_StillPeriod : sea_rows::Period(waterTiling), 0.0f, 0.0f, 0.0f};
	glm::vec4 u_seaRows {0.0f};
	glm::vec4 u_seaMode {still ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
	if (!still)
	{
		// The rows ripple along the camera's view across the ground, 0.9 units at most
		const auto forward = camera.GetForward();
		const glm::vec2 forwardAlongGround {forward.x, forward.z};
		if (forwardAlongGround != glm::vec2(0.0f))
		{
			const auto ripple = 0.9f * glm::normalize(forwardAlongGround);
			u_seaParams.z = ripple.x;
			u_seaParams.w = ripple.y;
		}
		const auto* stats = bgfx::getStats();
		const glm::vec2 viewport {stats->width, stats->height};
		if (const auto range = sea_rows::ComputeScreenRange(camera.GetViewProjectionMatrix(), viewport, config.cameraNearClip))
		{
			// The ripple steps on with each frame the game's time moves on in
			if (Locator::time::value().GetFrameGameTime().count() != 0)
			{
				_seaRippleStep = (_seaRippleStep + 1) & 15;
			}
			const auto rows = sea_rows::MakeRows(*range);
			u_seaRows = {static_cast<float>(rows.first), static_cast<float>(rows.count), rows.inverseDepth, rows.inverseStep};
			u_seaMode.y = rows.softTop ? 1.0f : 0.0f;
		}
		else
		{
			// No rows: only what lies under the sea shows
			u_seaRows = {viewport.y, 0.0f, 0.0f, 0.0f};
		}
		u_seaParams.y = static_cast<float>(_seaRippleStep);
	}

	// The sea is coloured by the land's light at full luminosity
	glm::vec4 u_seaColour {1.0f};
	if (_landLightTable)
	{
		const auto texel = _landLightTable->GetTexels().back();
		u_seaColour = glm::vec4(static_cast<float>(texel & 0xFFu), static_cast<float>((texel >> 8) & 0xFFu),
		                        static_cast<float>((texel >> 16) & 0xFFu), 255.0f) /
		              255.0f;
	}
	const glm::vec4 u_seaCamera {camera.GetOrigin(), 0.0f};
	waterShader.SetUniformValue("u_seaParams", &u_seaParams);
	waterShader.SetUniformValue("u_seaRows", &u_seaRows);
	waterShader.SetUniformValue("u_seaMode", &u_seaMode);
	waterShader.SetUniformValue("u_seaColour", &u_seaColour);
	waterShader.SetUniformValue("u_seaCamera", &u_seaCamera);
}

void Renderer::DrawHandWaterGlow(const DrawSceneDesc& desc) const
{
	if (!_landLightTable || !Locator::handSystem::has_value() || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const float strength = HandLight::GetStrength(_landLightTable->GetLandColour());
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	if (!hand_water_glow::Shows(inTemple, strength))
	{
		return;
	}
	const auto handEntity =
	    Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
	const auto* transform = Locator::entitiesRegistry::value().TryGet<ecs::components::Transform>(handEntity);
	if (transform == nullptr)
	{
		return;
	}
	const glm::vec2 centre {transform->position.x, transform->position.z};
	const auto& island = Locator::terrainSystem::value();
	const auto altitudeAt = [&island](int x, int z) -> std::optional<uint8_t> {
		if (x < 0 || z < 0 || x >= LandIslandInterface::k_MapCellsPerSide || z >= LandIslandInterface::k_MapCellsPerSide)
		{
			return std::nullopt;
		}
		const auto* cell = island.FindCell({static_cast<uint16_t>(x), static_cast<uint16_t>(z)});
		return cell != nullptr ? std::optional<uint8_t>(cell->altitude) : std::nullopt;
	};
	if (!hand_water_glow::NearLowLand(centre, altitudeAt))
	{
		return;
	}

	const auto& textures = Locator::resources::value().GetTextures();
	const auto atmos = entt::hashed_string("raw/ATMOS");
	const auto atmosAlpha = entt::hashed_string("raw/ATMOSA");
	if (!textures.Contains(atmos.value()) || !textures.Contains(atmosAlpha.value()))
	{
		return;
	}
	const auto colour = hand_water_glow::Colour(_landLightTable->GetWarmColour(), strength);
	const glm::vec4 u_tint = glm::vec4(static_cast<float>((colour >> 16) & 0xFFu), static_cast<float>((colour >> 8) & 0xFFu),
	                                   static_cast<float>(colour & 0xFFu), static_cast<float>(colour >> 24)) /
	                         255.0f;
	// The sprite's x runs along the world's x and its y against the world's z, so the texture's corners fall as the
	// game lays them
	const glm::vec2 uvSize = hand_water_glow::k_UvMaximum - hand_water_glow::k_UvMinimum;
	const glm::vec4 u_sampleRect {uvSize, hand_water_glow::k_UvMinimum};
	// Lying flat, with its own alpha, adding to what is under the sea
	const glm::vec4 u_spriteParams {0.0f, 1.0f, 1.0f, 0.0f};
	glm::mat4 model(0.0f);
	model[0] = glm::vec4(hand_water_glow::k_HalfSize, 0.0f, 0.0f, 0.0f);
	model[1] = glm::vec4(0.0f, 0.0f, -hand_water_glow::k_HalfSize, 0.0f);
	model[2] = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
	model[3] = glm::vec4(centre.x, 0.0f, centre.y, 1.0f);

	const auto* spriteShader = _shaderManager->GetShader("Sprite");
	bgfx::setTransform(glm::value_ptr(model));
	spriteShader->SetUniformValue("u_sampleRect", glm::value_ptr(u_sampleRect));
	spriteShader->SetUniformValue("u_spriteParams", glm::value_ptr(u_spriteParams));
	spriteShader->SetUniformValue("u_tint", glm::value_ptr(u_tint));
	spriteShader->SetTextureSampler("s_diffuse", 0, *textures.Handle(atmos));
	spriteShader->SetTextureSampler("s_alpha", 1, *textures.Handle(atmosAlpha));
	_plane->GetVertexBuffer().Bind();
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
	               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE));
	spriteShader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawCelestialMesh(RenderPass viewId, const CelestialDraw& draw) const
{
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& textures = Locator::resources::value().GetTextures();
	if (!meshes.Contains(draw.meshId) || !textures.Contains(draw.textureId) || !textures.Contains(draw.alphaTextureId))
	{
		return;
	}
	const auto* program = _shaderManager->GetShader("Celestial");
	const auto texture = textures.Handle(draw.textureId);
	const auto alphaTexture = textures.Handle(draw.alphaTextureId);
	const auto mesh = meshes.Handle(draw.meshId);
	for (const auto& subMesh : mesh->GetSubMeshes())
	{
		for (const auto& primitive : subMesh->GetPrimitives())
		{
			bgfx::setTransform(glm::value_ptr(draw.model));
			program->SetTextureSampler("s_diffuse", 0, *texture);
			program->SetTextureSampler("s_alpha", 1, *alphaTexture);
			program->SetUniformValue("u_colour", &draw.colour);
			program->SetUniformValue("u_celestial", &draw.celestial);
			if (subMesh->GetMesh().IsIndexed())
			{
				subMesh->GetMesh().GetIndexBuffer().Bind(primitive.indicesCount, primitive.indicesOffset);
			}
			subMesh->GetMesh().GetVertexBuffer().Bind();
			bgfx::setState(draw.state);
			program->Submit(static_cast<bgfx::ViewId>(viewId));
		}
	}
}

namespace
{
/// Added to what is behind it
constexpr uint64_t k_AdditiveState =
    BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE);

/// Far out to the north west, facing the island
glm::mat4 SunModel(const glm::vec3& position)
{
	return glm::translate(position) * glm::rotate(-3.0f * glm::pi<float>() / 4.0f, glm::vec3(0.0f, 1.0f, 0.0f));
}
} // namespace

void Renderer::DrawMists(const DrawSceneDesc& desc) const
{
	if (Locator::temple::has_value() && Locator::temple::value().Active())
	{
		return;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& textures = Locator::resources::value().GetTextures();
	using ecs::components::Mist;
	if (!meshes.Contains(Mist::k_MeshId) || !textures.Contains(Mist::k_TextureId) ||
	    !textures.Contains(Mist::k_AlphaTextureId) || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto mesh = meshes.Handle(Mist::k_MeshId);
	const auto texture = textures.Handle(Mist::k_TextureId);
	const auto alphaTexture = textures.Handle(Mist::k_AlphaTextureId);
	const auto* program = _shaderManager->GetShader("Mist");
	const auto& island = Locator::terrainSystem::value();
	const glm::vec4 islandExtent {island.GetExtent().minimum, island.GetExtent().maximum};

	// Every mist faces the camera: the mesh's x across the screen, its y towards the camera and its z up the screen
	const auto& camera = *desc.camera;
	const auto origin = camera.GetOrigin();
	const glm::mat3 facing {camera.GetRight(), -camera.GetForward(), camera.GetUp()};
	// A mist that shrinks edge on is lit from straight above, more brightly
	const glm::vec4 skyLight {0.0f, 500000.0f, 0.0f, 210.0f};

	// The clouds' colour follows the sky's alignment, in the land's light at full luminosity
	const bool cloudsShown = detail_level::Clouds(Locator::config::value().detailLevel);
	uint32_t fullLight = 0xFFFFFF;
	if (_landLightTable)
	{
		const auto texel = _landLightTable->GetTexels().back();
		fullLight = ((texel & 0xFFu) << 16u) | (texel & 0xFF00u) | ((texel >> 16u) & 0xFFu);
	}
	const auto cloudColour = clouds::Colour(Locator::alignmentSystem::value().GetSkyAlignment(), fullLight);

	desc.entities.Each<const Mist, const ecs::components::Transform>([&](entt::entity entity, const Mist& mist,
	                                                                     const ecs::components::Transform& transform) {
		auto colour = mist.colour;
		if (desc.entities.AnyOf<ecs::components::Cloud>(entity))
		{
			if (!cloudsShown)
			{
				return;
			}
			// How far the cloud has faded at the track's ends, times the sky's own alpha, in whole numbers
			const auto cloudAlpha = ((mist.colour >> 24u) * (cloudColour >> 24u)) / 255u;
			colour = (cloudAlpha << 24u) | (cloudColour & 0xFFFFFFu);
		}
		const auto alpha = static_cast<float>(colour >> 24u);
		if (alpha <= 0.0f)
		{
			return;
		}
		// A mist that shrinks edge on keeps its width and squashes its depth and height
		glm::vec3 scale {mist.size};
		if (mist.shrinksEdgeOn)
		{
			scale.y = scale.z = mists::EdgeOnSize(mist.size, mist.edgeShrink, transform.position - origin);
		}
		const auto model = glm::translate(transform.position) * glm::mat4(facing) * glm::scale(scale);
		const auto frame = mists::FrameOffset(mists::Frame(mist.counter), mist.shrinksEdgeOn);
		const glm::vec4 u_mist {frame, 0.0f, 0.0f};
		const glm::vec4 u_mistColour {static_cast<float>((colour >> 16u) & 0xFFu) / 255.0f,
		                              static_cast<float>((colour >> 8u) & 0xFFu) / 255.0f,
		                              static_cast<float>(colour & 0xFFu) / 255.0f, alpha / 255.0f};
		// The others take the land's light where they stand and the haze, in the models' light
		const bool landLit = !mist.shrinksEdgeOn && _landLightTexture.has_value();
		const glm::vec4 u_landLight {landLit ? 1.0f : 0.0f, 1.0f, 0.0f, 0.0f};
		const glm::vec4 noHaze {0.0f};

		for (const auto& subMesh : mesh->GetSubMeshes())
		{
			for (const auto& primitive : subMesh->GetPrimitives())
			{
				bgfx::setTransform(glm::value_ptr(model));
				program->SetTextureSampler("s_diffuse", 0, *texture);
				program->SetTextureSampler("s_alpha", 1, *alphaTexture);
				program->SetTextureSampler("s_landLuminosity", 6, GetLandLuminosity());
				program->SetTextureSampler("s_landLight", 7, GetLandLightTexture());
				program->SetTextureSampler("s_landColour", 8, GetLandColour());
				program->SetUniformValue("u_islandExtent", &islandExtent);
				program->SetUniformValue("u_landLight", &u_landLight);
				program->SetUniformValue("u_haze", landLit ? &_haze[0] : &noHaze);
				program->SetUniformValue("u_hazeColour", &_haze[1]);
				program->SetUniformValue("u_modelLight", mist.shrinksEdgeOn ? &skyLight : &_modelLight);
				program->SetUniformValue("u_mist", &u_mist);
				program->SetUniformValue("u_mistColour", &u_mistColour);
				if (subMesh->GetMesh().IsIndexed())
				{
					subMesh->GetMesh().GetIndexBuffer().Bind(primitive.indicesCount, primitive.indicesOffset);
				}
				subMesh->GetMesh().GetVertexBuffer().Bind();
				// Both sides, blended over what is behind, tested against but not writing depth
				bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA);
				// In its place among everything that blends, the farthest first
				program->Submit(static_cast<bgfx::ViewId>(TranslucentView(desc.viewId)),
				                zsort::Depth(transform.position, origin));
			}
		}
	});
}

void Renderer::DrawCreatureFootprints(const DrawSceneDesc& desc) const
{
	namespace prints = creature_footprints;
	const bool reflection = desc.viewId == RenderPass::Reflection;
	if ((desc.viewId != RenderPass::Main && !reflection) || !Locator::footprintSystem::has_value() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	const auto& footprints = Locator::footprintSystem::value();
	if (!footprints.IsShown() || footprints.GetPrints().empty())
	{
		return;
	}
	// White where the prints' pictures are, and their shapes in the alpha beside it
	static constexpr auto k_TextureId = entt::hashed_string("raw/misc0");
	static constexpr auto k_AlphaTextureId = entt::hashed_string("raw/misc0a");
	const auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(k_TextureId.value()) || !textures.Contains(k_AlphaTextureId.value()))
	{
		return;
	}
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	std::vector<Vertex> vertices;
	vertices.reserve(footprints.GetPrints().size() * prints::k_Indices.size());
	for (const auto& print : footprints.GetPrints())
	{
		// The sea's reflection shows only the prints on the land above it
		if (reflection &&
		    std::ranges::any_of(print.corners, [](const glm::vec3& corner) { return corner.y <= prints::k_Lift; }))
		{
			continue;
		}
		// Black, as opaque as the print still is
		const auto colour = static_cast<uint32_t>(print.alpha) << 24u;
		for (const auto corner : prints::k_Indices)
		{
			vertices.push_back({print.corners.at(corner), print.uvs.at(corner), colour});
		}
	}
	if (vertices.empty())
	{
		return;
	}
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto count = static_cast<uint32_t>(vertices.size());
	if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, count, layout);
	std::memcpy(buffer.data, vertices.data(), vertices.size() * sizeof(Vertex));
	const auto* program = _shaderManager->GetShader("WorldTextured");
	program->SetTextureSampler("s_diffuse", 0, *textures.Handle(k_TextureId));
	program->SetTextureSampler("s_alpha", 1, *textures.Handle(k_AlphaTextureId));
	bgfx::setVertexBuffer(0, &buffer);
	// Unlit, blended over the land, tested against depth but leaving none, both sides
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA);
	// Lying on the land, they are blended first of all that blends, straight after the land is drawn
	program->Submit(static_cast<uint16_t>(TranslucentView(desc.viewId)), std::numeric_limits<uint32_t>::max());
}

void Renderer::DrawGroundBlobs(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::terrainSystem::has_value() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	static constexpr auto k_TextureId = entt::hashed_string("raw/human_shadow");
	const auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(k_TextureId.value()))
	{
		return;
	}
	const auto& island = Locator::terrainSystem::value();
	const auto& meshes = Locator::resources::value().GetMeshes();
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	std::vector<Vertex> vertices;
	const auto addQuad = [&vertices](const ground_blobs::Quad& quad) {
		for (const auto corner : ground_blobs::k_Indices)
		{
			const auto opacity = static_cast<uint32_t>(ground_blobs::k_Opacity.at(corner) * 255.0f);
			vertices.push_back({quad.corners.at(corner), ground_blobs::k_Uvs.at(corner), (opacity << 24u) | 0xFFFFFFu});
		}
	};
	// Every villager out of doors and out of the sea casts one from each foot, on the land beneath it
	desc.entities.Each<const ecs::components::Villager, const ecs::components::Transform, const ecs::components::Mesh>(
	    [&](const ecs::components::Villager& /*villager*/, const ecs::components::Transform& transform,
	        const ecs::components::Mesh& mesh) {
		    if (transform.position.y <= ground_blobs::k_LowestHeight || !meshes.Contains(mesh.id))
		    {
			    return;
		    }
		    const auto& bones = meshes.Handle(mesh.id)->GetBoneMatrices();
		    if (std::ranges::any_of(ground_blobs::k_FootBones, [&bones](size_t bone) { return bone >= bones.size(); }))
		    {
			    return;
		    }
		    const auto model = glm::translate(transform.position) * glm::mat4(transform.rotation) * glm::scale(transform.scale);
		    const auto foot = [&](size_t bone) {
			    auto position = glm::vec3(model * bones.at(bone) * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
			    position.y = island.GetHeightAt(glm::vec2(position.x, position.z)) + ground_blobs::k_Lift;
			    return position;
		    };
		    const auto normal = island.GetNormalAt(glm::vec2(transform.position.x, transform.position.z));
		    const auto fall = ground_blobs::Fall(normal, transform.scale.x);
		    for (const auto& quad :
		         ground_blobs::Feet(foot(ground_blobs::k_FootBones[0]), foot(ground_blobs::k_FootBones[1]), fall))
		    {
			    addQuad(quad);
		    }
	    },
	    entt::exclude<ecs::components::AtHome>);
	if (vertices.empty())
	{
		return;
	}
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto count = static_cast<uint32_t>(vertices.size());
	if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, count, layout);
	std::memcpy(buffer.data, vertices.data(), vertices.size() * sizeof(Vertex));
	const auto* program = _shaderManager->GetShader("Blob");
	program->SetTextureSampler("s_diffuse", 0, *textures.Handle(k_TextureId));
	bgfx::setVertexBuffer(0, &buffer);
	// Blended over the land, tested against depth but leaving none, both sides
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA | BGFX_STATE_MSAA);
	program->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawInfluenceRipples(const DrawSceneDesc& desc) const
{
	using ecs::components::Mist;
	if (desc.viewId != RenderPass::Main || !Locator::influenceSystem::has_value())
	{
		return;
	}
	const auto& textures = Locator::resources::value().GetTextures();
	const auto ripples = Locator::influenceSystem::value().GetRipples();
	if (ripples.empty() || !textures.Contains(Mist::k_TextureId) || !textures.Contains(Mist::k_AlphaTextureId))
	{
		return;
	}
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto* program = _shaderManager->GetShader("WorldTextured");
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentView(desc.viewId));
	const auto origin = desc.camera->GetOrigin();
	// The last frame of the smoke texture, its corners as the puff's
	constexpr std::array<glm::vec2, 4> k_Uvs = {glm::vec2(0.875f, 0.875f), glm::vec2(1.0f, 0.875f), glm::vec2(1.0f, 1.0f),
	                                            glm::vec2(0.875f, 1.0f)};
	constexpr std::array<uint16_t, 6> k_Triangles = {0, 1, 2, 0, 2, 3};
	constexpr auto k_Vertices = static_cast<uint32_t>(influence::Ripple::k_Puffs * 4);
	constexpr auto k_Indices = static_cast<uint32_t>(influence::Ripple::k_Puffs * 6);
	for (const auto& ripple : ripples)
	{
		if (bgfx::getAvailTransientVertexBuffer(k_Vertices, layout) < k_Vertices ||
		    bgfx::getAvailTransientIndexBuffer(k_Indices) < k_Indices)
		{
			return;
		}
		bgfx::TransientVertexBuffer vertexBuffer;
		bgfx::TransientIndexBuffer indexBuffer;
		bgfx::allocTransientVertexBuffer(&vertexBuffer, k_Vertices, layout);
		bgfx::allocTransientIndexBuffer(&indexBuffer, k_Indices);
		const auto vertices = std::span(reinterpret_cast<Vertex*>(vertexBuffer.data), k_Vertices);
		const auto indices = std::span(reinterpret_cast<uint16_t*>(indexBuffer.data), k_Indices);
		const auto abgr = ((ripple.rgb & 0xFFu) << 16u) | (ripple.rgb & 0xFF00u) | ((ripple.rgb >> 16u) & 0xFFu);
		for (size_t puff = 0; puff < influence::Ripple::k_Puffs; ++puff)
		{
			const auto corners = influence::PuffCorners(ripple, puff);
			const auto colour = (static_cast<uint32_t>(ripple.alphas.at(puff)) << 24u) | abgr;
			for (size_t c = 0; c < corners.size(); ++c)
			{
				vertices[(puff * 4) + c] = {corners.at(c), k_Uvs.at(c), colour};
			}
			for (size_t i = 0; i < k_Triangles.size(); ++i)
			{
				indices[(puff * 6) + i] = static_cast<uint16_t>((puff * 4) + k_Triangles.at(i));
			}
		}
		program->SetTextureSampler("s_diffuse", 0, *textures.Handle(Mist::k_TextureId));
		program->SetTextureSampler("s_alpha", 1, *textures.Handle(Mist::k_AlphaTextureId));
		bgfx::setVertexBuffer(0, &vertexBuffer);
		bgfx::setIndexBuffer(&indexBuffer);
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA);
		program->Submit(viewId, zsort::Depth(ripple.point, origin));
	}
}

void Renderer::DrawInfluenceBorder(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::influenceSystem::has_value() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	// The border only shows from high enough over the land
	const auto alpha = influence::CurtainAlpha(desc.camera->GetOrigin().y);
	static constexpr auto k_TextureId = entt::hashed_string("raw/burn");
	static constexpr auto k_AlphaTextureId = entt::hashed_string("raw/burna");
	const auto& textures = Locator::resources::value().GetTextures();
	if (!alpha.has_value() || !textures.Contains(k_TextureId.value()) || !textures.Contains(k_AlphaTextureId.value()))
	{
		return;
	}
	const auto& influenceSystem = Locator::influenceSystem::value();
	const auto offset = influenceSystem.GetScrollOffset();
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto* program = _shaderManager->GetShader("WorldTextured");
	for (const auto& circle : influenceSystem.GetCircles())
	{
		if (!influenceSystem.IsBorderShown(circle.player))
		{
			continue;
		}
		const auto& curtain = circle.curtain;
		const auto vertexCount = static_cast<uint32_t>(curtain.positions.size());
		const auto indexCount = static_cast<uint32_t>(curtain.indices.size());
		if (bgfx::getAvailTransientVertexBuffer(vertexCount, layout) < vertexCount ||
		    bgfx::getAvailTransientIndexBuffer(indexCount) < indexCount)
		{
			return;
		}
		bgfx::TransientVertexBuffer vertexBuffer;
		bgfx::TransientIndexBuffer indexBuffer;
		bgfx::allocTransientVertexBuffer(&vertexBuffer, vertexCount, layout);
		bgfx::allocTransientIndexBuffer(&indexBuffer, indexCount);
		const auto vertices = std::span(reinterpret_cast<Vertex*>(vertexBuffer.data), vertexCount);
		const auto indices = std::span(reinterpret_cast<uint16_t*>(indexBuffer.data), indexCount);
		// The player's colour; only the middle row shows, the ground and top fading out to it, and not where the
		// circle is inside another of the player's. A hidden closing column fades to white, as the game's does.
		const auto rgb = influence::k_PlayerColours.at(static_cast<size_t>(circle.player) & 7u);
		const auto abgr = ((rgb & 0xFFu) << 16u) | (rgb & 0xFF00u) | ((rgb >> 16u) & 0xFFu);
		const auto lastColumn = circle.Columns() - 1;
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			const auto column = i / 3;
			const bool hidden = circle.hidden.at(column);
			const auto colour = hidden && column == lastColumn ? 0xFFFFFFu : abgr;
			const uint32_t a = i % 3 == 1 && !hidden ? *alpha : 0u;
			vertices[i] = {curtain.positions[i], curtain.uvs[i] + offset, (a << 24u) | colour};
		}
		for (size_t i = 0; i < indices.size(); ++i)
		{
			indices[i] = static_cast<uint16_t>(curtain.indices[i]);
		}
		program->SetTextureSampler("s_diffuse", 0, *textures.Handle(k_TextureId));
		program->SetTextureSampler("s_alpha", 1, *textures.Handle(k_AlphaTextureId));
		bgfx::setVertexBuffer(0, &vertexBuffer);
		bgfx::setIndexBuffer(&indexBuffer);
		// Blended over what is behind, tested against depth but leaving none, both sides
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA);
		program->Submit(static_cast<bgfx::ViewId>(desc.viewId));
	}
}

void Renderer::DrawChimneySmoke(const DrawSceneDesc& desc) const
{
	using ecs::components::ChimneySmoke;
	using ecs::components::Mist;
	if (desc.viewId != RenderPass::Main || (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	const auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(Mist::k_TextureId) || !textures.Contains(Mist::k_AlphaTextureId))
	{
		return;
	}
	const auto texture = textures.Handle(Mist::k_TextureId);
	const auto alphaTexture = textures.Handle(Mist::k_AlphaTextureId);
	const auto* spriteShader = _shaderManager->GetShader("Sprite");
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentView(desc.viewId));
	const auto origin = desc.camera->GetOrigin();
	// Turned to face the camera, with the texture's own alpha, blended over what is behind
	const glm::vec4 u_spriteParams {1.0f, 1.0f, 0.0f, 0.0f};
	desc.entities.Each<const ChimneySmoke>([&](const ChimneySmoke& smoke) {
		if (smoke.state == ChimneySmoke::State::Out)
		{
			return;
		}
		// The whole smoke takes its place in the sort by its chimney, its puffs in turn
		const auto depth = zsort::Depth(smoke.chimney, origin);
		for (const auto& puff : smoke.puffs)
		{
			if (puff.hidden)
			{
				continue;
			}
			const auto look = chimney_smoke::LookOf(puff.age, smoke.rgb);
			// Spinning in the plane of the screen
			const auto model = glm::translate(puff.position) * glm::rotate(-puff.angle, glm::vec3(0.0f, 0.0f, 1.0f)) *
			                   glm::scale(glm::vec3(look.halfWidth));
			const glm::vec4 u_sampleRect {0.125f, 0.125f, mists::FrameOffset(look.frame, false)};
			const float alpha = static_cast<float>(look.argb >> 24u) / 255.0f;
			const glm::vec3 colour =
			    glm::vec3(static_cast<float>((look.argb >> 16u) & 0xFFu), static_cast<float>((look.argb >> 8u) & 0xFFu),
			              static_cast<float>(look.argb & 0xFFu)) /
			    255.0f;
			// The shader multiplies the tint by the texture's alpha: premultiplied for the blend
			const glm::vec4 u_tint {colour * alpha, alpha};
			bgfx::setTransform(glm::value_ptr(model));
			spriteShader->SetUniformValue("u_sampleRect", glm::value_ptr(u_sampleRect));
			spriteShader->SetUniformValue("u_spriteParams", glm::value_ptr(u_spriteParams));
			spriteShader->SetUniformValue("u_tint", glm::value_ptr(u_tint));
			spriteShader->SetTextureSampler("s_diffuse", 0, *texture);
			spriteShader->SetTextureSampler("s_alpha", 1, *alphaTexture);
			_plane->GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
			               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA));
			spriteShader->Submit(viewId, depth);
		}
	});
}

void Renderer::DrawWaterRings(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::waterRingSystem::has_value() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	const auto rings = Locator::waterRingSystem::value().GetRings();
	const auto& textures = Locator::resources::value().GetTextures();
	if (rings.empty() || !textures.Contains(water_rings::k_TextureId.value()) ||
	    !textures.Contains(water_rings::k_AlphaTextureId.value()))
	{
		return;
	}
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto count = static_cast<uint32_t>(rings.size());
	if (bgfx::getAvailTransientVertexBuffer(count * 4, layout) < count * 4 ||
	    bgfx::getAvailTransientIndexBuffer(count * 6) < count * 6)
	{
		return;
	}
	bgfx::TransientVertexBuffer vertexBuffer;
	bgfx::TransientIndexBuffer indexBuffer;
	bgfx::allocTransientVertexBuffer(&vertexBuffer, count * 4, layout);
	bgfx::allocTransientIndexBuffer(&indexBuffer, count * 6);
	const auto vertices = std::span(reinterpret_cast<Vertex*>(vertexBuffer.data), count * 4);
	const auto indices = std::span(reinterpret_cast<uint16_t*>(indexBuffer.data), count * 6);
	constexpr std::array<uint16_t, 6> k_Triangles = {0, 1, 2, 0, 2, 3};
	for (size_t i = 0; i < rings.size(); ++i)
	{
		const auto& ring = rings[i];
		const auto corners = water_rings::Corners(ring);
		const auto uvs = water_rings::CellUvs(ring.cell);
		const auto rgb = ring.argb & 0xFFFFFFu;
		const auto abgr = (static_cast<uint32_t>(water_rings::Alpha(ring)) << 24u) | ((rgb & 0xFFu) << 16u) | (rgb & 0xFF00u) |
		                  ((rgb >> 16u) & 0xFFu);
		for (size_t c = 0; c < corners.size(); ++c)
		{
			vertices[(i * 4) + c] = {corners.at(c), uvs.at(c), abgr};
		}
		for (size_t t = 0; t < k_Triangles.size(); ++t)
		{
			indices[(i * 6) + t] = static_cast<uint16_t>((i * 4) + k_Triangles.at(t));
		}
	}
	const auto* program = _shaderManager->GetShader("WorldTextured");
	program->SetTextureSampler("s_diffuse", 0, *textures.Handle(water_rings::k_TextureId.value()));
	program->SetTextureSampler("s_alpha", 1, *textures.Handle(water_rings::k_AlphaTextureId.value()));
	bgfx::setVertexBuffer(0, &vertexBuffer);
	bgfx::setIndexBuffer(&indexBuffer);
	// Added over what is behind by their alpha, both sides, tested against depth but leaving none
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER |
	               BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE));
	program->Submit(static_cast<bgfx::ViewId>(desc.viewId));
}

void Renderer::DrawSnowfall(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::snowfallSystem::has_value() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	static constexpr auto k_TextureId = entt::hashed_string("raw/ATMOS");
	static constexpr auto k_AlphaTextureId = entt::hashed_string("raw/ATMOSA");
	const auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(k_TextureId.value()) || !textures.Contains(k_AlphaTextureId.value()))
	{
		return;
	}
	auto& snowfallSystem = Locator::snowfallSystem::value();
	const auto origin = desc.camera->GetOrigin();
	const auto tiles = snowfallSystem.TakeTiles(origin);
	if (tiles.empty())
	{
		return;
	}
	const auto flakes = snowfallSystem.GetFlakes();
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	// The flakes take the colour of the land's light at its brightest
	const auto landColour = _landLightTable ? _landLightTable->GetLandColour() : 0xFFFFFFu;
	const auto bgr = ((landColour & 0xFFu) << 16u) | (landColour & 0xFF00u) | ((landColour >> 16u) & 0xFFu);
	const auto* program = _shaderManager->GetShader("WorldTextured");
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentView(desc.viewId));
	constexpr std::array<uint16_t, 6> k_Triangles = {0, 1, 2, 2, 3, 0};
	for (const auto& tile : tiles)
	{
		const auto count = static_cast<uint32_t>(tile.flakes);
		if (bgfx::getAvailTransientVertexBuffer(count * 4, layout) < count * 4 ||
		    bgfx::getAvailTransientIndexBuffer(count * 6) < count * 6)
		{
			return;
		}
		bgfx::TransientVertexBuffer vertexBuffer;
		bgfx::TransientIndexBuffer indexBuffer;
		bgfx::allocTransientVertexBuffer(&vertexBuffer, count * 4, layout);
		bgfx::allocTransientIndexBuffer(&indexBuffer, count * 6);
		const auto vertices = std::span(reinterpret_cast<Vertex*>(vertexBuffer.data), count * 4);
		const auto indices = std::span(reinterpret_cast<uint16_t*>(indexBuffer.data), count * 6);
		const glm::vec3 corner {tile.corner.x, tile.ground, tile.corner.y};
		const auto colour = (static_cast<uint32_t>(tile.alpha) << 24u) | bgr;
		for (size_t i = 0; i < count; ++i)
		{
			const auto corners = snowfall::Corners(flakes[i]);
			for (size_t c = 0; c < corners.size(); ++c)
			{
				vertices[(i * 4) + c] = {corner + corners.at(c), snowfall::k_Uvs.at(c), colour};
			}
			for (size_t t = 0; t < k_Triangles.size(); ++t)
			{
				indices[(i * 6) + t] = static_cast<uint16_t>((i * 4) + k_Triangles.at(t));
			}
		}
		program->SetTextureSampler("s_diffuse", 0, *textures.Handle(k_TextureId));
		program->SetTextureSampler("s_alpha", 1, *textures.Handle(k_AlphaTextureId));
		bgfx::setVertexBuffer(0, &vertexBuffer);
		bgfx::setIndexBuffer(&indexBuffer);
		// Both sides of each flake, blended over what is behind, tested against depth but leaving none
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA);
		// In its place among what blends, by the ground under the quarter's corner
		program->Submit(viewId, zsort::Depth(corner, origin));
	}
}

void Renderer::DrawRain(const DrawSceneDesc& desc) const
{
	if (desc.viewId != RenderPass::Main || !Locator::rainSystem::has_value() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	static constexpr auto k_TextureId = entt::hashed_string("raw/ATMOS");
	static constexpr auto k_AlphaTextureId = entt::hashed_string("raw/ATMOSA");
	const auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(k_TextureId.value()) || !textures.Contains(k_AlphaTextureId.value()))
	{
		return;
	}
	auto& rainSystem = Locator::rainSystem::value();
	const auto origin = desc.camera->GetOrigin();
	const auto tiles = rainSystem.TakeTiles(origin);
	if (tiles.empty())
	{
		return;
	}
	const auto streaks = rainSystem.GetStreaks();
	const float height = rainSystem.GetHeight();
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		uint32_t colour;
	};
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
	    .end();
	const auto* program = _shaderManager->GetShader("WorldTextured");
	const auto viewId = static_cast<bgfx::ViewId>(TranslucentView(desc.viewId));
	for (const auto& tile : tiles)
	{
		const auto count = static_cast<uint32_t>(tile.streaks) * 2;
		if (bgfx::getAvailTransientVertexBuffer(count, layout) < count)
		{
			return;
		}
		bgfx::TransientVertexBuffer buffer;
		bgfx::allocTransientVertexBuffer(&buffer, count, layout);
		const auto vertices = std::span(reinterpret_cast<Vertex*>(buffer.data), count);
		const glm::vec3 centre {tile.centre.x, tile.ground, tile.centre.y};
		// Each streak is a white line, as opaque as the block's rain at the bottom and fainter at the top, with a row of
		// the texture scrolling down it
		for (size_t i = 0; i < static_cast<size_t>(tile.streaks); ++i)
		{
			const auto& streak = streaks[i];
			const auto ends = rain::Ends(streak, height);
			const auto bottom = static_cast<uint32_t>(rain::StreakAlpha(tile.alpha, streak.phase)) & 0xFFu;
			const auto top = static_cast<uint32_t>(rain::StreakAlpha(tile.alphaTop, streak.phase)) & 0xFFu;
			vertices[i * 2] = {centre + ends[0], {streak.scroll, rain::k_TextureRow}, (bottom << 24u) | 0xFFFFFFu};
			vertices[(i * 2) + 1] = {centre + ends[1], {streak.scroll + 1.0f, rain::k_TextureRow}, (top << 24u) | 0xFFFFFFu};
		}
		program->SetTextureSampler("s_diffuse", 0, *textures.Handle(k_TextureId));
		program->SetTextureSampler("s_alpha", 1, *textures.Handle(k_AlphaTextureId));
		bgfx::setVertexBuffer(0, &buffer);
		// Lines, blended over what is behind, tested against depth but leaving none
		bgfx::setState(BGFX_STATE_PT_LINES | BGFX_STATE_WRITE_RGB | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA);
		// In its place among what blends, by the ground under the block's centre
		program->Submit(viewId, zsort::Depth(centre, origin));
	}
}

void Renderer::DrawMoon(RenderPass viewId) const
{
	if (!Locator::camera::has_value())
	{
		return;
	}
	const auto placement = moon::Place(Locator::skySystem::value().GetClock().GetScriptTime());
	if (!placement)
	{
		return;
	}
	// The moon keeps its place beside the player's camera and faces it. Drawn so in the mirrored view, it is mirrored
	// in the sea with everything else.
	const auto& camera = Locator::camera::value();
	const auto centre = camera.GetOrigin() + placement->offset;
	const auto view = camera.GetViewMatrix(Camera::Interpolation::Current);
	const auto basis = moon::Basis(view, glm::inverse(view), centre);
	const auto moonColour = _landLightTable ? _landLightTable->GetMoonColour() : 0xFFFFFFu;
	const glm::vec3 colour = glm::vec3(static_cast<float>((moonColour >> 16) & 0xFFu),
	                                   static_cast<float>((moonColour >> 8) & 0xFFu), static_cast<float>(moonColour & 0xFFu)) /
	                         255.0f;
	// It shows less through an overcast
	const float alpha =
	    sky_dome::ThroughOvercast(placement->alpha, _overcast, detail_level::Fog(Locator::config::value().detailLevel)) /
	    255.0f;
	if (alpha <= 0.0f)
	{
		return;
	}

	// First its glow, added to the sky
	const auto& textures = Locator::resources::value().GetTextures();
	const auto atmos = entt::hashed_string("raw/ATMOS");
	const auto atmosAlpha = entt::hashed_string("raw/ATMOSA");
	bgfx::VertexLayout layout;
	layout.begin()
	    .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
	    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
	    .end();
	constexpr auto k_GlowVertices = static_cast<uint32_t>(moon::k_GlowIndices.size());
	if (textures.Contains(atmos.value()) && textures.Contains(atmosAlpha.value()) &&
	    bgfx::getAvailTransientVertexBuffer(k_GlowVertices, layout) == k_GlowVertices)
	{
		struct Vertex
		{
			glm::vec3 position;
			glm::vec2 uv;
		};
		bgfx::TransientVertexBuffer buffer;
		bgfx::allocTransientVertexBuffer(&buffer, k_GlowVertices, layout);
		const auto vertices = std::span(reinterpret_cast<Vertex*>(buffer.data), k_GlowVertices);
		const auto glow = moon::MakeGlow(basis, centre);
		for (size_t i = 0; i < vertices.size(); ++i)
		{
			const auto corner = moon::k_GlowIndices.at(i);
			vertices[i] = {glow.corners.at(corner), glow.uvs.at(corner)};
		}
		const auto* program = _shaderManager->GetShader("Celestial");
		const glm::mat4 identity(1.0f);
		const glm::vec4 glowColour {moon::GlowColour(colour), alpha};
		const glm::vec4 celestial {0.0f, 0.0f, 0.0f, 1.0f};
		bgfx::setTransform(glm::value_ptr(identity));
		program->SetTextureSampler("s_diffuse", 0, *textures.Handle(atmos));
		program->SetTextureSampler("s_alpha", 1, *textures.Handle(atmosAlpha));
		program->SetUniformValue("u_colour", &glowColour);
		program->SetUniformValue("u_celestial", &celestial);
		bgfx::setVertexBuffer(0, &buffer);
		bgfx::setState(k_AdditiveState | BGFX_STATE_DEPTH_TEST_GREATER);
		program->Submit(static_cast<bgfx::ViewId>(viewId));
	}

	// Then the moon, blended over the sky, its face turned to the real moon's phase. It leaves its depth, so the land
	// nearer than it is drawn over it and the land beyond it stays hidden.
	const auto now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
	const auto phase = moon::Phase(now.count());
	DrawCelestialMesh(
	    viewId, {
	                .meshId = SkyInterface::k_MoonMeshId.value(),
	                .textureId = SkyInterface::k_MoonTextureId.value(),
	                .alphaTextureId = SkyInterface::k_MoonAlphaTextureId.value(),
	                .model = moon::Model(basis, centre, phase),
	                .colour = glm::vec4(colour, alpha),
	                .celestial = {std::cos(phase), std::sin(phase), 1.0f, 1.0f},
	                .state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_BLEND_ALPHA,
	            });
}

void Renderer::DrawSun(RenderPass viewId) const
{
	const auto placement = sun::Place(Locator::skySystem::value().GetClock().GetScriptTime());
	if (!placement)
	{
		return;
	}
	// It shows less through an overcast
	const float alpha =
	    sky_dome::ThroughOvercast(placement->alpha, _overcast, detail_level::Fog(Locator::config::value().detailLevel));
	// In a warm colour, added to the sky drawn before it, leaving no depth; the land drawn after it covers it
	DrawCelestialMesh(viewId, {
	                              .meshId = SkyInterface::k_SunMeshId.value(),
	                              .textureId = SkyInterface::k_SunTextureId.value(),
	                              .alphaTextureId = SkyInterface::k_SunTextureId.value(),
	                              .model = SunModel(placement->position),
	                              .colour = {0x95 / 255.0f, 0x7C / 255.0f, 0x63 / 255.0f, alpha / 255.0f},
	                              .celestial = glm::vec4(0.0f),
	                              .state = k_AdditiveState | BGFX_STATE_DEPTH_TEST_GREATER,
	                          });
}

void Renderer::DrawSunGlare(const Camera& camera) const
{
	const auto placement = sun::Place(Locator::skySystem::value().GetClock().GetScriptTime());
	if (!placement)
	{
		return;
	}
	const auto model = SunModel(placement->position);

	// Each sample of the sun the land or a thing hides from the camera dims the glare by a fifth
	int hidden = 0;
	const auto origin = camera.GetOrigin();
	const auto right = glm::vec3(model * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
	if (Locator::dynamicsSystem::has_value())
	{
		for (const auto& offset : sun::k_GlareSamples)
		{
			auto sample = placement->position + (right * offset.x) + glm::vec3(0.0f, offset.y, 0.0f);
			sample.y = std::max(sample.y, sun::k_GlareLowestSample);
			const auto towards = sample - origin;
			if (Locator::dynamicsSystem::value().RayCastClosestHit(origin, glm::normalize(towards), glm::length(towards)))
			{
				++hidden;
			}
		}
	}
	const auto frameMilliseconds = static_cast<uint32_t>(Locator::time::value().GetFrameGameTime().count());
	_sunGlare = sun::EaseGlare(_sunGlare, hidden, frameMilliseconds);
	if (_sunGlare <= 0.0f)
	{
		return;
	}
	// Larger, in an orange colour, over everything in the view. The temple isn't among what the samples can hit, so inside
	// it the glare is tested against the depth of what is drawn: its solid parts hide it, and it shows through its glass
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	DrawCelestialMesh(RenderPass::Main, {
	                                        .meshId = SkyInterface::k_SunMeshId.value(),
	                                        .textureId = SkyInterface::k_SunTextureId.value(),
	                                        .alphaTextureId = SkyInterface::k_SunTextureId.value(),
	                                        .model = model * glm::scale(glm::vec3(sun::k_GlareScale)),
	                                        .colour = {0xA0 / 255.0f, 0x6A / 255.0f, 0x35 / 255.0f,
	                                                   _sunGlare * placement->alpha / (255.0f * 255.0f)},
	                                        .celestial = glm::vec4(0.0f),
	                                        .state = k_AdditiveState | (inTemple ? BGFX_STATE_DEPTH_TEST_GREATER : 0),
	                                    });
}

namespace
{
/// Whether a target is made and sized to the land's cell corners
bool FitsLand(const std::unique_ptr<FrameBuffer>& frameBuffer, glm::u16vec2 size)
{
	uint16_t width = 0;
	uint16_t height = 0;
	if (frameBuffer)
	{
		frameBuffer->GetSize(width, height);
	}
	return frameBuffer && width == size.x && height == size.y;
}

/// A view drawing into a target a texel for each of the land's cell corners, in texels, a row for each cell along z,
/// whichever way up the backend keeps its targets. Its draws are kept in order.
bgfx::ViewId SetUpLandView(RenderPass pass, const FrameBuffer& frameBuffer, glm::u16vec2 size)
{
	const auto w = static_cast<float>(size.x);
	const auto h = static_cast<float>(size.y);
	const auto projection = bgfx::getCaps()->originBottomLeft ? glm::ortho(0.0f, w, 0.0f, h) : glm::ortho(0.0f, w, h, 0.0f);
	const glm::mat4 identity(1.0f);
	const auto viewId = static_cast<bgfx::ViewId>(pass);
	frameBuffer.Bind(pass);
	bgfx::setViewRect(viewId, 0, 0, size.x, size.y);
	bgfx::setViewMode(viewId, bgfx::ViewMode::Sequential);
	bgfx::setViewTransform(viewId, glm::value_ptr(identity), glm::value_ptr(projection));
	bgfx::touch(viewId);
	return viewId;
}

/// A rectangle of cells drawn into a land view, from a position and texture coordinate to another; the program's
/// samplers and uniforms are set before
void SubmitLandQuad(bgfx::ViewId viewId, const ShaderProgram& program, glm::vec2 from, glm::vec2 to, glm::vec2 uvFrom,
                    glm::vec2 uvTo, uint64_t state)
{
	struct Vertex
	{
		glm::vec2 position;
		glm::vec2 uv;
	};
	static const auto k_Layout = [] {
		bgfx::VertexLayout layout;
		layout.begin()
		    .add(bgfx::Attrib::Position, 2, bgfx::AttribType::Float)
		    .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
		    .end();
		return layout;
	}();
	if (bgfx::getAvailTransientVertexBuffer(6, k_Layout) < 6)
	{
		return;
	}
	bgfx::TransientVertexBuffer buffer;
	bgfx::allocTransientVertexBuffer(&buffer, 6, k_Layout);
	const auto vertices = std::span(reinterpret_cast<Vertex*>(buffer.data), 6);
	const std::array<Vertex, 4> corners {{
	    {{from.x, from.y}, {uvFrom.x, uvFrom.y}},
	    {{to.x, from.y}, {uvTo.x, uvFrom.y}},
	    {{to.x, to.y}, {uvTo.x, uvTo.y}},
	    {{from.x, to.y}, {uvFrom.x, uvTo.y}},
	}};
	constexpr std::array<size_t, 6> k_Indices {0, 1, 2, 2, 3, 0};
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		vertices[i] = corners.at(k_Indices.at(i));
	}
	bgfx::setVertexBuffer(0, &buffer);
	bgfx::setState(state);
	program.Submit(viewId);
}
} // namespace

const Texture2D& Renderer::GetLandLuminosity() const
{
	if (_landLuminosityFrameBuffer)
	{
		return _landLuminosityFrameBuffer->GetColorAttachment();
	}
	return Locator::terrainSystem::value().GetLuminosityMap();
}

void Renderer::DrawLandLuminosityPass(const DrawSceneDesc& drawDesc) const
{
	if (!drawDesc.drawIsland || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto& island = Locator::terrainSystem::value();
	const auto& luminosity = island.GetLuminosityMap();
	const auto size = luminosity.GetResolution();
	if (!FitsLand(_landLuminosityFrameBuffer, size))
	{
		_landLuminosityFrameBuffer =
		    std::make_unique<FrameBuffer>("LandLuminosity", size.x, size.y, graphics::TextureFormat::R8);
	}
	if (!FitsLand(_landShadeFrameBuffer, size))
	{
		_landShadeFrameBuffer = std::make_unique<FrameBuffer>("LandShade", size.x, size.y, graphics::TextureFormat::RGBA8);
	}
	const auto w = static_cast<float>(size.x);
	const auto h = static_cast<float>(size.y);
	const auto setUpView = [&size](RenderPass pass, const FrameBuffer& frameBuffer) {
		return SetUpLandView(pass, frameBuffer, size);
	};
	const auto& submitQuad = SubmitLandQuad;

	// What shades each cell this frame: the darkest of the clouds' shadows over it in red, from white, and the brightest
	// of the lights in alpha, from none
	const auto shadeView = setUpView(RenderPass::LandShade, *_landShadeFrameBuffer);
	bgfx::setViewClear(shadeView, BGFX_CLEAR_COLOR, 0xFF000000);
	const auto& shadeProgram = *_shaderManager->GetShader("LandShade");
	const auto& textures = Locator::resources::value().GetTextures();
	const glm::vec2 firstCell = island.GetExtent().minimum / LandIslandInterface::k_CellSize;

	// The clouds' shadows: each of the 40 by 40 cells under a cloud, from its corner, takes the darker of its luminosity
	// and the shadow's, by the cloud's alpha
	const auto shadowId = entt::hashed_string("raw/sclouds");
	if (detail_level::Clouds(Locator::config::value().detailLevel) && textures.Contains(shadowId.value()))
	{
		const auto shadow = textures.Handle(shadowId.value());
		constexpr float k_ShadowCells = 40.0f;
		const auto skyColour = clouds::Colour(Locator::alignmentSystem::value().GetSkyAlignment(), 0xFFFFFF);
		drawDesc.entities.Each<const ecs::components::Cloud, const ecs::components::Mist, const ecs::components::Transform>(
		    [&](const ecs::components::Cloud& /*unused*/, const ecs::components::Mist& mist,
		        const ecs::components::Transform& transform) {
			    const auto alpha = ((mist.colour >> 24u) * (skyColour >> 24u)) / 255u;
			    if (alpha == 0)
			    {
				    return;
			    }
			    // A texel's middle falls on each cell's corner
			    const glm::vec2 corner =
			        glm::vec2(transform.position.x, transform.position.z) / LandIslandInterface::k_CellSize - firstCell + 0.5f;
			    const float half = 0.5f / k_ShadowCells;
			    const glm::vec4 u_landShade {0.0f, static_cast<float>(alpha), 0.0f, 0.0f};
			    shadeProgram.SetTextureSampler("s_texture", 0, *shadow);
			    shadeProgram.SetUniformValue("u_landShade", &u_landShade);
			    submitQuad(shadeView, shadeProgram, corner, corner + (k_ShadowCells - 1.0f), glm::vec2(half),
			               glm::vec2(1.0f - half),
			               BGFX_STATE_WRITE_R | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) |
			                   BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MIN));
		    });
	}

	// The lights, once the land is dark enough: each stamps its image of brightness, a texel a cell from the cell its
	// point falls in, up to the second to last cell of the map
	const auto landColour = _landLightTable ? _landLightTable->GetLandColour() : 0xFFFFFFu;
	const auto stamp = [&](entt::id_type imageId, glm::vec2 xz, int32_t strength) {
		if (strength <= 0 || !textures.Contains(imageId))
		{
			return;
		}
		const auto image = textures.Handle(imageId);
		const auto side = static_cast<float>(image->GetResolution().x);
		const auto placement = village_lights::Place(xz);
		const auto origin = glm::vec2(placement.cell) - firstCell;
		constexpr float k_LastCell = static_cast<float>(LandIslandInterface::k_MapCellsPerSide - 1);
		const auto from = glm::max(origin, -firstCell);
		const auto to = glm::min(origin + (side - 1.0f), k_LastCell - firstCell);
		if (glm::any(glm::greaterThanEqual(from, to)))
		{
			return;
		}
		const glm::vec4 u_landShade {1.0f, static_cast<float>(strength), side, 0.0f};
		const glm::vec4 u_landStamp {glm::vec2(placement.weight), 0.0f, 0.0f};
		shadeProgram.SetTextureSampler("s_texture", 0, *image);
		shadeProgram.SetUniformValue("u_landShade", &u_landShade);
		shadeProgram.SetUniformValue("u_landStamp", &u_landStamp);
		submitQuad(shadeView, shadeProgram, from, to, from - origin, to - origin,
		           BGFX_STATE_WRITE_A | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) |
		               BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_MAX));
	};
	if (village_lights::IsDark(landColour))
	{
		// The hand's light, from the corner of its image, while the hand shows
		if (drawDesc.drawHand && Locator::handSystem::has_value())
		{
			const auto hand = Locator::handSystem::value()
			                      .GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
			if (const auto* transform = drawDesc.entities.TryGet<ecs::components::Transform>(hand); transform != nullptr)
			{
				stamp(village_lights::k_HandImageId.value(), HandLight::GetOrigin(transform->position),
				      village_lights::Strength(HandLight::GetStrength(landColour)));
			}
		}
		// The villages' lights by the hour, each from a little before it where it has flickered to
		const auto intensity = village_lights::Intensity(Locator::skySystem::value().GetClock().GetScriptTime());
		const auto strength = village_lights::VillageStrength(intensity);
		drawDesc.entities.Each<const ecs::components::VillageLight, const ecs::components::Transform>(
		    [&](const ecs::components::VillageLight& light, const ecs::components::Transform& transform) {
			    const glm::vec2 corner {(transform.position.x - village_lights::k_VillageReach) + light.flicker.x,
			                            (transform.position.z - village_lights::k_VillageReach) + light.flicker.y};
			    stamp(village_lights::k_VillageImageId.value(), corner, strength);
		    });
	}

	// The luminosity as the land was laid, under its shade
	const auto luminosityView = setUpView(RenderPass::LandLuminosity, *_landLuminosityFrameBuffer);
	const auto& luminosityProgram = *_shaderManager->GetShader("LandLuminosity");
	uint8_t fullLightGreen = 0xFF;
	if (_landLightTable)
	{
		fullLightGreen = static_cast<uint8_t>((_landLightTable->GetTexels().back() >> 8u) & 0xFFu);
	}
	const glm::vec4 u_landLuminosity {static_cast<float>(village_lights::Threshold(fullLightGreen)), 0.0f, 0.0f, 0.0f};
	luminosityProgram.SetTextureSampler("s_texture", 0, luminosity);
	luminosityProgram.SetTextureSampler("s_shade", 1, _landShadeFrameBuffer->GetColorAttachment());
	luminosityProgram.SetUniformValue("u_landLuminosity", &u_landLuminosity);
	submitQuad(luminosityView, luminosityProgram, {0.0f, 0.0f}, {w, h}, {0.0f, 0.0f}, {1.0f, 1.0f}, BGFX_STATE_WRITE_R);
}

const Texture2D& Renderer::GetLandColour() const
{
	if (_landColourFrameBuffer)
	{
		return _landColourFrameBuffer->GetColorAttachment();
	}
	return Locator::terrainSystem::value().GetCellColourMap();
}

void Renderer::UploadCreatureSkins(const DrawSceneDesc& drawDesc) const
{
	std::vector<entt::entity> seen;
	drawDesc.entities.Each<const ecs::components::CreatureSkin>([&](entt::entity entity,
	                                                                const ecs::components::CreatureSkin& skin) {
		if (skin.skins.empty())
		{
			return;
		}
		seen.push_back(entity);
		auto& entry = _creatureSkins[entity];
		if (entry.revision == skin.revision && entry.textures.size() == skin.skins.size())
		{
			return;
		}
		// The skins as painted this frame, before the body is drawn with them
		entry.textures.resize(skin.skins.size());
		entry.drawn.clear();
		for (size_t i = 0; i < skin.skins.size(); ++i)
		{
			const auto& painted = skin.skins[i];
			auto& texture = entry.textures[i];
			if (!texture)
			{
				texture = std::make_unique<Texture2D>("Creature Skin");
				texture->CreateWithinFrame(creature_tattoo::k_SkinSize, creature_tattoo::k_SkinSize, 1, TextureFormat::BGRA4,
				                           Wrapping::Repeat, Filter::Linear, nullptr);
			}
			texture->Update(painted.texels.data(), static_cast<uint32_t>(painted.texels.size() * sizeof(painted.texels[0])));
			entry.drawn.emplace_back(painted.id, texture.get());
		}
		entry.revision = skin.revision;
	});
	std::erase_if(_creatureSkins, [&seen](const auto& entry) { return std::ranges::find(seen, entry.first) == seen.end(); });
}

void Renderer::DrawSkyDomePass(const DrawSceneDesc& drawDesc) const
{
	if (!drawDesc.drawSky || !Locator::skySystem::has_value())
	{
		return;
	}
	auto& sky = Locator::skySystem::value();
	const auto frame = sky.AdvanceDome();
	if (frame.Get().empty())
	{
		return;
	}
	constexpr uint16_t k_Alignments = 3;
	constexpr glm::u16vec2 k_Size {sky_dome::k_Rows, sky_dome::k_Rows * k_Alignments};
	if (!_skyDomeFrameBuffer)
	{
		_skyDomeFrameBuffer = std::make_unique<FrameBuffer>("SkyDome", k_Size.x, k_Size.y, graphics::TextureFormat::RGBA8,
		                                                    std::nullopt, 1, Wrapping::ClampEdge);
	}
	const auto viewId = SetUpLandView(RenderPass::SkyDome, *_skyDomeFrameBuffer, k_Size);
	const auto& program = *_shaderManager->GetShader("SkyDome");
	constexpr auto k_Rows = static_cast<float>(sky_dome::k_Rows);
	for (const auto& rows : frame.Get())
	{
		const auto times = sky_dome::TimePair(rows.skyType);
		const auto first = static_cast<float>(rows.first);
		const auto last = static_cast<float>(rows.first + rows.count);
		for (uint16_t alignment = 0; alignment < k_Alignments; ++alignment)
		{
			// The pictures are laid out a layer for each time of day within each alignment
			const glm::vec4 u_skyDome {alignment * 3, times.lower, times.upper, times.weight};
			program.SetTextureSampler("s_diffuse", 0, sky.GetTexture());
			program.SetUniformValue("u_skyDome", &u_skyDome);
			const auto top = static_cast<float>(alignment) * k_Rows;
			SubmitLandQuad(viewId, program, {0.0f, top + first}, {k_Rows, top + last}, {0.0f, first / k_Rows},
			               {1.0f, last / k_Rows}, BGFX_STATE_WRITE_RGB);
		}
	}
}

void Renderer::DrawLandColourPass(const DrawSceneDesc& drawDesc) const
{
	if (!drawDesc.drawIsland || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto& island = Locator::terrainSystem::value();
	const auto& cellColours = island.GetCellColourMap();
	const auto size = cellColours.GetResolution();
	if (!FitsLand(_landColourFrameBuffer, size))
	{
		_landColourFrameBuffer = std::make_unique<FrameBuffer>("LandColour", size.x, size.y, graphics::TextureFormat::RGBA8);
	}
	const auto viewId = SetUpLandView(RenderPass::LandColour, *_landColourFrameBuffer, size);
	const auto& program = *_shaderManager->GetShader("LandColour");

	// The cells' colours as the land was laid
	const glm::vec4 u_copy {0.0f};
	program.SetTextureSampler("s_texture", 0, cellColours);
	program.SetUniformValue("u_landColourStamp", &u_copy);
	SubmitLandQuad(viewId, program, {0.0f, 0.0f}, glm::vec2(size), {0.0f, 0.0f}, {1.0f, 1.0f}, BGFX_STATE_WRITE_RGB);

	// The frame's stamps over them, in order, up to the game's most
	if (!_lightningGlowTexture)
	{
		const auto image = land_colour_stamps::LightningImage();
		constexpr auto k_Side = static_cast<uint16_t>(land_colour_stamps::k_LightningSide);
		_lightningGlowTexture = fromBgfx(bgfx::createTexture2D(k_Side, k_Side, false, 1, bgfx::TextureFormat::RGB8,
		                                                       BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_POINT,
		                                                       bgfx::copy(image.data(), static_cast<uint32_t>(image.size()))));
		bgfx::setName(toBgfx(*_lightningGlowTexture), "Lightning Glow");
	}
	const glm::vec2 firstCell = island.GetExtent().minimum / LandIslandInterface::k_CellSize;
	size_t stamps = 0;
	const auto stamp = [&](TextureHandle image, int32_t side, glm::vec2 corner, uint8_t strength,
	                       land_colour_stamps::Combine combine) {
		if (strength == 0 || stamps >= land_colour_stamps::k_MaxStamps)
		{
			return;
		}
		++stamps;
		const auto placement = land_colour_stamps::Place(corner);
		const auto origin = glm::vec2(placement.cell) - firstCell;
		constexpr float k_LastCell = static_cast<float>(LandIslandInterface::k_MapCellsPerSide - 1);
		const auto from = glm::max(origin, -firstCell);
		const auto to = glm::min(origin + static_cast<float>(side - 1), k_LastCell - firstCell);
		if (glm::any(glm::greaterThanEqual(from, to)))
		{
			return;
		}
		const glm::vec4 u_landColourStamp {1.0f, static_cast<float>(strength), static_cast<float>(side), 0.0f};
		const glm::vec4 u_landColourWeights {glm::vec2(placement.weight), 0.0f, 0.0f};
		program.SetTextureSampler("s_texture", 0, image);
		program.SetUniformValue("u_landColourStamp", &u_landColourStamp);
		program.SetUniformValue("u_landColourWeights", &u_landColourWeights);
		// An added stamp brightens the cells to at most white; another keeps the brighter colour
		const auto equation =
		    combine == land_colour_stamps::Combine::Add ? BGFX_STATE_BLEND_EQUATION_ADD : BGFX_STATE_BLEND_EQUATION_MAX;
		SubmitLandQuad(viewId, program, from, to, from - origin, to - origin,
		               BGFX_STATE_WRITE_RGB | BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_ONE) |
		                   BGFX_STATE_BLEND_EQUATION(equation));
	};

	// Each storm's lightning glows on the ground around where it struck
	drawDesc.entities.Each<const ecs::components::Storm>([&](const ecs::components::Storm& storm) {
		if (storm.dead)
		{
			return;
		}
		const auto strength = land_colour_stamps::Strength(lightning::GlowStrength(storm.flash));
		stamp(*_lightningGlowTexture, land_colour_stamps::k_LightningSide,
		      land_colour_stamps::CentredCorner(storm.flash.position, land_colour_stamps::k_LightningSide), strength,
		      land_colour_stamps::Combine::Add);
	});
}

void Renderer::DrawLandAlphaPass(const DrawSceneDesc& drawDesc) const
{
	if (!drawDesc.drawIsland)
	{
		return;
	}
	const auto viewId = static_cast<bgfx::ViewId>(RenderPass::LandAlpha);
	const auto& island = Locator::terrainSystem::value();
	const auto& frameBuffer = island.GetLandAlphaFramebuffer();
	frameBuffer.Bind(RenderPass::LandAlpha);
	uint16_t width = 0;
	uint16_t height = 0;
	frameBuffer.GetSize(width, height);
	bgfx::setViewRect(viewId, 0, 0, width, height);
	// The land is opaque but where a channel runs
	bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR, 0xFFFFFFFF);
	bgfx::touch(viewId);
	const auto view = island.GetOrthoView();
	const auto proj = island.GetOrthoProj();
	bgfx::setViewTransform(viewId, &view, &proj);
	DrawStreamFootprints(RenderPass::LandAlpha, ecs::components::StreamSegment::k_ChannelMeshId);
}

void Renderer::DrawObjectShadowPass(const DrawSceneDesc& drawDesc) const
{
	const auto viewId = static_cast<bgfx::ViewId>(RenderPass::ObjectShadow);
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::ObjectShadowPass);
	if (!drawDesc.drawIsland)
	{
		return;
	}

	// The same texels as the footprints, so that the land finds both at the same coordinates
	const auto& island = Locator::terrainSystem::value();
	uint16_t width = 0;
	uint16_t height = 0;
	island.GetFootprintFramebuffer().GetSize(width, height);
	uint16_t shadowWidth = 0;
	uint16_t shadowHeight = 0;
	if (_objectShadowFrameBuffer)
	{
		_objectShadowFrameBuffer->GetSize(shadowWidth, shadowHeight);
	}
	if (shadowWidth != width || shadowHeight != height)
	{
		_objectShadowFrameBuffer = std::make_unique<FrameBuffer>("Object Shadows", width, height, TextureFormat::R8,
		                                                         std::nullopt, static_cast<uint8_t>(1), Wrapping::ClampEdge);
	}

	_objectShadowFrameBuffer->Bind(RenderPass::ObjectShadow);
	bgfx::setViewRect(viewId, 0, 0, width, height);
	bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR, 0x00000000);
	bgfx::touch(viewId);
	if (!drawDesc.drawEntities)
	{
		return;
	}

	const auto view = island.GetOrthoView();
	const auto proj = island.GetOrthoProj();
	bgfx::setViewTransform(viewId, &view, &proj);

	const auto* shader = _shaderManager->GetShader("ObjectShadowInstanced");
	const auto sun = glm::vec4(ObjectShadows::k_Sun, 0.0f);
	shader->SetUniformValue("u_shadowSun", &sun);

	const auto& meshManager = Locator::resources::value().GetMeshes();
	const auto& renderCtx = Locator::rendereringSystem::value().GetContext();
	L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = RenderPass::ObjectShadow;
	submitDesc.program = shader;
	// Overlapping shadows cover the same texels, both sides of every triangle cast
	submitDesc.state = BGFX_STATE_WRITE_R;

	const auto drawCasters = [&](const std::map<entt::id_type, RenderContext::InstancedDrawDesc>& descs,
	                             const graphics::DynamicVertexBufferHandle& instances) {
		for (const auto& [meshId, placers] : descs)
		{
			if (!placers.castsShadow || placers.count == 0 || !meshManager.Contains(meshId))
			{
				continue;
			}
			const auto mesh = meshManager.Handle(meshId);
			submitDesc.instanceDesc = std::make_unique<graphics::InstanceDesc>(instances, placers.offset, placers.count);
			const static auto identity = glm::mat4(1.0f);
			submitDesc.modelMatrices = &identity;
			submitDesc.matrixCount = 1;
			if (mesh->IsBoned() && !mesh->GetBoneMatrices().empty())
			{
				submitDesc.modelMatrices = mesh->GetBoneMatrices().data();
				submitDesc.matrixCount = static_cast<uint8_t>(mesh->GetBoneMatrices().size());
			}
			DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
		}
	};
	drawCasters(renderCtx.instancedDrawDescs, renderCtx.instanceUniformBuffer);
	if (drawDesc.drawVegetation)
	{
		drawCasters(renderCtx.treeInstancedDrawDescs, renderCtx.treeInstanceUniformBuffer);
	}
}

TextureHandle Renderer::UpdateLandLight() const
{
	if (!_landLightTable)
	{
		_landLightTable = std::make_unique<LandLightTable>();
		_landLightTexture = fromBgfx(bgfx::createTexture2D(LandLightTable::k_Size, 1, false, 1, bgfx::TextureFormat::RGBA8,
		                                                   BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_POINT));
		bgfx::setName(toBgfx(*_landLightTexture), "Land Light");
	}
	if (const auto& palettes = Locator::resources::value().GetLandLightPalettes();
	    palettes.Contains(LandLightPalette::k_Id.value()))
	{
		// The clouds over the camera darken the land, and so does a flash of lightning
		const auto [skyType, alignment, overcast, flash] = FrameLandLightInputs();
		_landLightTable->Build(*palettes.Handle(LandLightPalette::k_Id.value()), skyType, alignment, overcast, flash);
		const auto& haze = _landLightTable->GetHaze();
		_haze = {glm::vec4(haze.nearDistance, haze.farDistance, haze.k, 1.0f), glm::vec4(haze.colour, 0.0f)};
		const auto detailLevel = Locator::config::value().detailLevel;
		_overcast = overcast;
		_skyTint = sky_dome::TintOf({
		    .hazeColour = haze.colour,
		    .overcast = overcast,
		    .flash = flash,
		    .darkness = sky_dome::Darkness(alignment + 1.0f),
		    .fog = detail_level::Fog(detailLevel),
		    .weather = detail_level::Weather(detailLevel),
		});
		const auto& texels = _landLightTable->GetTexels();
		bgfx::updateTexture2D(toBgfx(*_landLightTexture), 0, 0, 0, 0, LandLightTable::k_Size, 1,
		                      bgfx::copy(texels.data(), static_cast<uint32_t>(texels.size() * sizeof(texels[0]))));
	}
	return *_landLightTexture;
}

glm::vec4 Renderer::GetModelLight() const
{
	// By day, or with no hand to carry it, the sun
	auto light = model_light::k_Sun;
	if (Locator::handSystem::has_value() && Locator::skySystem::has_value() && Locator::camera::has_value())
	{
		const auto handEntity =
		    Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
		if (const auto* transform = Locator::entitiesRegistry::value().TryGet<ecs::components::Transform>(handEntity);
		    transform != nullptr)
		{
			const auto ground =
			    Locator::terrainSystem::has_value()
			        ? Locator::terrainSystem::value().GetHeightAt(glm::vec2(transform->position.x, transform->position.z))
			        : 0.0f;
			const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
			light = model_light::FrameLight(transform->position, ground, Locator::camera::value().GetOrigin(),
			                                Locator::skySystem::value().GetCurrentSkyType(), inTemple);
		}
	}
	return model_light::Uniform(light);
}

TextureHandle Renderer::GetLandLightTexture() const
{
	return _landLightTexture.value_or(_handShadowFrameBuffer->GetColorAttachment().GetNativeHandle());
}

void Renderer::DrawHandShadowPass(const DrawSceneDesc& drawDesc) const
{
	const auto viewId = static_cast<bgfx::ViewId>(RenderPass::HandShadow);
	_handShadow.reset();

	_handShadowFrameBuffer->Bind(RenderPass::HandShadow);
	bgfx::setViewRect(viewId, 0, 0, HandShadow::k_TextureSize, HandShadow::k_TextureSize);
	bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR, 0x00000000);
	bgfx::touch(viewId);

	if (!drawDesc.drawEntities || !drawDesc.drawIsland || !Locator::handSystem::has_value() ||
	    !Locator::skySystem::has_value() || !Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto handEntity =
	    Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
	const auto& registry = Locator::entitiesRegistry::value();
	const auto [hand, mesh, transform] =
	    registry.TryGet<ecs::components::Hand, ecs::components::Mesh, ecs::components::Transform>(handEntity);
	if (hand == nullptr || mesh == nullptr || transform == nullptr)
	{
		return;
	}
	const auto& meshManager = Locator::resources::value().GetMeshes();
	if (!meshManager.Contains(mesh->id))
	{
		return;
	}
	const auto handMesh = meshManager.Handle(mesh->id);
	const auto& modelBones = hand->boneMatrices.empty() ? handMesh->GetBoneMatrices() : hand->boneMatrices;

	// The hand's bones in the world, as the instanced draw of the main pass places them
	const auto model = glm::translate(transform->position) * glm::mat4(transform->rotation) * glm::scale(transform->scale);
	std::vector<glm::mat4> bones;
	bones.reserve(modelBones.size());
	for (const auto& bone : modelBones)
	{
		bones.push_back(model * bone);
	}

	const auto& sky = Locator::skySystem::value();
	const auto groundHeight =
	    Locator::terrainSystem::value().GetHeightAt(glm::vec2(transform->position.x, transform->position.z));
	const auto* caps = bgfx::getCaps();
	_handShadow = HandShadow::Compute(bones, drawDesc.camera->GetOrigin(), groundHeight, sky.GetTime(), sky.GetDayNightTimes(),
	                                  caps->originBottomLeft, caps->homogeneousDepth);
	if (!_handShadow)
	{
		return;
	}

	bgfx::setViewTransform(viewId, &_handShadow->view, &_handShadow->projection);
	L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = RenderPass::HandShadow;
	submitDesc.program = _shaderManager->GetShader("ShadowCaster");
	// Only the silhouette matters: no depth and both sides
	submitDesc.state = BGFX_STATE_WRITE_R;
	submitDesc.modelMatrices = bones.data();
	submitDesc.matrixCount = static_cast<uint8_t>(bones.size());
	DrawMesh(*handMesh, submitDesc, std::numeric_limits<uint8_t>::max());
}

void Renderer::DrawCreatureShadowPass(const DrawSceneDesc& drawDesc) const
{
	auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::CreatureShadowPass);
	const auto viewId = static_cast<bgfx::ViewId>(RenderPass::CreatureShadow);
	_creatureShadowCount = 0;
	uint16_t width = 0;
	uint16_t height = 0;
	_creatureShadowFrameBuffer->GetSize(width, height);
	_creatureShadowFrameBuffer->Bind(RenderPass::CreatureShadow);
	bgfx::setViewRect(viewId, 0, 0, width, height);
	bgfx::setViewClear(viewId, BGFX_CLEAR_COLOR, 0x00000000);
	bgfx::touch(viewId);

	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	if (!Locator::config::value().drawCreatureShadows || !drawDesc.drawEntities || !drawDesc.drawIsland || inTemple ||
	    !Locator::terrainSystem::has_value())
	{
		return;
	}
	// Each silhouette's matrices are folded into its bones, so the view itself places nothing
	const auto identity = glm::mat4(1.0f);
	bgfx::setViewTransform(viewId, &identity, &identity);

	const auto& meshManager = Locator::resources::value().GetMeshes();
	const auto& renderCtx = Locator::rendereringSystem::value().GetContext();
	const auto& land = Locator::terrainSystem::value();
	const auto camera = drawDesc.camera->GetOrigin();
	struct Caster
	{
		entt::entity entity;
		const L3DMesh* mesh;
		glm::mat4 model;
		glm::vec3 centre;
		float radius;
		float distance;
	};
	std::vector<Caster> casters;
	for (const auto& [entity, instance] : renderCtx.entityDraws)
	{
		const auto* mesh = drawDesc.entities.TryGet<const ecs::components::Mesh>(entity);
		if (mesh == nullptr || !drawDesc.entities.AnyOf<ecs::components::Creature>(entity) || !meshManager.Contains(mesh->id) ||
		    instance >= renderCtx.instanceUniforms.size())
		{
			continue;
		}
		const auto* body = &*meshManager.Handle(mesh->id);
		const auto& model = renderCtx.instanceUniforms[instance].model;
		const auto box = body->GetBoundingBox();
		const auto scale =
		    std::max({glm::length(glm::vec3(model[0])), glm::length(glm::vec3(model[1])), glm::length(glm::vec3(model[2]))});
		const auto centre = glm::vec3(model * glm::vec4(box.Center(), 1.0f));
		casters.push_back({.entity = entity,
		                   .mesh = body,
		                   .model = model,
		                   .centre = centre,
		                   .radius = glm::length(box.Size()) * 0.5f * scale,
		                   .distance = glm::distance(camera, centre)});
	}
	std::ranges::sort(casters, {}, &Caster::distance);

	// Cast from the scene's light, as the bodies are lit
	const auto light = glm::vec3(GetModelLight());
	const auto* caps = bgfx::getCaps();
	L3DMeshSubmitDesc submitDesc = {};
	submitDesc.viewId = RenderPass::CreatureShadow;
	submitDesc.program = _shaderManager->GetShader("ShadowCaster");
	// Only the silhouette matters: no depth and both sides
	submitDesc.state = BGFX_STATE_WRITE_R;
	std::vector<glm::mat4> bones;
	for (const auto& caster : casters)
	{
		if (_creatureShadowCount >= CreatureShadow::k_MaxShadows)
		{
			break;
		}
		const auto ground = land.GetHeightAt(glm::vec2(caster.centre.x, caster.centre.z));
		const auto shadow =
		    CreatureShadow::Compute(caster.centre, caster.radius, ground, light, camera, _creatureShadowCount,
		                            CreatureShadow::k_MaxShadows, caps->originBottomLeft, caps->homogeneousDepth);
		if (!shadow)
		{
			continue;
		}
		const auto* animation = drawDesc.entities.TryGet<const ecs::components::CreatureAnimation>(caster.entity);
		const auto& pose = animation != nullptr && animation->boneMatrices.size() == caster.mesh->GetBoneMatrices().size()
		                       ? animation->boneMatrices
		                       : caster.mesh->GetBoneMatrices();
		if (pose.empty())
		{
			continue;
		}
		const auto toCell = CreatureShadow::CellMatrix(_creatureShadowCount, CreatureShadow::k_MaxShadows) *
		                    shadow->projection * shadow->view * caster.model;
		bones.clear();
		for (const auto& bone : pose)
		{
			bones.push_back(toCell * bone);
		}
		submitDesc.modelMatrices = bones.data();
		submitDesc.matrixCount = static_cast<uint8_t>(bones.size());
		DrawMesh(*caster.mesh, submitDesc, std::numeric_limits<uint8_t>::max());

		_creatureShadowMatrices.at(_creatureShadowCount) = shadow->receiverMatrix;
		_creatureShadowParameters.at(_creatureShadowCount) =
		    glm::vec4(shadow->strength * HandShadow::k_MaxDarkness, shadow->startDepth, 0.0f, 0.0f);
		++_creatureShadowCount;
	}
}

void Renderer::SetCreatureShadowUniforms(const ShaderProgram& program, bool receives) const
{
	uint16_t width = 0;
	uint16_t height = 0;
	_creatureShadowFrameBuffer->GetSize(width, height);
	const auto count = receives ? _creatureShadowCount : uint8_t {0};
	const glm::vec4 u_creatureShadowInfo {static_cast<float>(count), static_cast<float>(CreatureShadow::k_MaxShadows),
	                                      1.0f / static_cast<float>(std::max<uint16_t>(width, 1)),
	                                      1.0f / static_cast<float>(std::max<uint16_t>(height, 1))};
	program.SetUniformValue("u_creatureShadowInfo", &u_creatureShadowInfo);
	program.SetTextureSampler("s_creatureShadows", 14, _creatureShadowFrameBuffer->GetColorAttachment());
	if (count > 0)
	{
		program.SetUniformArray("u_creatureShadowMatrix", _creatureShadowMatrices.data(), count);
		program.SetUniformArray("u_creatureShadow", _creatureShadowParameters.data(), count);
	}
}

void Renderer::DrawScene(const DrawSceneDesc& drawDesc) const noexcept
{
	UploadCreatureSkins(drawDesc);
	// TODO(bwrsandman): Footprint framebuffer doesn't need to be updated each frame
	DrawLandLuminosityPass(drawDesc);
	DrawLandColourPass(drawDesc);
	DrawSkyDomePass(drawDesc);
	DrawFootprintPass(drawDesc);
	DrawLandAlphaPass(drawDesc);
	UpdateLandLight();
	DrawObjectShadowPass(drawDesc);
	DrawTempleMapPass();
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::MainPassDrawModels);
		if (drawDesc.drawHand)
		{
			DrawHandShadowPass(drawDesc);
		}
		else
		{
			_handShadow.reset();
		}
	}
	DrawCreatureShadowPass(drawDesc);
	// Reflection Pass
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::ReflectionPass);
		const auto& instancedDrawDescs = Locator::rendereringSystem::value().GetContext().instancedDrawDescs;
		const bool showsReflection = drawDesc.drawEntities && std::ranges::any_of(instancedDrawDescs, [](const auto& entry) {
			                             return entry.second.showsReflection;
		                             });
		if (drawDesc.drawWater || showsReflection)
		{
			DrawSceneDesc drawPassDesc = drawDesc;

			const auto& frameBuffer = Locator::oceanSystem::value().GetReflectionFramebuffer();
			auto reflectionCamera = drawDesc.camera->Reflect();

			drawPassDesc.viewId = graphics::RenderPass::Reflection;
			drawPassDesc.camera = reflectionCamera.get();
			drawPassDesc.frameBuffer = &frameBuffer;
			drawPassDesc.drawWater = false;
			drawPassDesc.drawBoundingBoxes = false;
			drawPassDesc.cullBack = true;
			// The game mirrors the land under the sea without its small bump detail. Unlike the game, which mirrors
			// only the sky, the land and a few moving things, everything is mirrored here.
			drawPassDesc.smallBumpMapStrength = 0.0f;

			DrawPass(drawPassDesc);
		}
	}

	// Main Draw Pass
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::MainPass);
		DrawPass(drawDesc);
	}
}

void Renderer::DrawPass(const DrawSceneDesc& desc) const
{
	const auto& meshManager = Locator::resources::value().GetMeshes();
	auto& profiler = Locator::profiler::value();

	// The sky is drawn first, in its own pass into the same target, in the order the game draws it: the dome, the sun
	// and the moon. The scene's pass then draws everything else over it.
	const auto skyViewId = SkyPassOf(desc.viewId);
	if (desc.frameBuffer != nullptr)
	{
		desc.frameBuffer->Bind(desc.viewId);
		desc.frameBuffer->Bind(skyViewId);
	}
	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(skyViewId));
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));
	bgfx::setViewMode(static_cast<bgfx::ViewId>(skyViewId), bgfx::ViewMode::Sequential);
	// bgfx sorts a view's blended draws by their programs unless told to keep them in order. The game draws the temple
	// in the order it is submitted, and its blended parts must be too: the pool's water, drawn with the lightmap
	// program, would otherwise land after the hand, whose faded wrist writes depth, and leave a hole in the water
	// where the wrist should show it through.
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	bgfx::setViewMode(static_cast<bgfx::ViewId>(desc.viewId), inTemple ? bgfx::ViewMode::Sequential : bgfx::ViewMode::Default);
	// Outside the temple, what blends in the world is drawn after the rest of the scene in a pass of its own, all of it
	// together and the farthest first, as the game sorts it
	const auto translucentViewId = TranslucentView(desc.viewId);
	if (translucentViewId != desc.viewId)
	{
		if (desc.frameBuffer != nullptr)
		{
			desc.frameBuffer->Bind(translucentViewId);
		}
		bgfx::setViewMode(static_cast<bgfx::ViewId>(translucentViewId), bgfx::ViewMode::DepthDescending);
		_shaderManager->SetCamera(translucentViewId, *desc.camera);
	}
	const auto cameraOrigin = desc.camera->GetOrigin();

	_shaderManager->SetCamera(skyViewId, *desc.camera);
	_shaderManager->SetCamera(desc.viewId, *desc.camera);
	_modelLight = GetModelLight();
	if (Locator::camera::has_value())
	{
		const auto& camera = Locator::camera::value();
		_treeBrightness =
		    static_cast<float>(tree_brightness::Factor(camera.GetFocus(), camera.GetForward(), glm::vec3(_modelLight))) /
		    256.0f;
	}

	const auto* skyShader = _shaderManager->GetShader("Sky");
	const auto* waterShader = _shaderManager->GetShader("Water");
	const auto* terrainShader = _shaderManager->GetShader("Terrain");
	const auto* debugShader = _shaderManager->GetShader("DebugLine");
	// Trees are placed on the land already, so they are drawn where they are rather than moved onto the height map
	const auto* vegetationShaderInstanced = _shaderManager->GetShader("Vegetation");
	const auto* spriteShader = _shaderManager->GetShader("Sprite");
	const auto* debugShaderInstanced = _shaderManager->GetShader("DebugLineInstanced");
	const auto* objectShaderInstanced = _shaderManager->GetShader("ObjectInstanced");
	const auto* objectShaderMorphInstanced = _shaderManager->GetShader("ObjectMorphInstanced");
	const auto* objectShaderStaticInstanced = _shaderManager->GetShader("ObjectStaticInstanced");
	const auto* objectShaderHeightMapInstanced = _shaderManager->GetShader("ObjectHeightMapInstanced");
	const auto* objectShaderLightmapInstanced = _shaderManager->GetShader("ObjectLightmapInstanced");
	const auto* objectShaderReflectiveLightmapInstanced = _shaderManager->GetShader("ObjectReflectiveLightmapInstanced");

	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();

	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSky
		                                                                          : Profiler::Stage::MainPassDrawSky);
		if (desc.drawSky && _skyDomeFrameBuffer)
		{
			const auto modelMatrix = glm::mat4(1.0f);
			// The sky's alignment from 0, evil, to 2, good, mixes two of the alignments' domes
			const auto alignments = sky_dome::AlignmentPair(Locator::alignmentSystem::value().GetSkyAlignment() + 1.0f);
			const glm::vec4 u_skyAlignment {alignments.lower, alignments.upper, alignments.weight, 0.0f};

			skyShader->SetTextureSampler("s_diffuse", 0, _skyDomeFrameBuffer->GetColorAttachment());
			skyShader->SetUniformValue("u_skyAlignment", &u_skyAlignment);
			const glm::vec4 u_skyModulate {glm::vec3(_skyTint.modulate) / 255.0f, 1.0f};
			const glm::vec4 u_skyAdd {glm::vec3(_skyTint.add) / 255.0f, 0.0f};
			skyShader->SetUniformValue("u_skyModulate", &u_skyModulate);
			skyShader->SetUniformValue("u_skyAdd", &u_skyAdd);

			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = skyViewId;
			submitDesc.program = skyShader;
			submitDesc.state = k_BgfxDefaultStateInvertedZ;
			if (!desc.cullBack)
			{
				submitDesc.state &= ~BGFX_STATE_CULL_MASK;
				submitDesc.state |= BGFX_STATE_CULL_CCW;
			}
			submitDesc.modelMatrices = &modelMatrix;
			submitDesc.matrixCount = 1;
			submitDesc.isSky = true;

			DrawMesh(Locator::skySystem::value().GetMesh(), submitDesc, 0);
			DrawSun(skyViewId);
			DrawMoon(skyViewId);
		}
	}

	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawWater
		                                                                          : Profiler::Stage::MainPassDrawWater);
		if (desc.drawWater)
		{
			const auto& ocean = Locator::oceanSystem::value();
			const auto& mesh = ocean.GetMesh();
			mesh.GetIndexBuffer().Bind(mesh.GetIndexBuffer().GetCount(), 0);
			mesh.GetVertexBuffer().Bind();
			bgfx::setState(k_BgfxDefaultStateInvertedZ);
			auto diffuse = Locator::resources::value().GetTextures().Handle(ocean.GetDiffuseTexture());
			auto alpha = Locator::resources::value().GetTextures().Handle(ocean.GetAlphaTexture());
			waterShader->SetTextureSampler("s_diffuse", 0, *diffuse);
			waterShader->SetTextureSampler("s_alpha", 1, *alpha);
			waterShader->SetTextureSampler("s_reflection", 2, ocean.GetReflectionFramebuffer().GetColorAttachment());
			SetSeaUniforms(*waterShader, *desc.camera);
			waterShader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
		}
	}

	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawIsland
		                                                                          : Profiler::Stage::MainPassDrawIsland);
		if (desc.drawIsland)
		{
			auto& island = Locator::terrainSystem::value();
			auto islandExtent = glm::vec4(island.GetExtent().minimum, island.GetExtent().maximum);

			const auto& textures = Locator::resources::value().GetTextures();
			auto smallBump = textures.Handle(LandIslandInterface::k_SmallBumpTextureId);
			auto smallBumpAlpha = textures.Handle(LandIslandInterface::k_SmallBumpAlphaTextureId);
			// The land mirrored under the sea is lit at half
			const float lightScale = desc.viewId == RenderPass::Reflection ? 0.5f : 1.0f;
			const glm::vec4 u_skyAndBump = {skyType, lightScale, desc.smallBumpMapStrength, 0.0f};

			// The small bump detail fades out about a line across the ground: where the plane square to the camera's
			// view, 50 units ahead of it, meets the ground at the camera's height, or at 110.55 if the camera is higher
			const auto cameraForward = desc.camera->GetForward();
			const glm::vec2 forwardAlongGround {cameraForward.x, cameraForward.z};
			const float forwardLength = std::max(glm::length(forwardAlongGround), 1e-4f);
			constexpr float k_SmallBumpDistance = 50.0f;
			constexpr float k_SmallBumpHighestGround = 0.67f * 165.0f;
			const float lineDistance =
			    (k_SmallBumpDistance + cameraForward.y * std::max(0.0f, cameraOrigin.y - k_SmallBumpHighestGround)) /
			    forwardLength;
			const glm::vec4 u_smallBumpLine {cameraOrigin.x, cameraOrigin.z, forwardAlongGround / forwardLength};
			const glm::vec4 u_smallBump {lineDistance, 0.0f, 0.0f, 0.0f};
			auto u_objectShadows = glm::vec4(0.0f);
			if (_objectShadowFrameBuffer)
			{
				uint16_t width = 0;
				uint16_t height = 0;
				_objectShadowFrameBuffer->GetSize(width, height);
				u_objectShadows =
				    glm::vec4(ObjectShadows::k_MaxDarkness, 1.0f / static_cast<float>(std::max<uint16_t>(width, 1)),
				              1.0f / static_cast<float>(std::max<uint16_t>(height, 1)), 0.0f);
			}

			// The hand's shadow falls on the land, not on its reflection
			const auto& handShadow = desc.viewId == RenderPass::Main ? _handShadow : std::nullopt;
			const auto handShadowMatrix = handShadow ? handShadow->receiverMatrix : glm::mat4(0.0f);
			const auto u_handShadow =
			    handShadow ? glm::vec4(handShadow->strength * HandShadow::k_MaxDarkness, handShadow->startDepth, 0.0f, 0.0f)
			               : glm::vec4(0.0f);

			// The hand's light is in the land's own lighting, so the reflection shows it too

			// clang-format off
			// The land and its small bump detail come premultiplied over the sea
			constexpr auto defaultState = 0u
				| BGFX_STATE_WRITE_MASK
				| BGFX_STATE_DEPTH_TEST_GREATER
				| BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA)
				| BGFX_STATE_MSAA
			;

			constexpr auto discard = 0u
				| BGFX_DISCARD_INSTANCE_DATA
				| BGFX_DISCARD_INDEX_BUFFER
				| BGFX_DISCARD_TRANSFORM
				| BGFX_DISCARD_VERTEX_STREAMS
				| BGFX_DISCARD_STATE
			;
			// clang-format on

			// The snow lying on the land, when there is its noise to draw it with
			const auto setTerrainSnow = [&]() {
				const auto& textures = Locator::resources::value().GetTextures();
				const auto* depth = SnowDepth(_snowDepth, _snowRevision);
				const bool snowed = depth != nullptr && textures.Contains(snow_cover::k_NoiseTextureId.value());
				const glm::vec4 u_snow {snowed ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
				terrainShader->SetUniformValue("u_snow", &u_snow);
				if (snowed)
				{
					terrainShader->SetTextureSampler("s_snowDepth", 11, *depth);
					terrainShader->SetTextureSampler("s12_snowNoise", 12,
					                                 *textures.Handle(snow_cover::k_NoiseTextureId.value()));
				}
			};
			// bgfx keeps a uniform for the draws after it in the order they come, but the blocks are drawn in another
			// order, so each block sets them all
			const auto setTerrainUniforms = [&]() {
				terrainShader->SetTextureSampler("s0_blockTextures", 0, island.GetBlockTextures());
				terrainShader->SetTextureSampler("s9_landLuminosity", 9, GetLandLuminosity());
				terrainShader->SetTextureSampler("s10_landColour", 10, GetLandColour());
				terrainShader->SetTextureSampler("s1_smallBumpAlpha", 1, *smallBumpAlpha);
				terrainShader->SetTextureSampler("s8_landAlpha", 8, island.GetLandAlphaFramebuffer().GetColorAttachment());
				terrainShader->SetTextureSampler("s2_smallBump", 2, *smallBump);
				terrainShader->SetUniformValue("u_smallBumpLine", &u_smallBumpLine);
				terrainShader->SetUniformValue("u_smallBump", &u_smallBump);
				terrainShader->SetTextureSampler("s3_footprints", 3, island.GetFootprintFramebuffer().GetColorAttachment());
				terrainShader->SetTextureSampler("s5_objectShadows", 5,
				                                 _objectShadowFrameBuffer
				                                     ? _objectShadowFrameBuffer->GetColorAttachment()
				                                     : island.GetFootprintFramebuffer().GetColorAttachment());
				terrainShader->SetTextureSampler("s7_landLight", 7, GetLandLightTexture());
				terrainShader->SetUniformValue("u_haze", &_haze[0]);
				terrainShader->SetUniformValue("u_hazeColour", &_haze[1]);
				terrainShader->SetUniformValue("u_skyAndBump", &u_skyAndBump);
				terrainShader->SetUniformValue("u_objectShadows", &u_objectShadows);
				terrainShader->SetUniformValue("u_islandExtent", &islandExtent);
				terrainShader->SetTextureSampler("s4_handShadow", 4, _handShadowFrameBuffer->GetColorAttachment());
				terrainShader->SetUniformValue("u_handShadowMatrix", &handShadowMatrix);
				terrainShader->SetUniformValue("u_handShadow", &u_handShadow);
				// The creatures' shadows fall on the land, not on its reflection
				SetCreatureShadowUniforms(*terrainShader, desc.viewId == RenderPass::Main);
				setTerrainSnow();
			};

			for (size_t i = 0; const auto& block : island.GetBlocks())
			{
				setTerrainUniforms();
				// pack uniforms
				const glm::vec4 mapPositionAndSize = glm::vec4(block.GetMapPosition(), 160.0f, 160.0f);
				terrainShader->SetUniformValue("u_blockPositionAndSize", &mapPositionAndSize);
				const glm::vec4 u_block {static_cast<float>(i++), 0.0f, 0.0f, 0.0f};
				terrainShader->SetUniformValue("u_block", &u_block);

				block.BindVertices();

				bgfx::setState(defaultState | (desc.cullBack ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW), 0);
				// The game draws the land's blocks the nearest first, which bgfx keeps by their distance
				const auto centre = block.GetMapPosition() + glm::vec2(80.0f);
				terrainShader->Submit(static_cast<bgfx::ViewId>(desc.viewId),
				                      zsort::Depth(glm::vec3(centre.x, 0.0f, centre.y), cameraOrigin), discard);
			}
			_shaderManager->DiscardBindings();
			DrawCreatureFootprints(desc);
		}
	}

	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawModels
		                                                                          : Profiler::Stage::MainPassDrawModels);
		if (desc.drawEntities)
		{
			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = desc.viewId;
			submitDesc.program = objectShaderInstanced;
			submitDesc.state = 0u                              //
			                   | BGFX_STATE_WRITE_MASK         //
			                   | BGFX_STATE_DEPTH_TEST_GREATER //
			                   | BGFX_STATE_MSAA               //
			    ;
			const auto& renderCtx = Locator::rendereringSystem::value().GetContext();

			// Instance meshes
			// A creature is drawn with its own pose, and its body blended from its meshes
			struct EntityPose
			{
				std::span<const glm::mat4> bones;
				const L3DMeshSubmitDesc::MorphTargets* morphTargets;
			};
			const auto drawInstances = [&](entt::id_type meshId, const RenderContext::InstancedDrawDesc& placers,
			                               bool useMaterialBlending, uint32_t first, uint32_t count,
			                               const EntityPose* pose = nullptr) {
				auto mesh = meshManager.Handle(meshId);

				submitDesc.useMaterialBlending = useMaterialBlending;
				// The game gives the rooms the temple's light; the hand keeps its own
				const bool templeLit = Locator::temple::has_value() && Locator::temple::value().Active() &&
				                       meshId != ecs::components::Hand::k_MeshId;
				const auto light = templeLit ? Locator::temple::value().GetLight() : TempleLight {};
				submitDesc.tint = glm::vec4(light.multiply, 0.0f);
				submitDesc.lightMultiply = light.multiply;
				submitDesc.lightAdd = light.add;
				submitDesc.landLightScale = meshId == ecs::components::Hand::k_MeshId ? 1.5f : 1.0f;
				submitDesc.snow = !inTemple && meshId != ecs::components::Hand::k_MeshId;
				submitDesc.creatureShadows = meshId != ecs::components::Hand::k_MeshId;
				submitDesc.unlit = placers.unlit;
				submitDesc.instanceDesc =
				    std::make_unique<graphics::InstanceDesc>(renderCtx.instanceUniformBuffer, first, count);
				if (mesh->IsBoned())
				{
					const auto animated = renderCtx.animatedBoneMatrices.find(meshId);
					const auto& bones = animated != renderCtx.animatedBoneMatrices.end() &&
					                            animated->second.size() == mesh->GetBoneMatrices().size()
					                        ? animated->second
					                        : mesh->GetBoneMatrices();
					submitDesc.modelMatrices = bones.data();
					submitDesc.matrixCount = static_cast<uint8_t>(bones.size());
					// TODO(bwrsandman): Get animation frame instead of default for the other boned meshes
				}
				else
				{
					const static auto identity = glm::mat4(1.0f);
					submitDesc.modelMatrices = &identity;
					submitDesc.matrixCount = 1;
				}
				submitDesc.isSky = false;
				submitDesc.onlyJoints = placers.onlyJoints;
				submitDesc.hideShutJoints = placers.hideShutJoints;
				// The temple's meshes, which are drawn as their materials say
				submitDesc.useMaterialCulling = placers.materialBlending;
				submitDesc.mirrored = desc.cullBack;
				submitDesc.uvOffset = placers.uvOffset;
				submitDesc.subMeshTextures = placers.subMeshTextures;
				submitDesc.subMeshGlows = placers.subMeshGlows;
				submitDesc.hiddenSubMeshes = placers.hiddenSubMeshes;
				// The rooms meet at their doorways, whose arches each room has a copy of: the player's room's is seen
				submitDesc.depthBias = placers.behindCurrentRoom ? k_OtherTempleRoomDepthBias : 0.0f;
				submitDesc.joints = {};
				if (Locator::temple::has_value() && Locator::temple::value().Active())
				{
					submitDesc.joints = Locator::temple::value().GetDoors().GetJoints();
				}
				submitDesc.morphWithTerrain = placers.morphWithTerrain;
				submitDesc.program = submitDesc.morphWithTerrain ? objectShaderHeightMapInstanced
				                     : mesh->IsBoned()           ? objectShaderInstanced
				                                                 : objectShaderStaticInstanced;
				// Only the temple's meshes have lightmaps, and they don't stand on the land
				submitDesc.lightmapProgram = submitDesc.morphWithTerrain ? nullptr
				                             : placers.showsReflection   ? objectShaderReflectiveLightmapInstanced
				                                                         : objectShaderLightmapInstanced;
				if (placers.showsReflection)
				{
					objectShaderReflectiveLightmapInstanced->SetTextureSampler(
					    "s_reflection", 4, Locator::oceanSystem::value().GetReflectionFramebuffer().GetColorAttachment());
				}

				submitDesc.morphTargets = nullptr;
				if (pose != nullptr)
				{
					submitDesc.landLightScale = CreatureLandLightScale(desc.viewId);
					submitDesc.modelMatrices = pose->bones.data();
					submitDesc.matrixCount = static_cast<uint8_t>(pose->bones.size());
					if (pose->morphTargets != nullptr)
					{
						submitDesc.program = objectShaderMorphInstanced;
						submitDesc.morphTargets = pose->morphTargets;
					}
				}

				// TODO(bwrsandman): choose the correct LOD
				DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
				submitDesc.morphTargets = nullptr;
			};
			for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
			{
				if (meshId != ecs::components::Hand::k_MeshId && !placers.translucent && !placers.perEntity &&
				    !(desc.viewId == RenderPass::Reflection && placers.hiddenFromReflection))
				{
					drawInstances(meshId, placers, placers.materialBlending, placers.offset, placers.count);
				}
			}
			// The creatures, each posed and shaped as it is, then its eyes
			for (const auto& [entity, instance] : renderCtx.entityDraws)
			{
				const auto* mesh = desc.entities.TryGet<const ecs::components::Mesh>(entity);
				const auto* creature = desc.entities.TryGet<const ecs::components::Creature>(entity);
				const auto* morph = desc.entities.TryGet<const ecs::components::CreatureMorph>(entity);
				const auto* animation = desc.entities.TryGet<const ecs::components::CreatureAnimation>(entity);
				if (mesh == nullptr || !meshManager.Contains(mesh->id))
				{
					continue;
				}
				const auto placers = renderCtx.instancedDrawDescs.find(mesh->id);
				if (placers == renderCtx.instancedDrawDescs.end() ||
				    (desc.viewId == RenderPass::Reflection && placers->second.hiddenFromReflection))
				{
					continue;
				}
				const auto bodyMesh = meshManager.Handle(mesh->id);
				std::optional<L3DMeshSubmitDesc::MorphTargets> targets;
				if (creature != nullptr && morph != nullptr)
				{
					const auto ids = creature_morph::MeshesOf(
					    creature->species, morph->drawn, [&meshManager](entt::id_type id) { return meshManager.Contains(id); });
					targets = L3DMeshSubmitDesc::MorphTargets {
					    .meshes = {&*meshManager.Handle(ids.evilGood), &*meshManager.Handle(ids.thinFat),
					               &*meshManager.Handle(ids.weakStrong)},
					    .weights = glm::abs(glm::vec3(morph->drawn.evilGood, morph->drawn.thinFat, morph->drawn.weakStrong)),
					    .skins = {},
					    .blendSeams = Locator::config::value().blendCreatureSeams,
					};
					if (const auto skins = _creatureSkins.find(entity); skins != _creatureSkins.end())
					{
						targets->skins = skins->second.drawn;
					}
				}
				const bool posed = animation != nullptr && animation->boneMatrices.size() == bodyMesh->GetBoneMatrices().size();
				const EntityPose pose {
				    .bones = posed ? std::span<const glm::mat4>(animation->boneMatrices)
				                   : std::span<const glm::mat4>(bodyMesh->GetBoneMatrices()),
				    .morphTargets = targets ? &*targets : nullptr,
				};
				if (pose.bones.empty())
				{
					continue;
				}
				drawInstances(mesh->id, placers->second, placers->second.materialBlending, instance, 1, &pose);
				DrawCreatureEyes(desc, entity, submitDesc);
				DrawCreatureHair(desc, entity);
			}
			DrawTempleUnderside(desc);
			// In the temple, whose draws keep their order, the sun's glare comes after its solid parts, which hide it, and
			// before its glass, which blends over it
			if (inTemple && desc.viewId == RenderPass::Main && desc.drawSky)
			{
				DrawSunGlare(*desc.camera);
			}
			// The translucent meshes blend over the opaque ones, each in its own place in the sort
			submitDesc.viewId = translucentViewId;
			for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
			{
				if (placers.translucent && !(desc.viewId == RenderPass::Reflection && placers.hiddenFromReflection))
				{
					for (uint32_t instance = placers.offset; instance < placers.offset + placers.count; ++instance)
					{
						const auto position = glm::vec3(renderCtx.instanceUniforms.at(instance).model[3]);
						submitDesc.sortDepth = zsort::Depth(position, cameraOrigin);
						drawInstances(meshId, placers, true, instance, 1);
					}
				}
			}
			submitDesc.viewId = desc.viewId;
			submitDesc.sortDepth = 0;
			// The main room's pool, the map over it and its markers, ahead of the hand, whose faded wrist they show
			// through
			DrawTemplePool(desc);
			DrawTempleMap(desc);
			DrawTempleMapMarkers(desc);
			DrawCaveTrophies(desc);
			DrawGroundBlobs(desc);
			DrawLeashes(desc);
			DrawWaterRings(desc);
			DrawRain(desc);
			DrawSnowfall(desc);
			DrawChimneySmoke(desc);
			DrawInfluenceBorder(desc);
			DrawInfluenceRipples(desc);
			// The mists blend over the rest, the farthest first
			DrawMists(desc);

			// The game draws the hand after the rest of the scene, blended by its translucent texture. Black & White
			// culls its back faces; here both sides are drawn, the inside first so that the outside blends over it.
			// The game's reflection of the main room has no hand in it
			if (const auto hand = renderCtx.instancedDrawDescs.find(ecs::components::Hand::k_MeshId);
			    desc.drawHand && hand != renderCtx.instancedDrawDescs.end() &&
			    !(desc.viewId == RenderPass::Reflection && hand->second.hiddenFromReflection))
			{
				// L3D meshes face clockwise, which the mirrored reflection pass and a mirrored hand each turn around
				const bool facesTurned = desc.cullBack != renderCtx.handMirrored;
				// It takes its place in the sort by where it is
				submitDesc.viewId = translucentViewId;
				if (hand->second.count > 0)
				{
					const auto position = glm::vec3(renderCtx.instanceUniforms.at(hand->second.offset).model[3]);
					submitDesc.sortDepth = zsort::Depth(position, cameraOrigin);
				}
				const auto cullFront = facesTurned ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW;
				const auto cullBack = facesTurned ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW;
				const auto opaqueState = submitDesc.state;

				// The inside does not write depth so the outside is never hidden behind it
				submitDesc.state = (opaqueState & ~(BGFX_STATE_CULL_MASK | BGFX_STATE_WRITE_Z)) | cullFront;
				drawInstances(hand->first, hand->second, true, hand->second.offset, hand->second.count);
				submitDesc.state = (opaqueState & ~BGFX_STATE_CULL_MASK) | cullBack;
				drawInstances(hand->first, hand->second, true, hand->second.offset, hand->second.count);
				submitDesc.state = opaqueState;
				submitDesc.viewId = desc.viewId;
				submitDesc.sortDepth = 0;
			}

			// Debug
			if (desc.viewId == graphics::RenderPass::Main)
			{
				for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
				{
					auto mesh = meshManager.Handle(meshId);
					if (!mesh->ContainsLandscapeFeature() || mesh->GetFootprints().empty())
					{
						continue;
					}
				}
				if (renderCtx.boundingBox)
				{
					const auto boundBoxOffset = static_cast<uint32_t>(renderCtx.instanceUniforms.size() / 2);
					const auto boundBoxCount = static_cast<uint32_t>(renderCtx.instanceUniforms.size() / 2);
					renderCtx.boundingBox->GetVertexBuffer().Bind();
					bgfx::setInstanceDataBuffer(toBgfx(renderCtx.instanceUniformBuffer), boundBoxOffset, boundBoxCount);
					bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
					debugShaderInstanced->Submit(static_cast<bgfx::ViewId>(desc.viewId));
				}
				if (renderCtx.footpaths)
				{
					renderCtx.footpaths->GetVertexBuffer().Bind();
					bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
					debugShader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
				}
				if (renderCtx.streams)
				{
					renderCtx.streams->GetVertexBuffer().Bind();
					bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
					debugShader->Submit(static_cast<bgfx::ViewId>(desc.viewId));
				}
			}
		}

		{
			auto subSection =
			    profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawVegetation
			                                                               : Profiler::Stage::MainPassDrawVegetation);
			const auto& renderCtx = Locator::rendereringSystem::value().GetContext();

			if (desc.drawVegetation)
			{
				L3DMeshSubmitDesc submitDesc = {};
				submitDesc.viewId = desc.viewId;
				submitDesc.program = vegetationShaderInstanced;
				submitDesc.state = 0u                              //
				                   | BGFX_STATE_WRITE_MASK         //
				                   | BGFX_STATE_DEPTH_TEST_GREATER //
				                   | BGFX_STATE_MSAA               //
				    ;

				// Instance meshes
				for (const auto& [meshId, placers] : renderCtx.treeInstancedDrawDescs)
				{
					auto mesh = meshManager.Handle(meshId);
					submitDesc.instanceDesc = std::make_unique<graphics::InstanceDesc>(renderCtx.treeInstanceUniformBuffer,
					                                                                   placers.offset, placers.count);
					const static auto identity = glm::mat4(1.0f);
					submitDesc.modelMatrices = &identity;
					submitDesc.matrixCount = 1;
					submitDesc.isSky = false;
					submitDesc.morphWithTerrain = false;
					submitDesc.landLightScale = _treeBrightness;
					submitDesc.snow = !inTemple;
					submitDesc.program = vegetationShaderInstanced;
					DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
				}

				// Draw tree bounding boxes if enabled
				if (renderCtx.boundingBox && renderCtx.treeInstanceCount > 0)
				{
					const auto boundBoxOffset = renderCtx.treeInstanceCount;
					const auto boundBoxCount = renderCtx.treeInstanceCount;
					renderCtx.boundingBox->GetVertexBuffer().Bind();
					bgfx::setInstanceDataBuffer(toBgfx(renderCtx.treeInstanceUniformBuffer), boundBoxOffset, boundBoxCount);
					bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
					debugShaderInstanced->Submit(static_cast<bgfx::ViewId>(desc.viewId));
				}
			}
		}

		if (desc.drawEntities)
		{
			DrawLightBeams(desc);
			DrawMistDomes(desc);
			DrawTempleText(desc);
		}

		{
			auto subSection =
			    profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSprites
			                                                               : Profiler::Stage::MainPassDrawSprites);

			if (desc.drawSprites)
			{
				using namespace ecs::components;

				auto& registry = Locator::entitiesRegistry::value();
				registry.Each<const Sprite, const Transform>([this, &spriteShader, &desc, translucentViewId, cameraOrigin,
				                                              &registry](entt::entity entity, const Sprite& sprite,
				                                                         const Transform& transform) {
					// The temple draws the glows of the rooms it draws whole, and the main room reflects its own glows
					// alone in its floor
					if (const auto* templePart = registry.TryGet<const TempleInteriorPart>(entity); templePart != nullptr)
					{
						const bool inMainRoom = templePart->room == TempleRoom::Main;
						const bool drawn = Locator::temple::value().IsRoomDrawn(templePart->room);
						if (desc.viewId == RenderPass::Reflection ? !inMainRoom : !drawn)
						{
							return;
						}
					}
					glm::mat4 modelMatrix = glm::mat4(1.0f);
					modelMatrix = glm::translate(modelMatrix, transform.position);
					modelMatrix *= glm::mat4(transform.rotation);
					modelMatrix = glm::scale(modelMatrix, transform.scale);

					glm::vec4 u_sampleRect(sprite.uvExtent, sprite.uvMin);

					bgfx::setTransform(glm::value_ptr(modelMatrix));
					spriteShader->SetUniformValue("u_sampleRect", glm::value_ptr(u_sampleRect));
					const glm::vec4 u_spriteParams {sprite.facesCamera ? 1.0f : 0.0f, sprite.alpha.has_value() ? 1.0f : 0.0f,
					                                sprite.additive ? 1.0f : 0.0f, 0.0f};
					spriteShader->SetUniformValue("u_spriteParams", glm::value_ptr(u_spriteParams));
					spriteShader->SetTextureSampler("s_alpha", 1, sprite.alpha.value_or(sprite.texture));
					// The shader multiplies the tint by the texture's alpha, which for alpha blending gives colours
					// premultiplied by alpha when the tint is
					const auto tint =
					    sprite.additive ? sprite.tint : glm::vec4(glm::vec3(sprite.tint) * sprite.tint.a, sprite.tint.a);
					spriteShader->SetUniformValue("u_tint", glm::value_ptr(tint));
					spriteShader->SetTextureSampler("s_diffuse", 0, sprite.texture);

					_plane->GetVertexBuffer().Bind();

					const auto blend = sprite.additive
					                       ? BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_SRC_ALPHA, BGFX_STATE_BLEND_ONE)
					                       : BGFX_STATE_BLEND_FUNC(BGFX_STATE_BLEND_ONE, BGFX_STATE_BLEND_INV_SRC_ALPHA);
					bgfx::setState(0 | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | blend |
					               BGFX_STATE_BLEND_EQUATION(BGFX_STATE_BLEND_EQUATION_ADD));

					spriteShader->Submit(static_cast<bgfx::ViewId>(translucentViewId),
					                     zsort::Depth(transform.position, cameraOrigin));
				});
			}
		}
	}

	// The sun's glare over everything else in the view; the temple's comes before its glass
	if (desc.viewId == RenderPass::Main && desc.drawSky && !(Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		DrawSunGlare(*desc.camera);
	}

	// The hand's glow lies on the water, under the sea that is blended over it
	if (desc.viewId == RenderPass::Reflection && desc.drawHand)
	{
		DrawHandWaterGlow(desc);
	}

	// Enable stats or debug text.
	auto debugMode = BGFX_DEBUG_NONE;
	if (_bgfxDebug)
	{
		debugMode |= BGFX_DEBUG_STATS;
	}
	if (desc.wireframe)
	{
		debugMode |= BGFX_DEBUG_WIREFRAME;
	}
	if (_bgfxProfile)
	{
		debugMode |= BGFX_DEBUG_PROFILER;
	}
	bgfx::setDebug(debugMode);
}

void Renderer::Frame() noexcept
{
	// Advance to next frame. Process submitted rendering primitives.
	bgfx::frame();
	_shaderManager->FrameEnded();
}

void Renderer::RequestScreenshot(const std::filesystem::path& filepath) noexcept
{
	const bgfx::FrameBufferHandle mainBackbuffer = BGFX_INVALID_HANDLE;
	bgfx::requestScreenShot(mainBackbuffer, filepath.string().c_str());
}
