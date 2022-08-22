/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <map>
#include <vector>

#include <entt/core/fwd.hpp>
#include <entt/entity/entity.hpp>

#include "Window.h"

namespace openblack::debug::gui
{

class Camera: public Window
{
public:
	Camera();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	void DrawControls();
	void DrawInstanceList();
	void DrawCameraResourceList();
	entt::id_type _selectedCameraPath;
	entt::entity _selectedCameraPathInstance;
	std::map<entt::id_type, std::tuple<bool, entt::entity>> _drawingCameraPaths;
	std::map<entt::id_type, std::vector<entt::entity>> _cameraPathEntities;
	bool _running = false;
};

} // namespace openblack::debug::gui
