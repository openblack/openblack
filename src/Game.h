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
#ifndef OPENBLACK_GAME_H
#define OPENBLACK_GAME_H

#include "GameWindow.h"

#include <LHVM/LHVM.hpp>
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
class L3DModel;
class SkinnedModel;
class Sky;
class Water;

namespace Graphics
{
class ShaderProgram;
class ShaderManager;
} // namespace Graphics

namespace LHScriptX
{
class Script;
}

namespace Video
{
class VideoPlayer;
}

class Game
{
  public:
	Game(int argc, char** argv);
	~Game();

	void Run();

	void LoadMap(const std::string& name);
	void LoadLandscape(const std::string& name);

	const std::string& GetGamePath();

	GameWindow& GetWindow() { return *_window; }
	Camera& GetCamera() { return *_camera; }
	MeshPack& GetMeshPack() { return *_meshPack; }
	FileSystem& GetFileSystem() { return *_fileSystem; }
	Graphics::ShaderManager& GetShaderManager() { return *_shaderManager; }

	static Game* instance()
	{
		return sInstance;
	}

  private:
	static Game* sInstance;

	std::unique_ptr<Graphics::ShaderManager> _shaderManager;

	std::unique_ptr<GameWindow> _window;
	std::unique_ptr<Camera> _camera;

	std::unique_ptr<FileSystem> _fileSystem;
	std::unique_ptr<LandIsland> _landIsland;
	std::unique_ptr<MeshPack> _meshPack;
	//std::unique_ptr<Video::VideoPlayer> _videoPlayer;

	std::unique_ptr<SkinnedModel> _testModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<LHScriptX::Script> _scriptx;
	std::unique_ptr<LHVM::LHVM> _lhvm;

	bool _wireframe;
	float _timeOfDay;
	float _bumpmapStrength;

	glm::ivec2 _mousePosition;
	glm::vec3 _intersection;

	glm::vec3 _modelPosition;
	glm::vec3 _modelRotation;
	glm::vec3 _modelScale;

	bool _running;

	void guiLoop();
};
} // namespace OpenBlack

#endif
