/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "Gui/CinemaBars.h"
#include "Gui/ScriptFade.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "ECS System implementations should only be included in Locator.cpp"
#endif

namespace openblack::ecs::systems
{

class CinematicDirectorSystem final: public CinematicDirectorSystemInterface
{
public:
	void FadeTo(uint8_t red, uint8_t green, uint8_t blue, int8_t seconds) override;
	void FadeBackToNormal(int8_t seconds) override;
	[[nodiscard]] bool IsFadeFinished() const override;
	[[nodiscard]] uint32_t GetFadeColour() const override;

	void SetWideScreen(bool on, uint32_t owner) override;
	[[nodiscard]] bool IsWideScreenOn() const override;
	[[nodiscard]] uint32_t GetWideScreenOwner() const override;
	[[nodiscard]] bool IsWideScreenTransitionFinished() const override;
	[[nodiscard]] float GetWideScreenFraction() const override;
	[[nodiscard]] bool IsInterfaceActive() const override;
	[[nodiscard]] bool TakeHideDialogs() override;

	void Reset() override { *this = CinematicDirectorSystem {}; }
	void ProcessTurn() override;
	void Update(std::chrono::duration<float, std::milli> gameTime) override;

	void SetCloseClipping(bool close) override { _closeClipping = close; }
	[[nodiscard]] bool IsCloseClipping() const override { return _closeClipping; }

private:
	gui::ScriptFade _fade;
	gui::CinemaBars _bars;
	uint32_t _wideScreenOwner {0};
	bool _interfaceActive {true};
	bool _hideDialogs {false};
	bool _closeClipping {false};
};

} // namespace openblack::ecs::systems
