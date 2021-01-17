/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include <bgfx/bgfx.h>
#include <entt/entity/fwd.hpp>
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4701) // func_common.inl glm::any template
#endif
#include <glm/mat4x4.hpp>
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#include <spdlog/common.h>

#include "GameWindow.h"
#include "InfoConstants.h"

union SDL_Event;

namespace openblack
{
class Camera;
class FileSystem;
class GameWindow;
class EventManager;
class LandIsland;
class Profiler;
class Renderer;
class L3DAnim;
class L3DMesh;
class Sky;
class Water;

namespace debug::gui
{
class Gui;
}

namespace graphics
{
class Texture2D;
}

namespace LHVM
{
class LHVM;
}

namespace ecs
{
class Map;
class Registry;
} // namespace ecs

namespace audio
{
class AudioManager;
} // namespace audio

namespace lhscriptx
{
class Script;
}

enum class LoggingSubsystem : uint8_t
{
	game,
	graphics,
	scripting,
	audio,
	pathfinding,
	ai,

	_count
};

constexpr static std::array<std::string_view, static_cast<size_t>(LoggingSubsystem::_count)> k_LoggingSubsystemStrs {
    "game",        //
    "graphics",    //
    "scripting",   //
    "audio",       //
    "pathfinding", //
    "ai",          //
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
	std::array<spdlog::level::level_enum, k_LoggingSubsystemStrs.size()> logLevels;
	std::string startLevel;
	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> requestScreenshot;
};

class Game
{
public:
	static constexpr auto k_TurnDuration = std::chrono::milliseconds(100);
	static constexpr float k_TurnDurationMultiplierSlow = 2.0f;
	static constexpr float k_TurnDurationMultiplierNormal = 1.0f;
	static constexpr float k_TurnDurationMultiplierFast = 0.5f;

	struct Config
	{
		bool wireframe {false};
		bool showVillagerNames {false};
		bool debugVillagerNames {false};
		bool debugVillagerStates {false};

		bool drawSky {true};
		bool drawWater {true};
		bool drawIsland {true};
		bool drawEntities {true};
		bool drawSprites {true};
		bool drawTestModel {true};
		bool drawDebugCross {true};
		bool drawBoundingBoxes {false};
		bool drawFootpaths {false};
		bool drawStreams {false};

		float timeOfDay {12.0f};
		float skyAlignment {0.0f};
		float bumpMapStrength {1.0f};
		float smallBumpMapStrength {1.0f};

		float cameraXFov {70.0f};
		float cameraNearClip {1.0f};
		float cameraFarClip {static_cast<float>(0x10000)};

		bool bgfxDebug {false};
		bool bgfxProfile {false};
		bool running {false};

		uint32_t numFramesToSimulate {0};
	};

	explicit Game(Arguments&& args);
	virtual ~Game();

	bool ProcessEvents(const SDL_Event& event);
	bool GameLogicLoop();
	bool Update();
	bool Initialize();
	bool Run();

	void LoadMap(const std::filesystem::path& path);
	void LoadLandscape(const std::filesystem::path& path);

	bool LoadVariables();

	void SetGamePath(std::filesystem::path gamePath);
	const std::filesystem::path& GetGamePath();
	void SetTime(float time);
	void SetGameSpeed(float multiplier) { _gameSpeedMultiplier = multiplier; }
	[[nodiscard]] float GetGameSpeed() const { return _gameSpeedMultiplier; }

	GameWindow* GetWindow() { return _window.get(); }
	[[nodiscard]] const GameWindow& GetWindow() const { return *_window; }
	Camera& GetCamera() { return *_camera; }
	[[nodiscard]] Profiler& GetProfiler() const { return *_profiler; }
	[[nodiscard]] Renderer& GetRenderer() const { return *_renderer; }
	[[nodiscard]] Camera& GetCamera() const { return *_camera; }
	[[nodiscard]] Sky& GetSky() const { return *_sky; }
	[[nodiscard]] Water& GetWater() const { return *_water; }
	LandIsland& GetLandIsland()
	{
		if (_landIsland == nullptr)
		{
			throw std::runtime_error("Cannot get landscape before any are loaded");
		}
		return *_landIsland;
	}
	[[nodiscard]] LandIsland& GetLandIsland() const
	{
		if (_landIsland == nullptr)
		{
			throw std::runtime_error("Cannot get landscape before any are loaded");
		}
		return *_landIsland;
	}
	[[nodiscard]] entt::entity GetHand() const;
	[[nodiscard]] const LHVM::LHVM& GetLhvm() const { return *_lhvm; }
	LHVM::LHVM& GetLhvm() { return *_lhvm; }
	FileSystem& GetFileSystem() { return *_fileSystem; }
	ecs::Registry& GetEntityRegistry() { return *_entityRegistry; }
	[[nodiscard]] ecs::Registry& GetEntityRegistry() const { return *_entityRegistry; }
	const InfoConstants& GetInfoConstants() { return _infoConstants; } ///< Access should be only read-only
	[[nodiscard]] ecs::Map& GetEntityMap() { return *_entityMap; }
	[[nodiscard]] const ecs::Map& GetEntityMap() const { return *_entityMap; }
	Config& GetConfig() { return _config; }
	[[nodiscard]] const Config& GetConfig() const { return _config; }
	[[nodiscard]] uint16_t GetTurn() const { return _turnCount; }
	[[nodiscard]] bool IsPaused() const { return _paused; }
	[[nodiscard]] std::chrono::duration<float, std::milli> GetDeltaTime() const { return _turnDeltaTime; }
	[[nodiscard]] const glm::ivec2& GetMousePosition() const { return _mousePosition; }

	void RequestScreenshot(const std::filesystem::path& path);

	static Game* Instance() { return sInstance; }

private:
	static Game* sInstance;

	/// path to Lionhead Studios Ltd/Black & White folder
	std::filesystem::path _gamePath;

	std::unique_ptr<GameWindow> _window;
	std::unique_ptr<Renderer> _renderer;
	std::unique_ptr<debug::gui::Gui> _gui;
	std::unique_ptr<Camera> _camera;
	std::unique_ptr<Profiler> _profiler;
	std::unique_ptr<EventManager> _eventManager;

	std::unique_ptr<FileSystem> _fileSystem;
	std::unique_ptr<LandIsland> _landIsland;

	// std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DAnim> _testAnimation;
	std::unique_ptr<L3DMesh> _handModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<lhscriptx::Script> _scriptx;
	std::unique_ptr<LHVM::LHVM> _lhvm;
	std::unique_ptr<ecs::Registry> _entityRegistry;
	std::unique_ptr<ecs::Map> _entityMap;

	InfoConstants _infoConstants;
	Config _config;
	std::filesystem::path _startMap;

	std::chrono::steady_clock::time_point _lastGameLoopTime;
	std::chrono::steady_clock::duration _turnDeltaTime;
	float _gameSpeedMultiplier {1.0f};
	uint32_t _frameCount {0};
	uint16_t _turnCount {0};
	bool _paused {true};

	glm::ivec2 _mousePosition;
	glm::mat4 _handPose;

	entt::entity _handEntity;
	bool _handGripping;

	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> _requestScreenshot;
};
} // namespace openblack
