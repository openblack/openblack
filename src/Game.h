/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
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

#include <glm/mat4x4.hpp>
#include <spdlog/common.h>

#include "Common/Zoomer.h"
#include "EngineConfig.h"
#include "Windowing/WindowingInterface.h" // For DisplayMode

union SDL_Event;

namespace openblack
{

namespace audio
{
class AtmosAudio;
class GameMusic;
} // namespace audio
class HandAnimation;
namespace gui
{
class GameInterface;
}
namespace ecs::components
{
struct Transform;
}

enum class LoggingSubsystem : uint8_t
{
	game,
	input,
	graphics,
	scripting,
	audio,
	pathfinding,
	ai,

	_count
};

constexpr static std::array<std::string_view, static_cast<size_t>(LoggingSubsystem::_count)> k_LoggingSubsystemStrs {
    "game",        //
    "input",       //
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
	/// The game's graphics detail level, 0 to 6
	uint8_t detailLevel {4};
	openblack::windowing::DisplayMode displayMode;
	GraphicsBackend graphicsBackend;
	std::string gamePath;
	float guiScale;
	uint32_t numFramesToSimulate;
	std::string logFile;
	std::array<spdlog::level::level_enum, k_LoggingSubsystemStrs.size()> logLevels;
	std::string startLevel;
	/// Start on the flat creature testbed rather than startLevel
	bool startTestbed {false};
	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> requestScreenshot;
};

class Game
{
public:
	static constexpr auto k_TurnDuration = std::chrono::milliseconds(100);
	static constexpr float k_TurnDurationMultiplierSlow = 2.0f;
	static constexpr float k_TurnDurationMultiplierNormal = 1.0f;
	static constexpr float k_TurnDurationMultiplierFast = 0.5f;
	/// The height of the hand at standard size, which the game pulls the hovering hand back from the land by
	static constexpr float k_HandHeight = 3.2f;
	/// The game's limits on how far the hand is from the camera
	static constexpr float k_HandMinDistance = 2.0f;
	static constexpr float k_HandMaxDistance = 1800.0f;
	/// Inside the temple: how far short of the room the hand hangs, its limits on how far it is from the camera, and
	/// how long it takes to turn to the surface the cursor is on
	static constexpr float k_HandTempleGap = 3.25f;
	static constexpr float k_HandTempleMinDistance = 4.0f;
	static constexpr float k_HandTempleMaxDistance = 300.0f;
	static constexpr float k_HandTempleTurnTime = 0.4f;
	/// Land lower than this is the sea, which the hand rests on
	static constexpr float k_HandSeaAltitude = 0.1f;
	/// Seconds the hand eases over to land further from and nearer to the camera
	static constexpr float k_HandEaseOutTime = 0.28f;
	static constexpr float k_HandEaseInTime = 0.1f;
	/// Seconds the hand takes to settle onto the land it grips, the game's pose cross-fade
	static constexpr float k_HandGripSettleTime = 0.13f;

	explicit Game(Arguments&& args) noexcept;
	virtual ~Game() noexcept;

	bool ProcessEvents(const SDL_Event& event) noexcept;
	bool GameLogicLoop() noexcept;
	/// The game's music, for a turn of the world or of the temple
	void ProcessMusicTurn(glm::vec3 cameraPosition, bool inCitadel);
	/// The audio of a turn inside the temple, while the world is paused
	void ProcessTempleAudioTurn();
	/// The keys that take the player into the temple's rooms, from outside it or within
	void ProcessTempleRoomKeys();
	bool Update() noexcept;
	bool Initialize() noexcept;
	bool Run() noexcept;

	bool LoadMap(const std::filesystem::path& path) noexcept;
	void LoadLandscape(const std::filesystem::path& path);
	/// Loads the testbed: a flat plane over the whole map, with nothing on it, for trying out creatures
	void LoadTestbed() noexcept;

	void SetTime(float time) noexcept;
	/// How many times longer a turn takes: 2 is half speed
	void SetGameSpeed(float multiplier);
	[[nodiscard]] float GetGameSpeed() const;

	[[nodiscard]] uint32_t GetTurn() const;
	[[nodiscard]] bool IsPaused() const;
	[[nodiscard]] std::chrono::duration<float, std::milli> GetDeltaTime() const { return _turnDeltaTime; }
	[[nodiscard]] const glm::ivec2& GetMousePosition() const { return _mousePosition; }
	[[nodiscard]] const audio::AtmosAudio* GetAtmosAudio() const { return _atmosAudio.get(); }
	[[nodiscard]] audio::GameMusic* GetGameMusic() { return _gameMusic.get(); }
	[[nodiscard]] const audio::GameMusic* GetGameMusic() const { return _gameMusic.get(); }
	[[nodiscard]] const HandAnimation* GetHandAnimation() const { return _handAnimation.get(); }

	void RequestScreenshot(const std::filesystem::path& path) noexcept;

	static Game* Instance() { return sInstance; }

private:
	static Game* sInstance;

	/// Clears the last land's entities, weather and effects before a new one loads
	void PrepareNewLand();
	/// What every land starts with once its landscape is loaded: the sky's clock, a player and the land's physics
	void SetUpLandscape();
	/// Starts the game clock, the atmosphere's sounds and the music on the new land
	void StartNewLand();

	/// path to Lionhead Studios Ltd/Black & White folder
	const std::filesystem::path _gamePath;

	std::filesystem::path _startMap;
	bool _startTestbed {false};

	std::chrono::steady_clock::time_point _lastGameLoopTime;
	std::chrono::steady_clock::duration _turnDeltaTime;
	uint32_t _frameCount {0};
	glm::ivec2 _mousePosition;
	bool _handGripping;
	bool _handRotating {false};
	/// The hand sits on the line of sight through the cursor, this far from the camera
	glm::vec3 _handRayDirection {0.0f, -1.0f, 0.0f};
	float _handDistance {k_HandMinDistance};
	Zoomer _handHoverZoomer;
	bool _handWasDragging {false};
	/// The land the hand grips while it drags it, and where the hand was when it gripped
	glm::vec3 _handGripPoint {0.0f, 0.0f, 0.0f};
	glm::vec3 _handGripFrom {0.0f, 0.0f, 0.0f};
	Zoomer _handGripBlend;
	/// The way the surface the cursor is on in the temple faces, which the hand turns to
	Zoomer3 _handTempleNormal {glm::vec3(0.0f, 1.0f, 0.0f)};
	/// Where the hand holds on while the camera turns
	glm::vec3 _handHoldPoint {0.0f, 0.0f, 0.0f};
	bool _handWasRotating {false};
	/// Where the cursor points at in the world, on the landscape or the sea
	std::optional<glm::vec3> _cursorWorldPosition;
	/// Grabbing the sea plays G_HandInWater_01 to _10 in turn
	uint32_t _handInWaterSample {0};

	/// Plays the sound of the hand grabbing the land or the sea at the grab point, as the game does
	void PlayHandGrabSound();
	/// Places the hand on the line of sight through the cursor the way the game does
	void PlaceHand(ecs::components::Transform& handTransform, float deltaSeconds);
	/// Loads the hand animations of Data/CTR/hh.hbn for the hand mesh
	void LoadHandAnimation();
	/// What moves each species' body, from Data/CTR's .cbn files, by the species their base mesh names
	static void LoadCreatureRigs();
	/// Acts on what the player chose in the game's menu: continuing restores the pause it had before it opened
	void HandleInterfaceAction();

	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> _requestScreenshot;
	std::unique_ptr<audio::AtmosAudio> _atmosAudio;
	std::unique_ptr<audio::GameMusic> _gameMusic;
	std::unique_ptr<HandAnimation> _handAnimation;
	/// The game's own interface, null without the game's files for it
	std::unique_ptr<gui::GameInterface> _interface;
	/// Whether the game was paused when the menu opened, which pauses it
	bool _pausedBeforeMenu {true};
	bool _menuWasOpen {false};
};
} // namespace openblack
