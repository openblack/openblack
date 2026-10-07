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

#include "Window.h"

namespace openblack::debug::gui
{

/// Puts things into the physics to watch them fly, collide and come to rest
class Physics final: public Window
{
public:
	Physics() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	/// Which rock (or toy) to drop, as a static object's type
	int _type {36};
	float _height {20.0f};
	float _scale {1.0f};
	std::array<float, 3> _velocity {0.0f, 0.0f, 0.0f};
	/// An object already in the world to throw instead
	int _entity {-1};
};

} // namespace openblack::debug::gui
