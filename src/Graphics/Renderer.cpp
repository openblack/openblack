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
#include <limits>
#include <map>
#include <memory>
#define LOCATOR_IMPLEMENTATIONS

#include <cstdint>

#include <SDL_video.h>
#include <bgfx/platform.h>
#include <bimg/bimg.h>
#include <bx/file.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DAnim.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandBlock.h"
#include "3D/LandIslandInterface.h"
#include "3D/LandLightTable.h"
#include "3D/OceanInterface.h"
#include "3D/OrientedText.h"
#include "3D/SkyInterface.h"
#include "3D/TempleDoors.h"
#include "3D/TempleInteriorInterface.h"
#include "3D/TempleMap.h"
#include "Camera/Camera.h"
#include "ECS/Components/Hand.h"
#include "ECS/Components/LightBeam.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MistDome.h"
#include "ECS/Components/Sprite.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/RenderingSystemInterface.h"
#include "EngineConfig.h"
#include "FileSystem/FileSystemInterface.h"
#include "Game.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/GraphicsHandleBgfx.h"
#include "Graphics/HandLight.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/LightBeams.h"
#include "Graphics/ModelLight.h"
#include "Graphics/ObjectShadows.h"
#include "Graphics/Primitive.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/VertexBuffer.h"
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

		// Must terminate, continuing will cause crash anyway.
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
	{
		constexpr uint32_t k_White = 0xFFFFFFFF;
		_whiteTexture = fromBgfx(bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, BGFX_SAMPLER_NONE,
		                                               bgfx::copy(&k_White, sizeof(k_White))));
		bgfx::setName(toBgfx(*_whiteTexture), "White");
	}
	// LH3D rasterises shadows with eight coverage samples per texel, multisampling gives the same soft edges
	_handShadowFrameBuffer =
	    std::make_unique<FrameBuffer>("Hand Shadow", HandShadow::k_TextureSize, HandShadow::k_TextureSize, TextureFormat::R8,
	                                  std::nullopt, static_cast<uint8_t>(8), Wrapping::ClampEdge);

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
	_plane.reset();
	_handShadowFrameBuffer.reset();
	_objectShadowFrameBuffer.reset();
	_templeMapFrameBuffer.reset();
	if (_handLightTexture)
	{
		bgfx::destroy(toBgfx(*_handLightTexture));
	}
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
	_shaderManager.reset();
	bgfx::frame();
	bgfx::shutdown();
}

void Renderer::ConfigureView(graphics::RenderPass viewId, glm::u16vec2 resolution, uint32_t clearColor) const noexcept
{
	bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 0.0f, 0);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, resolution.x, resolution.y);
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

void Renderer::DrawSubMesh(const graphics::L3DMesh& mesh, const graphics::L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc,
                           bool preserveState, const TextureHandle* subMeshTexture, glm::vec3 glow) const
{
	assert(&subMesh.GetMesh());
	// We don't draw physics meshes, we haven't implemented statuses (building and graves) and modern GPUs can handle high lod
	if (!desc.drawAll && (subMesh.IsPhysics() || subMesh.GetFlags().status != 0 || (subMesh.GetFlags().lodMask & 1) != 1))
	{
		return;
	}

	const auto& island = Locator::terrainSystem::value();

	auto extent = island.GetExtent();
	auto islandExtent = glm::vec4(extent.minimum, extent.maximum);
	const auto& heightMap = island.GetHeightMap();

	auto const& skins = mesh.GetSkins();
	// LH3DMesh::DrawLightMap draws the submeshes with a lightmap through it and the others as they are
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

	// LH3DMesh turns a submesh with a joint about its pivot by its matrix of the table, before the mesh's own matrix
	const auto* modelMatrices = desc.modelMatrices;
	glm::mat4 jointModel;
	if (const auto& joint = subMesh.GetJoint();
	    joint.has_value() && joint->index < desc.joints.size() && modelMatrices != nullptr && desc.matrixCount == 1)
	{
		jointModel = *modelMatrices * glm::translate(glm::mat4(1.0f), joint->pivot) * desc.joints[joint->index] *
		             glm::translate(glm::mat4(1.0f), -joint->pivot);
		modelMatrices = &jointModel;
	}
	bool lastPreserveState = false;
	const auto& primitives = subMesh.GetPrimitives();
	for (auto it = primitives.begin(); it != primitives.end(); ++it)
	{
		const auto& prim = *it;

		const bool hasNext = std::next(it) != primitives.end();

		const Texture2D* texture = GetTexture(prim.skinID, skins);
		const Texture2D* nextTexture = !hasNext ? nullptr : GetTexture(std::next(it)->skinID, skins);

		// Primitives drawn with their own material's blending can't share render state, nor can a submesh with a texture
		// of its own
		const bool primitivePreserveState = !desc.useMaterialBlending && subMeshTexture == nullptr &&
		                                    desc.subMeshGlows.empty() && texture != nullptr && texture == nextTexture &&
		                                    (preserveState || hasNext);

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
			if (program->HasUniform("u_uvOffset"))
			{
				const glm::vec4 u_uvOffset {desc.uvOffset, 0.0f, 0.0f};
				program->SetUniformValue("u_uvOffset", &u_uvOffset);
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
				const glm::vec4 u_landLight {landLit ? 1.0f : 0.0f, desc.landLightScale, 0.0f, 0.0f};
				program->SetTextureSampler("s_landLuminosity", 3, island.GetLuminosityMap());
				program->SetTextureSampler("s_landLight", 4, _landLightTexture.value_or(GetHandLightTexture()));
				program->SetUniformValue("u_islandExtent", &islandExtent);
				program->SetUniformValue("u_landLight", &u_landLight);
			}
			if (program->HasUniform("s_handLight"))
			{
				program->SetTextureSampler("s_handLight", 2, GetHandLightTexture());
				program->SetUniformValue("u_handLight", &_handLight);
			}
			if (program->HasUniform("u_modelLight"))
			{
				program->SetUniformValue("u_modelLight", &_modelLight);
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
				if (desc.useMaterialBlending)
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

			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(program->GetRawHandle()), 0,
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

	// GatheringText::DrawTextRawOriented draws the glyphs blended by their coverage, tested against the room's depth
	const auto* shader = _shaderManager->GetShader("Text3D");
	const auto model = glm::translate(glm::mat4(1.0f), temple.GetPosition());
	bgfx::setTransform(glm::value_ptr(model));
	bgfx::setVertexBuffer(0, &buffer);
	shader->SetTextureSampler("s_texture", 0, *texture);
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_DEPTH_TEST_GREATER | BGFX_STATE_MSAA |
	               BGFX_STATE_BLEND_ALPHA);
	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(shader->GetRawHandle()));
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
		// TODO(raffclar): confirm how LH3DObject::Draw lights an object given a colour
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

void Renderer::DrawTempleMapPass(const DrawSceneDesc& desc) const
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

	// 8 texels a block, as LH3D's pages of the land's textures give the map, over the 32 by 32 blocks there can be
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
	const glm::vec4 u_skyAndBump = {0.0f, desc.bumpMapStrength, 0.0f, 1.0f};
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
	terrainShader->SetTextureSampler("s0_materials", 0, island.GetAlbedoArray());
	terrainShader->SetTextureSampler("s1_bump", 1, island.GetBump());
	terrainShader->SetTextureSampler("s2_smallBump", 2, *smallBump);
	terrainShader->SetTextureSampler("s3_footprints", 3, island.GetFootprintFramebuffer().GetColorAttachment());
	terrainShader->SetTextureSampler("s4_handShadow", 4, _handShadowFrameBuffer->GetColorAttachment());
	terrainShader->SetTextureSampler("s5_objectShadows", 5,
	                                 _objectShadowFrameBuffer ? _objectShadowFrameBuffer->GetColorAttachment()
	                                                          : island.GetFootprintFramebuffer().GetColorAttachment());
	terrainShader->SetTextureSampler("s6_handLight", 6, GetHandLightTexture());
	terrainShader->SetTextureSampler("s7_landLight", 7, _landLightTexture.value_or(GetHandLightTexture()));
	terrainShader->SetUniformValue("u_skyAndBump", &u_skyAndBump);
	terrainShader->SetUniformValue("u_objectShadows", &u_objectShadows);
	terrainShader->SetUniformValue("u_islandExtent", &islandExtent);
	terrainShader->SetUniformValue("u_handShadowMatrix", &noHandShadow);
	terrainShader->SetUniformValue("u_handShadow", &noHand);
	terrainShader->SetUniformValue("u_handLight", &noHand);
	for (const auto& block : island.GetBlocks())
	{
		const glm::vec4 mapPositionAndSize = glm::vec4(block.GetMapPosition(), 160.0f, 160.0f);
		terrainShader->SetUniformValue("u_blockPositionAndSize", &mapPositionAndSize);
		block.GetMesh().GetVertexBuffer().Bind();
		bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A);
		// The textures stay bound from one block to the next
		bgfx::submit(viewId, toBgfx(terrainShader->GetRawHandle()), 0,
		             BGFX_DISCARD_INSTANCE_DATA | BGFX_DISCARD_INDEX_BUFFER | BGFX_DISCARD_TRANSFORM |
		                 BGFX_DISCARD_VERTEX_STREAMS | BGFX_DISCARD_STATE);
	}
	bgfx::discard(BGFX_DISCARD_BINDINGS);
}

void Renderer::DrawTempleUnderside(const DrawSceneDesc& desc) const
{
	// The rooms' meshes don't quite meet everywhere: the sills of the main room's doorways stand a hundredth of a unit off
	// their doors' frames, and their edges have vertices of others part way along them. Vanilla draws the sky behind the
	// temple (DoCitadelDraw), so it shows through the cracks there too, as specks of the outside. Looking down through
	// them, this shows black instead. The creature's room goes deepest, to 70.5 below the temple.
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
	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(shader->GetRawHandle()));
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

	// Render mode 5: the land's texture by the vertices' colours, blended by their alpha
	const auto* shader = _shaderManager->GetShader("Text3D");
	const auto model = glm::translate(glm::mat4(1.0f), temple.GetPosition());
	bgfx::setTransform(glm::value_ptr(model));
	bgfx::setVertexBuffer(0, &buffer);
	shader->SetTextureSampler("s_texture", 0, _templeMapFrameBuffer->GetColorAttachment());
	bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_GREATER |
	               BGFX_STATE_MSAA | BGFX_STATE_BLEND_ALPHA);
	bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(shader->GetRawHandle()));
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
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(spriteShader->GetRawHandle()));
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
		// MiniMap::DrawMarker: ApplyCitadelColoring puts them in the temple's light
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
	// fn_00787340 gives every material of the icons the one texture, with the alpha LH3D reads from beside it
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
		// CreatureRoom::Draw draws the belts and the medals past wood in LH3D's render mode 2, which adds the first of
		// LH3DObject's environment maps, envmap.raw (0xC37EAC)
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
		// The colour multiplies what lights them: the medals come out the mid grey of vanilla's at 0x80. ApplyCitadelColoring
		// puts it in the temple's light.
		const auto& light = Locator::temple::value().GetLight();
		submitDesc.tint = glm::vec4(light.Colour(colour / 255.0f), 0.0f);
		submitDesc.lightAdd = light.add;
		DrawMesh(*meshes.Handle(trophy.mesh), submitDesc, std::numeric_limits<uint8_t>::max());
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
	// Render mode 6 of the smoke's material: blended by alpha, without writing depth, from both sides
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
		    bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(shader->GetRawHandle()));
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

	// Render mode 0xd: added by alpha, without writing depth, from both sides
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
		bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(beamShader->GetRawHandle()));
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
			bgfx::submit(static_cast<bgfx::ViewId>(viewId), toBgfx(footprintShaderInstanced->GetRawHandle()));
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
			bgfx::submit(static_cast<bgfx::ViewId>(viewId), toBgfx(footprintShaderInstanced->GetRawHandle()));
		}
	}
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

glm::vec4 Renderer::GetHandLight(const DrawSceneDesc& drawDesc) const
{
	if (!_handLightLoaded)
	{
		_handLightLoaded = true;
		auto& fileSystem = Locator::filesystem::value();
		const auto path = fileSystem.GetPath<filesystem::Path::Textures>() / "light_hand.raw";
		if (fileSystem.Exists(path))
		{
			const auto map = fileSystem.ReadAll(path);
			if (map.size() >= static_cast<size_t>(HandLight::k_Size) * HandLight::k_Size)
			{
				const auto* memory = bgfx::copy(map.data(), HandLight::k_Size * HandLight::k_Size);
				_handLightTexture =
				    fromBgfx(bgfx::createTexture2D(HandLight::k_Size, HandLight::k_Size, false, 1, bgfx::TextureFormat::R8,
				                                   BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP, memory));
				bgfx::setName(toBgfx(*_handLightTexture), "Hand Light");
			}
		}
	}

	auto handLight = glm::vec4(0.0f);
	if (!_handLightTexture || !drawDesc.drawHand || !Locator::handSystem::has_value() || !Locator::skySystem::has_value())
	{
		return handLight;
	}
	const auto handEntity =
	    Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(ecs::systems::HandSystemInterface::Side::Left)];
	const auto* transform = Locator::entitiesRegistry::value().TryGet<ecs::components::Transform>(handEntity);
	if (transform == nullptr)
	{
		return handLight;
	}
	const auto origin = HandLight::GetOrigin(transform->position);
	handLight = glm::vec4(origin, HandLight::GetStrength(Locator::skySystem::value().GetCurrentSkyType()), 0.0f);
	return handLight;
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
		const auto skyType = Locator::skySystem::has_value() ? Locator::skySystem::value().GetCurrentSkyType() : 2.0f;
		const auto alignment =
		    Locator::alignmentSystem::has_value() ? Locator::alignmentSystem::value().GetSkyAlignment() : 0.0f;
		_landLightTable->Build(*palettes.Handle(LandLightPalette::k_Id.value()), skyType, alignment);
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
			light = model_light::FrameLight(transform->position, ground, Locator::camera::value().GetOrigin(),
			                                Locator::skySystem::value().GetCurrentSkyType());
		}
	}
	return model_light::Uniform(light);
}

TextureHandle Renderer::GetHandLightTexture() const
{
	return _handLightTexture ? *_handLightTexture : _handShadowFrameBuffer->GetColorAttachment().GetNativeHandle();
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

void Renderer::DrawScene(const DrawSceneDesc& drawDesc) const noexcept
{
	// TODO(bwrsandman): Footprint framebuffer doesn't need to be updated each frame
	DrawFootprintPass(drawDesc);
	UpdateLandLight();
	DrawObjectShadowPass(drawDesc);
	DrawTempleMapPass(drawDesc);
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

			DrawPass(drawPassDesc);
		}
	}

	// Main Draw Pass
	{
		auto section = Locator::profiler::value().BeginScoped(Profiler::Stage::MainPass);
		DrawPass(drawDesc);
	}
	// Meshes drawn outside of the scene, as in the mesh viewer, aren't lit by the hand
	_handLight = glm::vec4(0.0f);
}

void Renderer::DrawPass(const DrawSceneDesc& desc) const
{
	const auto& meshManager = Locator::resources::value().GetMeshes();
	auto& profiler = Locator::profiler::value();

	if (desc.frameBuffer != nullptr)
	{
		desc.frameBuffer->Bind(desc.viewId);
	}
	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));
	// bgfx sorts a view's blended draws by their programs unless told to keep them in order. LH3D draws the temple
	// in the order it is submitted, and its blended parts must be too: the pool's water, drawn with the lightmap
	// program, would otherwise land after the hand, whose faded wrist writes depth, and leave a hole in the water
	// where the wrist should show it through.
	const bool inTemple = Locator::temple::has_value() && Locator::temple::value().Active();
	bgfx::setViewMode(static_cast<bgfx::ViewId>(desc.viewId), inTemple ? bgfx::ViewMode::Sequential : bgfx::ViewMode::Default);

	_shaderManager->SetCamera(desc.viewId, *desc.camera);
	// The hand lights whatever is around it at night, the land, the sea and the things on them
	_handLight = GetHandLight(desc);
	_modelLight = GetModelLight();

	const auto* skyShader = _shaderManager->GetShader("Sky");
	const auto* waterShader = _shaderManager->GetShader("Water");
	const auto* terrainShader = _shaderManager->GetShader("Terrain");
	const auto* debugShader = _shaderManager->GetShader("DebugLine");
	// Trees are placed on the land already, so they are drawn where they are rather than moved onto the height map
	const auto* vegetationShaderInstanced = _shaderManager->GetShader("Vegetation");
	const auto* spriteShader = _shaderManager->GetShader("Sprite");
	const auto* debugShaderInstanced = _shaderManager->GetShader("DebugLineInstanced");
	const auto* objectShaderInstanced = _shaderManager->GetShader("ObjectInstanced");
	const auto* objectShaderStaticInstanced = _shaderManager->GetShader("ObjectStaticInstanced");
	const auto* objectShaderHeightMapInstanced = _shaderManager->GetShader("ObjectHeightMapInstanced");
	const auto* objectShaderLightmapInstanced = _shaderManager->GetShader("ObjectLightmapInstanced");
	const auto* objectShaderReflectiveLightmapInstanced = _shaderManager->GetShader("ObjectReflectiveLightmapInstanced");

	const auto skyType = Locator::skySystem::value().GetCurrentSkyType();

	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSky
		                                                                          : Profiler::Stage::MainPassDrawSky);
		if (desc.drawSky)
		{
			const auto modelMatrix = glm::mat4(1.0f);
			// The sky's alignment from 0, evil, to 2, good
			const glm::vec4 u_typeAlignment = {skyType, Locator::alignmentSystem::value().GetSkyAlignment() + 1.0f, 0.0f, 0.0f};

			skyShader->SetTextureSampler("s_diffuse", 0, Locator::skySystem::value().GetTexture());
			skyShader->SetUniformValue("u_typeAlignment", &u_typeAlignment);

			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = desc.viewId;
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
			const glm::vec4 u_sky = {skyType, 0.0f, 0.0f, 0.0f};
			waterShader->SetUniformValue("u_sky", &u_sky); // fs
			waterShader->SetTextureSampler("s_handLight", 3, GetHandLightTexture());
			waterShader->SetUniformValue("u_handLight", &_handLight);
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(waterShader->GetRawHandle()));
		}
	}

	{
		auto section = profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawIsland
		                                                                          : Profiler::Stage::MainPassDrawIsland);
		if (desc.drawIsland)
		{
			auto& island = Locator::terrainSystem::value();
			auto islandExtent = glm::vec4(island.GetExtent().minimum, island.GetExtent().maximum);

			auto texture = Locator::resources::value().GetTextures().Handle(LandIslandInterface::k_SmallBumpTextureId);
			const glm::vec4 u_skyAndBump = {skyType, desc.bumpMapStrength, desc.smallBumpMapStrength, 0.0f};
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

			terrainShader->SetTextureSampler("s0_materials", 0, island.GetAlbedoArray());
			terrainShader->SetTextureSampler("s1_bump", 1, island.GetBump());
			terrainShader->SetTextureSampler("s2_smallBump", 2, *texture);
			terrainShader->SetTextureSampler("s3_footprints", 3, island.GetFootprintFramebuffer().GetColorAttachment());
			terrainShader->SetTextureSampler("s5_objectShadows", 5,
			                                 _objectShadowFrameBuffer ? _objectShadowFrameBuffer->GetColorAttachment()
			                                                          : island.GetFootprintFramebuffer().GetColorAttachment());

			terrainShader->SetTextureSampler("s7_landLight", 7, _landLightTexture.value_or(GetHandLightTexture()));
			terrainShader->SetUniformValue("u_skyAndBump", &u_skyAndBump);
			terrainShader->SetUniformValue("u_objectShadows", &u_objectShadows);
			terrainShader->SetUniformValue("u_islandExtent", &islandExtent);

			// The hand's shadow falls on the land, not on its reflection
			const auto& handShadow = desc.viewId == RenderPass::Main ? _handShadow : std::nullopt;
			const auto handShadowMatrix = handShadow ? handShadow->receiverMatrix : glm::mat4(0.0f);
			const auto u_handShadow =
			    handShadow ? glm::vec4(handShadow->strength * HandShadow::k_MaxDarkness, handShadow->startDepth, 0.0f, 0.0f)
			               : glm::vec4(0.0f);
			terrainShader->SetTextureSampler("s4_handShadow", 4, _handShadowFrameBuffer->GetColorAttachment());
			terrainShader->SetUniformValue("u_handShadowMatrix", &handShadowMatrix);
			terrainShader->SetUniformValue("u_handShadow", &u_handShadow);

			// The hand's light is in the land's own lighting, so the reflection shows it too
			terrainShader->SetTextureSampler("s6_handLight", 6, GetHandLightTexture());
			terrainShader->SetUniformValue("u_handLight", &_handLight);

			// clang-format off
			constexpr auto defaultState = 0u
				| BGFX_STATE_WRITE_MASK
				| BGFX_STATE_DEPTH_TEST_GREATER
				| BGFX_STATE_BLEND_ALPHA
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

			for (const auto& block : island.GetBlocks())
			{
				// pack uniforms
				const glm::vec4 mapPositionAndSize = glm::vec4(block.GetMapPosition(), 160.0f, 160.0f);
				terrainShader->SetUniformValue("u_blockPositionAndSize", &mapPositionAndSize);

				block.GetMesh().GetVertexBuffer().Bind();

				bgfx::setState(defaultState | (desc.cullBack ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW), 0);
				bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(terrainShader->GetRawHandle()), 0, discard);
			}
			bgfx::discard(BGFX_DISCARD_BINDINGS);
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
			const auto drawInstances = [&](entt::id_type meshId, const RenderContext::InstancedDrawDesc& placers,
			                               bool useMaterialBlending) {
				auto mesh = meshManager.Handle(meshId);

				submitDesc.useMaterialBlending = useMaterialBlending;
				// TempleRoom::Draw gives the rooms the temple's light; the hand keeps its own
				const bool templeLit = Locator::temple::has_value() && Locator::temple::value().Active() &&
				                       meshId != ecs::components::Hand::k_MeshId;
				const auto light = templeLit ? Locator::temple::value().GetLight() : TempleLight {};
				submitDesc.tint = glm::vec4(light.multiply, 0.0f);
				submitDesc.lightMultiply = light.multiply;
				submitDesc.lightAdd = light.add;
				submitDesc.landLightScale = meshId == ecs::components::Hand::k_MeshId ? 1.5f : 1.0f;
				submitDesc.instanceDesc =
				    std::make_unique<graphics::InstanceDesc>(renderCtx.instanceUniformBuffer, placers.offset, placers.count);
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

				// TODO(bwrsandman): choose the correct LOD
				DrawMesh(*mesh, submitDesc, std::numeric_limits<uint8_t>::max());
			};
			for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
			{
				if (meshId != ecs::components::Hand::k_MeshId && !placers.translucent &&
				    !(desc.viewId == RenderPass::Reflection && placers.hiddenFromReflection))
				{
					drawInstances(meshId, placers, placers.materialBlending);
				}
			}
			DrawTempleUnderside(desc);
			// The translucent meshes blend over the opaque ones
			for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
			{
				if (placers.translucent && !(desc.viewId == RenderPass::Reflection && placers.hiddenFromReflection))
				{
					drawInstances(meshId, placers, true);
				}
			}
			// WorldRoom::Draw's pool, the map over it and its markers, ahead of the hand, whose faded wrist they show
			// through
			DrawTemplePool(desc);
			DrawTempleMap(desc);
			DrawTempleMapMarkers(desc);
			DrawCaveTrophies(desc);
			// CHand draws the hand after the rest of the scene, blended by its translucent texture. Black & White culls
			// its back faces; here both sides are drawn, the inside first so that the outside blends over it.
			// WorldRoom::Draw's reflection of the main room has no hand in it
			if (const auto hand = renderCtx.instancedDrawDescs.find(ecs::components::Hand::k_MeshId);
			    desc.drawHand && hand != renderCtx.instancedDrawDescs.end() &&
			    !(desc.viewId == RenderPass::Reflection && hand->second.hiddenFromReflection))
			{
				// L3D meshes face clockwise, which the mirrored reflection pass and a mirrored hand each turn around
				const bool facesTurned = desc.cullBack != renderCtx.handMirrored;
				const auto cullFront = facesTurned ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW;
				const auto cullBack = facesTurned ? BGFX_STATE_CULL_CW : BGFX_STATE_CULL_CCW;
				const auto opaqueState = submitDesc.state;

				// The inside does not write depth so the outside is never hidden behind it
				submitDesc.state = (opaqueState & ~(BGFX_STATE_CULL_MASK | BGFX_STATE_WRITE_Z)) | cullFront;
				drawInstances(hand->first, hand->second, true);
				submitDesc.state = (opaqueState & ~BGFX_STATE_CULL_MASK) | cullBack;
				drawInstances(hand->first, hand->second, true);
				submitDesc.state = opaqueState;
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
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(debugShaderInstanced->GetRawHandle()));
				}
				if (renderCtx.footpaths)
				{
					renderCtx.footpaths->GetVertexBuffer().Bind();
					bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(debugShader->GetRawHandle()));
				}
				if (renderCtx.streams)
				{
					renderCtx.streams->GetVertexBuffer().Bind();
					bgfx::setState(k_BgfxDefaultStateInvertedZ | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(debugShader->GetRawHandle()));
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
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(debugShaderInstanced->GetRawHandle()));
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
				registry.Each<const Sprite, const Transform>([this, &spriteShader, &desc,
				                                              &registry](entt::entity entity, const Sprite& sprite,
				                                                         const Transform& transform) {
					// Temple::Draw draws the glows of the rooms it draws whole, and WorldRoom::Draw reflects the main
					// room's alone in its floor
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

					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), toBgfx(spriteShader->GetRawHandle()));
				});
			}
		}
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
}

void Renderer::RequestScreenshot(const std::filesystem::path& filepath) noexcept
{
	const bgfx::FrameBufferHandle mainBackbuffer = BGFX_INVALID_HANDLE;
	bgfx::requestScreenShot(mainBackbuffer, filepath.string().c_str());
}
