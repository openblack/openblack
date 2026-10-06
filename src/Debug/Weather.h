/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include "ECS/Systems/WeatherSystemInterface.h"
#include "Window.h"

namespace openblack::debug::gui
{

/// Forces the weather over the whole island: rain, snow and lightning storms from presets or made to measure, with the
/// weather at the camera and the storms alive
class Weather final: public Window
{
public:
	Weather() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	void DrawAtCamera() noexcept;
	void DrawPresets() noexcept;
	void DrawCustom() noexcept;
	void DrawActions() noexcept;
	void DrawStorms() noexcept;

	/// The storm being made to measure
	ecs::systems::ForcedStorm _storm;
	/// Its wind, as a direction in degrees and a strength
	float _windDegrees {0.0f};
	int _windStrength {0};
	bool _lightning {false};
};

} // namespace openblack::debug::gui
