/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
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

#include "EngineConfig.h"
#include "InfoConstants.h"
#include "Windowing/WindowingInterface.h" // For DisplayMode

union SDL_Event;

namespace openblack
{
class Profiler;
class L3DAnim;
class L3DMesh;
class Sky;
class Water;

namespace graphics
{
class Renderer;
class Texture2D;
} // namespace graphics

namespace lhvm
{
class LHVM;
} // namespace lhvm

namespace lhscriptx
{
class Script;
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
	openblack::windowing::DisplayMode displayMode;
	bgfx::RendererType::Enum rendererType;
	std::string gamePath;
	float guiScale;
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

	explicit Game(Arguments&& args) noexcept;
	virtual ~Game() noexcept;

	bool ProcessEvents(const SDL_Event& event) noexcept;
	bool GameLogicLoop() noexcept;
	bool Update() noexcept;
	bool Initialize() noexcept;
	bool Run() noexcept;

	bool LoadMap(const std::filesystem::path& path) noexcept;
	void LoadLandscape(const std::filesystem::path& path);

	void SetTime(float time) noexcept;
	void SetGameSpeed(float multiplier) { _gameSpeedMultiplier = multiplier; }
	[[nodiscard]] float GetGameSpeed() const { return _gameSpeedMultiplier; }

	[[nodiscard]] Profiler& GetProfiler() const { return *_profiler; }
	[[nodiscard]] Sky& GetSky() const { return *_sky; }
	[[nodiscard]] Water& GetWater() const { return *_water; }
	[[nodiscard]] entt::entity GetHand() const;
	[[nodiscard]] const lhvm::LHVM& GetLhvm() const { return *_lhvm; }
	lhvm::LHVM& GetLhvm() { return *_lhvm; }
	[[nodiscard]] uint16_t GetTurn() const { return _turnCount; }
	[[nodiscard]] bool IsPaused() const { return _paused; }
	[[nodiscard]] std::chrono::duration<float, std::milli> GetDeltaTime() const { return _turnDeltaTime; }
	[[nodiscard]] const glm::ivec2& GetMousePosition() const { return _mousePosition; }

	void RequestScreenshot(const std::filesystem::path& path) noexcept;

	static Game* Instance() { return sInstance; }

private:
	static Game* sInstance;

	/// path to Lionhead Studios Ltd/Black & White folder
	const std::filesystem::path _gamePath;

	std::unique_ptr<Profiler> _profiler;

	// std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DMesh> _testModel;
	std::unique_ptr<L3DAnim> _testAnimation;
	std::unique_ptr<L3DMesh> _handModel;
	std::unique_ptr<Sky> _sky;
	std::unique_ptr<Water> _water;
	std::unique_ptr<lhscriptx::Script> _scriptx;
	std::unique_ptr<lhvm::LHVM> _lhvm;

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
