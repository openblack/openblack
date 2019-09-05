/* OpenBlack - A reimplementation of Lionhead's Black & White.
 *
 * OpenBlack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * OpenBlack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * OpenBlack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with OpenBlack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "GameWindow.h"

#include <LHVM/LHVM.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

namespace OpenBlack
{
class Camera;
class FileSystem;
class MeshPack;
class MeshViewer;
class LandIsland;
class Renderer;
class L3DMesh;
class Sky;
class Water;

namespace LHScriptX
{
class Script;
}

namespace Entities
{
class Registry;
}

class Game
{
  public:
	Game(int argc, char** argv);
	~Game();

	void Run();

	void LoadMap(const std::string& name);
	void LoadLandscape(const std::string& name);

	void SetGamePath(const std::string &gamePath);
	const std::string& GetGamePath();

	GameWindow& GetWindow() { return *_window; }
	Camera& GetCamera() { return *_camera; }
	[[nodiscard]] Camera& GetCamera() const { return *_camera; }
	[[nodiscard]] Sky& GetSky() const { return *_sky; }
	[[nodiscard]] Water& GetWater() const { return *_water; }
	LandIsland& GetLandIsland() { return *_landIsland; }
	[[nodiscard]] LandIsland& GetLandIsland() const { return *_landIsland; }
	[[nodiscard]] SkinnedModel& GetTestModel() const { return *_testModel; }
	MeshPack& GetMeshPack() { return *_meshPack; }
	FileSystem& GetFileSystem() { return *_fileSystem; }
	Entities::Registry& GetEntityRegistry() { return *_entityRegistry; }
	[[nodiscard]] Entities::Registry& GetEntityRegistry() const { return *_entityRegistry; }
	[[nodiscard]] float GetTimeOfDay() const { return _timeOfDay; }
	[[nodiscard]] float GetBumpmapStrength() const { return _bumpmapStrength; }
	[[nodiscard]] float GetSmallBumpmapStrength() const { return _smallBumpmapStrength; }
	[[nodiscard]] glm::mat4 GetModelMatrix() const;
	[[nodiscard]] bool GetIsWireframe() const { return _wireframe; }

	static Game* instance()
	{
		return sInstance;
	}

  private:
	static Game* sInstance;

	std::string sGamePath; // path to Lionhead Studios Ltd/Black & White folder

	std::unique_ptr<GameWindow> _window;
	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<Camera> _camera;

	std::unique_ptr<FileSystem> _fileSystem;
	std::unique_ptr<LandIsland> _landIsland;
	std::unique_ptr<MeshPack> _meshPack;
	std::unique_ptr<MeshViewer> _meshViewer;

	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<LHScriptX::Script> _scriptx;
	std::unique_ptr<LHVM::LHVM> _lhvm;
	std::unique_ptr<Entities::Registry> _entityRegistry;

	bool _wireframe;
	bool _waterDebug;

	float _timeOfDay;
	float _bumpmapStrength;
	float _smallBumpmapStrength;

	glm::ivec2 _mousePosition;
	glm::vec3 _intersection;

	glm::vec3 _modelPosition;
	glm::vec3 _modelRotation;
	glm::vec3 _modelScale;

	bool _running;

	void guiLoop();
};
} // namespace OpenBlack
