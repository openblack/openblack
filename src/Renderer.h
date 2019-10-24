/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <array>
#include <chrono>
#include <memory>
#include <string_view>
#include <vector>

#include <SDL.h>
#include <3D/Camera.h>

namespace openblack
{
struct BgfxCallback;
class GameWindow;
class Game;
class LandIsland;
class Profiler;
class Sky;
class Water;

namespace Entities
{
class Registry;
}

namespace graphics
{
class ShaderProgram;
class ShaderManager;
class DebugLines;
} // namespace graphics


struct ShaderDefinition {
  const std::string_view name;
  const std::string_view vertexShaderName;
  const std::string_view fragmentShaderName;
};

constexpr std::array Shaders {
	ShaderDefinition{"DebugLine", "vs_line", "fs_line"},
	ShaderDefinition{"Terrain", "vs_terrain", "fs_terrain"},
	ShaderDefinition{"Object", "vs_object", "fs_object"},
	ShaderDefinition{"Water", "vs_water", "fs_water"},
};

class Renderer {

  public:
	enum class Api {
		OpenGl,
	};
	struct RequiredAttribute {
		Api api;
		uint32_t name;
		int value;
	};
	struct DrawSceneDesc {
		uint8_t viewId;
		Profiler& profiler;
		bool drawSky;
		const Sky& sky;
		bool drawWater;
		const Water& water;
		bool drawIsland;
		const LandIsland& island;
		bool drawEntities;
		const Entities::Registry& entities;
		bool drawDebugCross;
		bool cullBack;
		bool bgfxDebug;
		bool wireframe;
		bool profile;
	};

	Renderer() = delete;
	explicit Renderer(const GameWindow& window, bool vsync);

	virtual ~Renderer();

	void LoadShaders();
	[[nodiscard]] graphics::ShaderManager& GetShaderManager() const;

	void UpdateDebugCrossPose(std::chrono::microseconds dt, const glm::vec3 &position, float scale);

	void UploadUniforms(std::chrono::microseconds dt, uint8_t viewId, const Game &game, const Camera &camera);
	void ClearScene(uint8_t viewId, int width, int height);
	void DrawScene(const DrawSceneDesc &desc);
	void Frame();

  private:
	std::unique_ptr<graphics::ShaderManager> _shaderManager;
	std::unique_ptr<BgfxCallback> _bgfxCallback;

	std::unique_ptr<graphics::DebugLines> _debugCross;
};
} // namespace openblack
