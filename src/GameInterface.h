/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <filesystem>

namespace openblack
{
class GameInterface
{
public:
	virtual ~GameInterface() = default;

	virtual bool LoadMap(const std::filesystem::path& path) noexcept = 0;
	virtual void LoadLandscape(const std::filesystem::path& path) = 0;
	virtual void SetTime(float time) noexcept = 0;
	virtual void SetGameSpeed(float multiplier) = 0;
	virtual float GetGameSpeed() const = 0;
	virtual uint32_t GetTurn() const = 0;
	virtual bool IsPaused() const = 0;
	virtual std::chrono::duration<float, std::milli> GetDeltaTime() const = 0;
	virtual void RequestScreenshot(const std::filesystem::path& path) noexcept = 0;
};
} // namespace openblack
