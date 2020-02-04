/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
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
class LandIsland;
class Profiler;
class Sky;
class Water;

namespace entities
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
		const entities::Registry& entities;
		bool drawDebugCross;
		bool drawBoundingBoxes;
		bool cullBack;
		bool bgfxDebug;
		bool wireframe;
		bool profile;
	};

	Renderer() = delete;
	explicit Renderer(const GameWindow* window, bgfx::RendererType::Enum rendererType, bool vsync);

	virtual ~Renderer();

	void LoadShaders();
	[[nodiscard]] graphics::ShaderManager& GetShaderManager() const;

	void UpdateDebugCrossUniforms(const glm::vec3& position, float scale);

	void ConfigureView(graphics::RenderPass viewId, uint16_t width, uint16_t height) const;

	void DrawScene(const DrawSceneDesc& desc) const;
	void Frame();

private:
	void DrawPass(const DrawSceneDesc& desc) const;

	std::unique_ptr<graphics::ShaderManager> _shaderManager;
	std::unique_ptr<BgfxCallback> _bgfxCallback;

	std::unique_ptr<graphics::DebugLines> _debugCross;
};
} // namespace openblack
