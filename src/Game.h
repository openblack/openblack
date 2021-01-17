/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#ifdef HAS_FILESYSTEM
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif // HAS_FILESYSTEM
#include <memory>
#include <string>
#include <vector>

#include <SDL.h>
#include <bgfx/bgfx.h>
#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>

#include <LHVM/LHVM.h>

#include "GameWindow.h"
#include "InfoConstants.h"

namespace openblack
{
class AnimationPack;
class Camera;
class FileSystem;
class GameWindow;
class Gui;
class EventManager;
class MeshPack;
class LandIsland;
class Profiler;
class Renderer;
class L3DAnim;
class L3DMesh;
class Sky;
class Water;
namespace entities::components
{
struct Transform;
}

namespace audio
{
class SoundPack;
class SoundHandler;
} // namespace audio

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
	static constexpr auto kTurnDuration = std::chrono::milliseconds(100);
	static constexpr float kTurnDurationMultiplierSlow = 2.0f;
	static constexpr float kTurnDurationMultiplierNormal = 1.0f;
	static constexpr float kTurnDurationMultiplierFast = 0.5f;

	struct Config
	{
		Config() {}

		bool wireframe {false};
		bool waterDebug {false};
		bool showProfiler {false};
		bool showLandIsland {false};
		bool showAudioDebugger {false};
		bool showVillagerNames {false};
		bool debugVillagerNames {false};

		bool drawSky {true};
		bool drawWater {true};
		bool drawIsland {true};
		bool drawEntities {true};
		bool drawTestModel {true};
		bool drawDebugCross {true};
		bool drawBoundingBoxes {false};
		bool drawFootpaths {false};
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
	bool GameLogicLoop();
	bool Update();
	bool Run();

	void LoadMap(const fs::path& path);
	void LoadLandscape(const fs::path& path);

	bool LoadVariables();

	void SetGamePath(const fs::path& path);
	const fs::path& GetGamePath();
	void SetGameSpeed(float multiplier) { _gameSpeedMultiplier = multiplier; }
	float GetGameSpeed() const { return _gameSpeedMultiplier; }

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
	[[nodiscard]] L3DMesh& GetTestModel() const { return *_testModel; }
	[[nodiscard]] L3DMesh& GetHandModel() const { return *_handModel; }
	const entities::components::Transform& GetHandTransform() const;
	entities::components::Transform& GetHandTransform();
	AnimationPack& GetAnimationPack() { return *_animationPack; }
	MeshPack& GetMeshPack() { return *_meshPack; }
	audio::SoundHandler& GetSoundHandler() const { return *_soundHandler; }
	[[nodiscard]] const LHVM::LHVM* GetLhvm() { return _lhvm.get(); }
	FileSystem& GetFileSystem() { return *_fileSystem; }
	entities::Registry& GetEntityRegistry() { return *_entityRegistry; }
	[[nodiscard]] entities::Registry& GetEntityRegistry() const { return *_entityRegistry; }
	const InfoConstants& GetInfoConstants() { return _infoConstants; } ///< Access should be only read-only
	Config& GetConfig() { return _config; }
	[[nodiscard]] const Config& GetConfig() const { return _config; }
	[[nodiscard]] uint16_t GetTurn() const { return _turnCount; }
	[[nodiscard]] std::chrono::duration<float, std::milli> GetDeltaTime() const { return _turnDeltaTime; }
	[[nodiscard]] const glm::ivec2& GetMousePosition() const { return _mousePosition; }

	static Game* instance() { return sInstance; }

private:
	static Game* sInstance;

	/// path to Lionhead Studios Ltd/Black & White folder
	fs::path _gamePath;

	std::unique_ptr<GameWindow> _window;
	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<Gui> _gui;
	std::unique_ptr<Camera> _camera;
	std::unique_ptr<Profiler> _profiler;
	std::unique_ptr<EventManager> _eventManager;

	std::unique_ptr<FileSystem> _fileSystem;
	std::unique_ptr<LandIsland> _landIsland;
	std::unique_ptr<MeshPack> _meshPack;
	std::unique_ptr<AnimationPack> _animationPack;
	std::unique_ptr<audio::SoundHandler> _soundHandler;

	// std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DAnim> _testAnimation;
	std::unique_ptr<L3DMesh> _handModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<lhscriptx::Script> _scriptx;
	std::unique_ptr<LHVM::LHVM> _lhvm;
	std::unique_ptr<entities::Registry> _entityRegistry;

	InfoConstants _infoConstants;
	Config _config;

	std::chrono::steady_clock::time_point _lastGameLoopTime;
	std::chrono::steady_clock::duration _turnDeltaTime;
	float _gameSpeedMultiplier;
	uint32_t _frameCount;
	uint16_t _turnCount;

	glm::ivec2 _mousePosition;
	glm::vec3 _intersection;

	entt::entity _handEntity;
	bool _handGripping;
};
} // namespace openblack
