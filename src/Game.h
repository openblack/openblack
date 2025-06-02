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

#include <bgfx/bgfx.h>
#include <glm/mat4x4.hpp>
#include <spdlog/common.h>

#include "ECS/Components/PlayerHand.h"
#include "GameInterface.h"
#include "Windowing/WindowingInterface.h" // For DisplayMode

union SDL_Event;

namespace openblack
{

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

class Game: public GameInterface
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

	bool LoadMap(const std::filesystem::path& path) noexcept override;
	void LoadLandscape(const std::filesystem::path& path) override;

	void SetTime(float time) noexcept override;
	void SetGameSpeed(float multiplier) override { _gameSpeedMultiplier = multiplier; }
	[[nodiscard]] float GetGameSpeed() const override { return _gameSpeedMultiplier; }

	[[nodiscard]] uint32_t GetTurn() const override { return _turnCount; }
	[[nodiscard]] bool IsPaused() const override { return _paused; }
	[[nodiscard]] std::chrono::duration<float, std::milli> GetDeltaTime() const override { return _turnDeltaTime; }

	void RequestScreenshot(const std::filesystem::path& path) noexcept override;

protected:
	[[nodiscard]] const glm::ivec2& GetMousePosition() const;
	[[nodiscard]] const ecs::components::PlayerHand& GetPlayerHand() const;

private:
	glm::ivec2& GetMousePosition();
	ecs::components::PlayerHand& GetPlayerHand();

	/// path to Lionhead Studios Ltd/Black & White folder
	const std::filesystem::path _gamePath;

	std::filesystem::path _startMap;

	std::chrono::steady_clock::time_point _lastGameLoopTime;
	std::chrono::steady_clock::duration _turnDeltaTime;
	float _gameSpeedMultiplier {1.0f};
	uint32_t _frameCount {0};
	uint32_t _turnCount {0};
	bool _paused {true};

	std::optional<std::pair</* frame number */ uint32_t, /* output */ std::filesystem::path>> _requestScreenshot;
};
} // namespace openblack
