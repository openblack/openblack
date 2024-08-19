/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <map>
#include <vector>

#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>

#include "Window.h"

namespace openblack::debug::gui
{

class Camera final: public Window
{
public:
	Camera();

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

private:
	void DrawControls();
	void DrawCameraResourceList();
	entt::id_type _selectedCameraPath;
	std::map<entt::id_type, std::tuple<bool, entt::entity>> _drawingCameraPaths;
	std::map<entt::id_type, std::vector<entt::entity>> _cameraPathEntities;
};

} // namespace openblack::debug::gui
