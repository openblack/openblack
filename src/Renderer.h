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
  const std::string name;
  const std::string vertexShaderFile;
  const std::string fragmentShaderFile;
};

static const std::array Shaders {
	ShaderDefinition{"DebugLine", "shaders/vs_line.sc", "shaders/fs_line.sc"},
	ShaderDefinition{"Terrain", "shaders/vs_terrain.sc", "shaders/fs_terrain.sc"},
	ShaderDefinition{"Object", "shaders/vs_object.sc", "shaders/fs_object.sc"},
	ShaderDefinition{"Water", "shaders/vs_water.sc", "shaders/fs_water.sc"},
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
		Sky& sky;
		bool drawWater;
		Water& water;
		bool drawIsland;
		LandIsland& island;
		bool drawEntities;
		Entities::Registry& entities;
		bool drawDebugCross;
		bool cullBack;
		bool bgfxDebug;
		bool wireframe;
		bool profile;
	};

	Renderer() = delete;
	explicit Renderer(const GameWindow& window, bool vsync, const std::string binaryPath);

	virtual ~Renderer();

	void LoadShaders(const std::string &binaryPath);
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
