/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <optional>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

#include "Window.h"

namespace openblack::debug::gui
{

class PathFinding: public Window
{
public:
	PathFinding();

protected:
	void Draw(Game& game) override;
	void Update(Game& game, const Renderer& renderer) override;
	void ProcessEventOpen(const SDL_Event& event) override;
	void ProcessEventAlways(const SDL_Event& event) override;

private:
	enum class HandTo
	{
		None,
		Entity,
		Destination,
	};

	HandTo _handTo {HandTo::None};
	glm::vec3 _handPosition {0.0f, 0.0f, 0.0f};
	glm::vec3 _destination {0.0f, 0.0f, 0.0f};
	std::optional<entt::entity> _selectedVillager;
	std::optional<entt::entity> _selectedFootpath;
};

} // namespace openblack::debug::gui
