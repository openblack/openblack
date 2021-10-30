/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
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
#include <array>
#include <memory>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <entt/entity/fwd.hpp>
#include <glm/glm.hpp>
#include <spdlog/common.h>

#include "GameWindow.h"
#include "InfoConstants.h"

typedef union SDL_Event SDL_Event;

namespace openblack
{
class AnimationPack;
class Camera;
class FileSystem;
class GameWindow;
class EventManager;
class MeshPack;
class LandIsland;
class Profiler;
class Renderer;
class L3DAnim;
class L3DMesh;
class Sky;
class Water;

namespace gui
{
class Gui;
}

namespace LHVM
{
class LHVM;
}

namespace ecs
{
namespace components
{
struct Transform;
}
namespace systems
{
class DynamicsSystem;
}
class Registry;
} // namespace ecs

namespace lhscriptx
{
class Script;
}

enum class LoggingSubsystem : uint8_t
{
	game,
	graphics,
	scripting,

	_count
};

static std::array<std::string_view, static_cast<size_t>(LoggingSubsystem::_count)> LoggingSubsystemStrs {
    "game",
    "graphics",
    "scripting",
};

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
	std::array<spdlog::level::level_enum, LoggingSubsystemStrs.size()> logLevels;
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

		float timeOfDay {12.0f};
		float skyAlignment {0.0f};
		float bumpMapStrength {1.0f};
		float smallBumpMapStrength {1.0f};

		bool bgfxDebug {false};
		bool bgfxProfile {false};
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
	void SetTime(float time);
	void SetGameSpeed(float multiplier) { _gameSpeedMultiplier = multiplier; }
	float GetGameSpeed() const { return _gameSpeedMultiplier; }

	GameWindow* GetWindow() { return _window.get(); }
	[[nodiscard]] const GameWindow& GetWindow() const { return *_window; }
	Camera& GetCamera() { return *_camera; }
	[[nodiscard]] Profiler& GetProfiler() const { return *_profiler; }
	[[nodiscard]] Renderer& GetRenderer() const { return *_renderer; }
	const ecs::systems::DynamicsSystem& GetDynamicsSystem() const { return *_dynamicsSystem; }
	[[nodiscard]] Camera& GetCamera() const { return *_camera; }
	[[nodiscard]] Sky& GetSky() const { return *_sky; }
	[[nodiscard]] Water& GetWater() const { return *_water; }
	LandIsland& GetLandIsland() { return *_landIsland; }
	[[nodiscard]] LandIsland& GetLandIsland() const { return *_landIsland; }
	[[nodiscard]] L3DMesh& GetTestModel() const { return *_testModel; }
	[[nodiscard]] L3DMesh& GetHandModel() const { return *_handModel; }
	entt::entity GetHand() const;
	AnimationPack& GetAnimationPack() { return *_animationPack; }
	MeshPack& GetMeshPack() { return *_meshPack; }
	[[nodiscard]] const LHVM::LHVM& GetLhvm() { return *_lhvm; }
	FileSystem& GetFileSystem() { return *_fileSystem; }
	ecs::Registry& GetEntityRegistry() { return *_entityRegistry; }
	[[nodiscard]] ecs::Registry& GetEntityRegistry() const { return *_entityRegistry; }
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
	std::unique_ptr<ecs::systems::DynamicsSystem> _dynamicsSystem;
	std::unique_ptr<gui::Gui> _gui;
	std::unique_ptr<Camera> _camera;
	std::unique_ptr<Profiler> _profiler;
	std::unique_ptr<EventManager> _eventManager;

	std::unique_ptr<FileSystem> _fileSystem;
	std::unique_ptr<LandIsland> _landIsland;
	std::unique_ptr<MeshPack> _meshPack;
	std::unique_ptr<AnimationPack> _animationPack;

	// std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DAnim> _testAnimation;
	std::unique_ptr<L3DMesh> _handModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<lhscriptx::Script> _scriptx;
	std::unique_ptr<LHVM::LHVM> _lhvm;
	std::unique_ptr<ecs::Registry> _entityRegistry;

	InfoConstants _infoConstants;
	Config _config;

	std::chrono::steady_clock::time_point _lastGameLoopTime;
	std::chrono::steady_clock::duration _turnDeltaTime;
	float _gameSpeedMultiplier;
	uint32_t _frameCount;
	uint16_t _turnCount;

	glm::ivec2 _mousePosition;
	glm::mat4 _handPose;

	entt::entity _handEntity;
	bool _handGripping;
};
} // namespace openblack
