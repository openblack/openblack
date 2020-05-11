/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Renderer.h"

#include "3D/Camera.h"
#include "3D/L3DMesh.h"
#include "3D/LandIsland.h"
#include "3D/MeshPack.h"
#include "3D/Sky.h"
#include "3D/Water.h"
#include "Entities/Registry.h"
#include "Game.h" // TODO: remove use of this global
#include "GameWindow.h"
#include "Graphics/DebugLines.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/ShaderManager.h"
#include "Graphics/VertexBuffer.h"
#include "Profiler.h"

#include <SDL_video.h>
#include <bgfx/platform.h>
#include <spdlog/spdlog.h>

using namespace openblack;
using namespace openblack::graphics;

namespace openblack
{
struct BgfxCallback: public bgfx::CallbackI
{
	~BgfxCallback() override = default;

	void fatal(const char* filePath, uint16_t line, bgfx::Fatal::Enum code, const char* str) override
	{
		const static std::array CodeLookup = {
		    "DebugCheck", "InvalidShader", "UnableToInitialize", "UnableToCreateTexture", "DeviceLost",
		};
		spdlog::critical("bgfx: {}:{}: FATAL ({}): {}", filePath, line, CodeLookup[code], str);

		// Must terminate, continuing will cause crash anyway.
		throw std::runtime_error(std::string("bgfx: ") + filePath + ":" + std::to_string(line) + ": FATAL (" +
		                         CodeLookup[code] + "): " + str);
	}

	void traceVargs(const char* filePath, uint16_t line, const char* format, va_list argList) override
	{
		char temp[0x2000];
		char* out = temp;
		int32_t len = vsnprintf(out, sizeof(temp), format, argList);
		if ((int32_t)sizeof(temp) < len)
		{
			out = (char*)alloca(len + 1);
			len = vsnprintf(out, len, format, argList);
		}
		out[len] = '\0';
		if (len > 0 && out[len - 1] == '\n')
		{
			out[len - 1] = '\0';
		}
		spdlog::debug("bgfx: {}:{}: {}", filePath, line, out);
	}
	void profilerBegin(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override {}
	void profilerBeginLiteral(const char* name, uint32_t abgr, const char* filePath, uint16_t line) override {}
	void profilerEnd() override {}
	// Reading and writing to shader cache
	uint32_t cacheReadSize(uint64_t id) override { return 0; }
	bool cacheRead(uint64_t id, void* data, uint32_t size) override { return false; }
	void cacheWrite(uint64_t id, const void* data, uint32_t size) override {}
	// Saving a screen shot
	void screenShot(const char* filePath, uint32_t width, uint32_t height, uint32_t pitch, const void* data, uint32_t size,
	                bool yflip) override
	{
	}
	// Saving a video
	void captureBegin(uint32_t width, uint32_t height, uint32_t pitch, bgfx::TextureFormat::Enum _format, bool yflip) override
	{
	}
	void captureEnd() override {}
	void captureFrame(const void* data, uint32_t size) override {}
};

} // namespace openblack

Renderer::Renderer(const GameWindow* window, bgfx::RendererType::Enum rendererType, bool vsync)
    : _shaderManager(std::make_unique<ShaderManager>())
    , _bgfxCallback(std::make_unique<BgfxCallback>())
{
	bgfx::Init init {};
	init.type = rendererType;

	// Get render area size
	int drawable_width;
	int drawable_height;
	if (rendererType != bgfx::RendererType::Noop)
	{
		window->GetSize(drawable_width, drawable_height);
		init.resolution.width = static_cast<uint32_t>(drawable_width);
		init.resolution.height = static_cast<uint32_t>(drawable_height);

		// Get Native Handles from SDL window
		window->GetNativeHandles(init.platformData.nwh, init.platformData.ndt);
	}

	init.resolution.reset = BGFX_RESET_NONE;
	if (vsync)
	{
		init.resolution.reset |= BGFX_RESET_VSYNC;
	}
	init.callback = dynamic_cast<bgfx::CallbackI*>(_bgfxCallback.get());

	if (!bgfx::init(init))
	{
		throw std::runtime_error("Failed to initialize bgfx.");
	}

	LoadShaders();

	// allocate vertex buffers for our debug draw
	_debugCross = DebugLines::CreateCross();

	// give debug names to views
	for (bgfx::ViewId i = 0; i < static_cast<bgfx::ViewId>(graphics::RenderPass::_count); ++i)
	{
		bgfx::setViewName(i, RenderPassNames[i].data());
	}
}

Renderer::~Renderer()
{
	_shaderManager.reset();
	_debugCross.reset();
	bgfx::shutdown();
}

void Renderer::LoadShaders()
{
	for (const auto& shader : Shaders)
	{
		_shaderManager->LoadShader(shader.name.data(), shader.vertexShaderName.data(), shader.fragmentShaderName.data());
	}
}

void Renderer::ConfigureView(graphics::RenderPass viewId, uint16_t width, uint16_t height) const
{
	static const uint32_t clearColor = 0x274659ff;

	bgfx::setViewClear(static_cast<bgfx::ViewId>(viewId), BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, clearColor, 1.0f, 0);
	bgfx::setViewRect(static_cast<bgfx::ViewId>(viewId), 0, 0, width, height);
}

graphics::ShaderManager& Renderer::GetShaderManager() const
{
	return *_shaderManager;
}

void Renderer::UpdateDebugCrossUniforms(const glm::vec3& position, float scale)
{
	_debugCross->SetPose(position, glm::vec3(scale, scale, scale));
}

void Renderer::DrawSubMesh(const L3DMesh& mesh, const L3DSubMesh& subMesh, const L3DMeshSubmitDesc& desc,
                           graphics::RenderPass viewId, const ShaderProgram& program, uint64_t state, uint32_t rgba,
                           bool preserveState) const
{
	assert(&subMesh.GetMesh());
	assert(!subMesh.isPhysics());

	auto const& skins = mesh.GetSkins();

	auto getTexture = [&skins](uint32_t skinID) -> const Texture2D* {
		if (skinID != 0xFFFFFFFF)
		{
			if (skins.find(skinID) != skins.end())
				return skins.at(skinID).get();
			else
				return &Game::instance()->GetMeshPack().GetTexture(skinID);
		}
		return nullptr;
	};

	bool lastPreserveState = false;
	const auto& primitives = subMesh.GetPrimitives();
	for (auto it = primitives.begin(); it != primitives.end(); ++it)
	{
		const auto& prim = *it;

		const bool hasNext = std::next(it) != primitives.end();

		const Texture2D* texture = getTexture(prim.skinID);
		const Texture2D* nextTexture = !hasNext ? nullptr : getTexture(std::next(it)->skinID);

		bool primitivePreserveState = texture == nextTexture && (preserveState || hasNext);

		uint32_t skip = Mesh::SkipState::SkipNone;
		if (!lastPreserveState)
		{
			if (desc.modelMatrices && desc.matrixCount > 0)
			{
				bgfx::setTransform(desc.modelMatrices, desc.matrixCount);
			}
			if (texture)
			{
				program.SetTextureSampler("s_diffuse", 0, *texture);
			}
		}
		else
		{
			skip |= Mesh::SkipState::SkipRenderState;
			skip |= Mesh::SkipState::SkipVertexBuffer;
		}

		{
			if (desc.instanceBuffer && (skip & Mesh::SkipState::SkipInstanceBuffer) == 0)
			{
				bgfx::setInstanceDataBuffer(*desc.instanceBuffer, desc.instanceStart, desc.instanceCount);
			}
			if (subMesh.GetMesh().isIndexed() && (skip & Mesh::SkipState::SkipIndexBuffer) == 0)
			{
				subMesh.GetMesh().GetIndexBuffer().Bind(prim.indicesCount, prim.indicesOffset);
			}
			if ((skip & Mesh::SkipState::SkipVertexBuffer) == 0)
			{
				subMesh.GetMesh().GetVertexBuffer().Bind();
			}
			if ((skip & Mesh::SkipState::SkipRenderState) == 0)
			{
				bgfx::setState(desc.state, desc.rgba);
			}

			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), desc.program->GetRawHandle(), 0,
			             primitivePreserveState ? BGFX_DISCARD_NONE : BGFX_DISCARD_ALL);
		}
		lastPreserveState = primitivePreserveState;
	}
}

void Renderer::DrawMesh(const L3DMesh& mesh, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const
{

	if (mesh.GetNumSubMeshes() == 0)
	{
		spdlog::warn("Mesh {} has no submeshes to draw", mesh.GetDebugName());
		return;
	}

	const auto& subMeshes = mesh.GetSubMeshes();

	if (subMeshIndex != std::numeric_limits<uint8_t>::max())
	{
		if (subMeshIndex >= mesh.GetNumSubMeshes())
		{
			spdlog::warn("tried to draw submesh out of range ({}/{})", subMeshIndex, mesh.GetNumSubMeshes());
		}

		DrawSubMesh(mesh, *subMeshes[subMeshIndex], desc, desc.viewId, *desc.program, desc.state, desc.rgba, false);
		return;
	}

	for (auto it = subMeshes.begin(); it != subMeshes.end(); ++it)
	{
		const L3DSubMesh& subMesh = *it->get();
		if (!subMesh.isPhysics())
		{
			DrawSubMesh(mesh, subMesh, desc, desc.viewId, *desc.program, desc.state, desc.rgba,
			            std::next(it) != subMeshes.end());
		}
	}
}

void Renderer::DrawScene(const DrawSceneDesc& drawDesc) const
{
	// Reflection Pass
	{
		auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::ReflectionPass);
		if (drawDesc.drawWater)
		{
			DrawSceneDesc drawPassDesc = drawDesc;

			auto& frameBuffer = drawDesc.water.GetFrameBuffer();
			auto reflectionCamera = drawDesc.camera->Reflect(drawDesc.water.GetReflectionPlane());

			drawPassDesc.viewId = graphics::RenderPass::Reflection;
			drawPassDesc.camera = reflectionCamera.get();
			drawPassDesc.frameBuffer = &frameBuffer;
			drawPassDesc.drawWater = false;
			drawPassDesc.drawDebugCross = false;
			drawPassDesc.drawBoundingBoxes = false;
			drawPassDesc.cullBack = true;
			DrawPass(drawPassDesc);
		}
	}

	// Main Draw Pass
	{
		auto section = drawDesc.profiler.BeginScoped(Profiler::Stage::MainPass);
		DrawPass(drawDesc);
	}
}

void Renderer::DrawPass(const DrawSceneDesc& desc) const
{
	if (desc.frameBuffer)
	{
		desc.frameBuffer->Bind(desc.viewId);
	}
	// This dummy draw call is here to make sure that view is cleared if no
	// other draw calls are submitted to view
	bgfx::touch(static_cast<bgfx::ViewId>(desc.viewId));

	_shaderManager->SetCamera(desc.viewId, *desc.camera);

	auto objectShader = _shaderManager->GetShader("Object");
	auto waterShader = _shaderManager->GetShader("Water");
	auto terrainShader = _shaderManager->GetShader("Terrain");
	auto debugShader = _shaderManager->GetShader("DebugLine");
	auto debugShaderInstanced = _shaderManager->GetShader("DebugLineInstanced");
	auto objectShaderInstanced = _shaderManager->GetShader("ObjectInstanced");

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawSky
		                                                                               : Profiler::Stage::MainPassDrawSky);
		if (desc.drawSky)
		{
			auto modelMatrix = glm::mat4(1.0f);

			objectShader->SetTextureSampler("s_diffuse", 0, *desc.sky._texture);

			L3DMeshSubmitDesc submit_desc = {};
			submit_desc.viewId = desc.viewId;
			submit_desc.program = objectShader;
			submit_desc.state = BGFX_STATE_DEFAULT;
			if (!desc.cullBack)
			{
				submit_desc.state &= ~BGFX_STATE_CULL_MASK;
				submit_desc.state |= BGFX_STATE_CULL_CCW;
			}
			submit_desc.modelMatrices = &modelMatrix;
			submit_desc.matrixCount = 1;

			DrawMesh(*desc.sky._model, submit_desc, 0);
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawWater
		                                                                               : Profiler::Stage::MainPassDrawWater);
		if (desc.drawWater)
		{
			const auto& mesh = desc.water._mesh;
			mesh->GetIndexBuffer().Bind(mesh->GetIndexBuffer().GetCount(), 0);
			mesh->GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEFAULT);
			waterShader->SetTextureSampler("s_diffuse", 0, *desc.water._texture);
			waterShader->SetTextureSampler("s_reflection", 1, desc.water.GetFrameBuffer().GetColorAttachment());
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), waterShader->GetRawHandle());
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawIsland
		                                                                               : Profiler::Stage::MainPassDrawIsland);
		if (desc.drawIsland)
		{
			for (auto& block : desc.island.GetBlocks())
			{
				terrainShader->SetTextureSampler("s0_materials", 0, desc.island.GetAlbedoArray());
				terrainShader->SetTextureSampler("s1_bump", 1, desc.island.GetBump());
				terrainShader->SetTextureSampler("s2_smallBump", 2, desc.island.GetSmallBump());
				terrainShader->SetUniformValue("u_timeOfDay", &desc.timeOfDay);
				terrainShader->SetUniformValue("u_bumpmapStrength", &desc.bumpMapStrength);
				terrainShader->SetUniformValue("u_smallBumpmapStrength", &desc.smallBumpMapStrength);

				glm::vec4 mapPosition = block.GetMapPosition();
				terrainShader->SetUniformValue("u_blockPosition", &mapPosition);

				// clang-format off
				constexpr auto defaultState = 0u
					| BGFX_STATE_WRITE_MASK
					| BGFX_STATE_DEPTH_TEST_LESS
					| BGFX_STATE_BLEND_ALPHA
					| BGFX_STATE_MSAA
				;
				// clang-format on

				block.GetMesh().GetVertexBuffer().Bind();
				bgfx::setState(defaultState | (desc.cullBack ? BGFX_STATE_CULL_CCW : BGFX_STATE_CULL_CW), 0);
				bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), terrainShader->GetRawHandle());
			}
		}
	}

	{
		auto section = desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawModels
		                                                                               : Profiler::Stage::MainPassDrawModels);
		if (desc.drawEntities)
		{
			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.program = objectShaderInstanced;
			// clang-format off
			submitDesc.state = 0u
				| BGFX_STATE_WRITE_MASK
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_MSAA
			;
			// clang-format on
			auto& renderCtx = desc.entities.Context().renderContext;

			// Instance meshes
			for (const auto& [meshId, placers] : renderCtx.instancedDrawDescs)
			{
				const L3DMesh& mesh = Game::instance()->GetMeshPack().GetMesh(static_cast<uint32_t>(meshId));
				submitDesc.instanceBuffer = &renderCtx.instanceUniformBuffer;
				submitDesc.instanceStart = placers.offset;
				submitDesc.instanceCount = placers.count;
				if (mesh.IsBoned())
				{
					submitDesc.modelMatrices = mesh.GetBoneMatrices().data();
					submitDesc.matrixCount = mesh.GetBoneMatrices().size();
					// TODO(bwrsandman): Get animation frame instead of default
				}
				else
				{
					const auto identity = glm::mat4(1.0f);
					submitDesc.modelMatrices = &identity;
					submitDesc.matrixCount = 1;
				}
				// TODO(bwrsandman): choose the correct LOD
				DrawMesh(mesh, submitDesc, std::numeric_limits<uint8_t>::max());
			}

			// Debug
			if (desc.viewId == graphics::RenderPass::Main)
			{
				if (renderCtx.boundingBox)
				{
					auto boundBoxOffset = renderCtx.instanceUniforms.size() / 2;
					auto boundBoxCount = renderCtx.instanceUniforms.size() / 2;
					renderCtx.boundingBox->GetMesh().GetVertexBuffer().Bind();
					bgfx::setInstanceDataBuffer(renderCtx.instanceUniformBuffer, boundBoxOffset, boundBoxCount);
					bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShaderInstanced->GetRawHandle());
				}
				if (renderCtx.streams)
				{
					bgfx::setTransform(&renderCtx.streams->GetModel());
					renderCtx.streams->GetMesh().GetVertexBuffer().Bind();
					bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
					bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
				}
			}
		}

		if (desc.drawTestModel)
		{
			L3DMeshSubmitDesc submitDesc = {};
			submitDesc.viewId = desc.viewId;
			submitDesc.program = _shaderManager->GetShader("Object");
			// clang-format off
			submitDesc.state = 0u
				| BGFX_STATE_WRITE_MASK
				| BGFX_STATE_DEPTH_TEST_LESS
				| BGFX_STATE_CULL_CCW
				| BGFX_STATE_MSAA
			;
			// clang-format on
			submitDesc.modelMatrices = desc.testModel.GetBoneMatrices().data();
			submitDesc.matrixCount = static_cast<uint8_t>(desc.testModel.GetBoneMatrices().size());
			desc.testModel.Submit(submitDesc, 0);
		}
	}

	{
		auto section =
		    desc.profiler.BeginScoped(desc.viewId == RenderPass::Reflection ? Profiler::Stage::ReflectionDrawDebugCross
		                                                                    : Profiler::Stage::MainPassDrawDebugCross);
		if (desc.drawDebugCross)
		{
			bgfx::setTransform(&_debugCross->GetModel());
			_debugCross->GetMesh().GetVertexBuffer().Bind();
			bgfx::setState(BGFX_STATE_DEFAULT | BGFX_STATE_PT_LINES);
			bgfx::submit(static_cast<bgfx::ViewId>(desc.viewId), debugShader->GetRawHandle());
		}
	}

	// Enable stats or debug text.
	auto debugMode = BGFX_DEBUG_NONE;
	if (desc.bgfxDebug)
	{
		debugMode |= BGFX_DEBUG_STATS;
	}
	if (desc.wireframe)
	{
		debugMode |= BGFX_DEBUG_WIREFRAME;
	}
	if (desc.profile)
	{
		debugMode |= BGFX_DEBUG_PROFILER;
	}
	bgfx::setDebug(debugMode);
}

void Renderer::Frame()
{
	// Advance to next frame. Process submitted rendering primitives.
	bgfx::frame();
}
