/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "GameWindow.h"

#include <LHVM/LHVM.h>
#include <SDL.h>
#include <bgfx/bgfx.h>
#include <glm/glm.hpp>

#include <entt/entity/fwd.hpp>
#include <memory>
#include <string>
#include <vector>

namespace openblack
{
class Camera;
class FileSystem;
class GameWindow;
class Gui;
class EventManager;
class MeshPack;
class LandIsland;
class Profiler;
class Renderer;
class L3DMesh;
class Sky;
class Water;
struct Transform;

namespace lhscriptx
{
class Script;
}

namespace entities
{
class Registry;
}

struct Arguments
{
	std::string executablePath;
	int windowWidth;
	int windowHeight;
	bool vsync;
	openblack::DisplayMode displayMode;
	bgfx::RendererType::Enum rendererType;
	std::string gamePath;
	float scale;
	uint32_t numFramesToSimulate;
	std::string logFile;
	uint32_t logLevel;
};

class Game
{
public:
	struct Config
	{
		Config() {}

		bool wireframe {false};
		bool waterDebug {false};
		bool showProfiler {false};
		bool showLandIsland {false};

		bool drawSky {true};
		bool drawWater {true};
		bool drawIsland {true};
		bool drawEntities {true};
		bool drawTestModel {true};
		bool drawDebugCross {true};
		bool drawBoundingBoxes {false};
		bool drawStreams {false};

		float timeOfDay {1.0f};
		float bumpMapStrength {1.0f};
		float smallBumpMapStrength {1.0f};

		bool bgfxDebug {false};
		bool running {false};

		uint32_t numFramesToSimulate {0};
	};

	Game(Arguments&& args);
	virtual ~Game();

	bool ProcessEvents(const SDL_Event& event);
	bool Update();
	void Run();

	void LoadMap(const std::string& name);
	void LoadLandscape(const std::string& name);

	void LoadVariables();

	void SetGamePath(const std::string& gamePath);
	const std::string& GetGamePath();

	GameWindow* GetWindow() { return _window.get(); }
	[[nodiscard]] const GameWindow& GetWindow() const { return *_window; }
	Camera& GetCamera() { return *_camera; }
	[[nodiscard]] Profiler& GetProfiler() const { return *_profiler; }
	[[nodiscard]] Renderer& GetRenderer() const { return *_renderer; }
	[[nodiscard]] Camera& GetCamera() const { return *_camera; }
	[[nodiscard]] Sky& GetSky() const { return *_sky; }
	[[nodiscard]] Water& GetWater() const { return *_water; }
	LandIsland& GetLandIsland() { return *_landIsland; }
	[[nodiscard]] LandIsland& GetLandIsland() const { return *_landIsland; }
	// [[nodiscard]] L3DMesh& GetTestModel() const { return *_testModel; }
	[[nodiscard]] L3DMesh& GetHandModel() const { return *_handModel; }
	const Transform& GetHandTransform() const;
	Transform& GetHandTransform();
	MeshPack& GetMeshPack() { return *_meshPack; }
	[[nodiscard]] const LHVM::LHVM* GetLhvm() { return _lhvm.get(); }
	FileSystem& GetFileSystem() { return *_fileSystem; }
	entities::Registry& GetEntityRegistry() { return *_entityRegistry; }
	[[nodiscard]] entities::Registry& GetEntityRegistry() const { return *_entityRegistry; }
	Config& GetConfig() { return _config; }
	[[nodiscard]] const Config& GetConfig() const { return _config; }
	[[nodiscard]] const glm::ivec2& GetMousePosition() const { return _mousePosition; }

	static Game* instance() { return sInstance; }

private:
	static Game* sInstance;

	std::string sGamePath; // path to Lionhead Studios Ltd/Black & White folder

	std::unique_ptr<GameWindow> _window;
	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<Gui> _gui;
	std::unique_ptr<Camera> _camera;
	std::unique_ptr<Profiler> _profiler;
	std::unique_ptr<EventManager> _eventManager;

	std::unique_ptr<FileSystem> _fileSystem;
	std::unique_ptr<LandIsland> _landIsland;
	std::unique_ptr<MeshPack> _meshPack;

	// std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _handModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<lhscriptx::Script> _scriptx;
	std::unique_ptr<LHVM::LHVM> _lhvm;
	std::unique_ptr<entities::Registry> _entityRegistry;

	Config _config;

	uint32_t _frameCount;

	glm::ivec2 _mousePosition;
	glm::vec3 _intersection;

	entt::entity _handEntity;
	bool _handGripping;
};
} // namespace openblack
