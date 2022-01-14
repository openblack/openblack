/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "Graphics/RenderPass.h"

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <glm/fwd.hpp>
#include <glm/mat4x4.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace openblack
{
struct BgfxCallback;
class Camera;
class GameWindow;
class Game;
class L3DAnim;
class L3DMesh;
class L3DSubMesh;
class LandIsland;
class MeshPack;
class Profiler;
class Sky;
class Water;

namespace ecs
{
class Registry;
}

namespace graphics
{
class DebugLines;
class FrameBuffer;
class ShaderManager;
class ShaderProgram;
} // namespace graphics

struct ShaderDefinition
{
	const std::string_view name;
	const std::string_view vertexShaderName;
	const std::string_view fragmentShaderName;
};

constexpr std::array Shaders {
    ShaderDefinition {"DebugLine", "vs_line", "fs_line"},
    ShaderDefinition {"DebugLineInstanced", "vs_line_instanced", "fs_line"},
    ShaderDefinition {"Terrain", "vs_terrain", "fs_terrain"},
    ShaderDefinition {"Object", "vs_object", "fs_object"},
    ShaderDefinition {"ObjectInstanced", "vs_object_instanced", "fs_object"},
    ShaderDefinition {"Sky", "vs_object", "fs_sky"},
    ShaderDefinition {"Water", "vs_water", "fs_water"},
};

class Renderer
{

public:
	enum class Api
	{
		OpenGl,
	};

	struct RequiredAttribute
	{
		Api api;
		uint32_t name;
		int value;
	};
	struct DrawSceneDesc
	{
		uint32_t time;
		graphics::RenderPass viewId;
		Profiler& profiler;
		const Camera* camera;
		const graphics::FrameBuffer* frameBuffer;
		bool drawSky;
		const Sky& sky;
		bool drawWater;
		const Water& water;
		bool drawIsland;
		const LandIsland& island;
		bool drawEntities;
		const ecs::Registry& entities;
		bool drawTestModel;
		const L3DMesh& testModel;
		const L3DAnim& testAnimation;
		bool drawDebugCross;
		bool drawBoundingBoxes;
		bool cullBack;
		bool bgfxDebug;
		bool bgfxProfile;
		bool wireframe;
		float timeOfDay;
		float bumpMapStrength;
		float smallBumpMapStrength;
	};

	struct L3DMeshSubmitDesc
	{
		graphics::RenderPass viewId;
		const graphics::ShaderProgram* program;
		uint64_t state;
		uint32_t rgba;
		const glm::mat4* modelMatrices;
		uint8_t matrixCount;
		const bgfx::DynamicVertexBufferHandle* instanceBuffer;
		uint32_t instanceStart;
		uint32_t instanceCount;
		bool isSky;
		float skyType;
	};

	Renderer() = delete;
	explicit Renderer(const GameWindow* window, bgfx::RendererType::Enum rendererType, bool vsync);

	virtual ~Renderer();

	void LoadShaders();
	[[nodiscard]] graphics::ShaderManager& GetShaderManager() const;

	void UpdateDebugCrossUniforms(const glm::mat4& pose);

	void ConfigureView(graphics::RenderPass viewId, uint16_t width, uint16_t height) const;

	void DrawScene(const MeshPack& meshPack, const DrawSceneDesc& desc) const;
	void DrawMesh(const L3DMesh& mesh, const MeshPack& meshPack, const L3DMeshSubmitDesc& desc, uint8_t subMeshIndex) const;
	void Frame();

private:
	void DrawSubMesh(const L3DMesh& mesh, const L3DSubMesh& subMesh, const MeshPack& meshPack, const L3DMeshSubmitDesc& desc,
	                 bool preserveState) const;
	void DrawPass(const MeshPack& meshPack, const DrawSceneDesc& desc) const;

	std::unique_ptr<graphics::ShaderManager> _shaderManager;
	std::unique_ptr<BgfxCallback> _bgfxCallback;

	std::unique_ptr<graphics::DebugLines> _debugCross;
	glm::mat4 _debugCrossPose;
};
} // namespace openblack
