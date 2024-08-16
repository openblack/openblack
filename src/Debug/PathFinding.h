/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

#include "Window.h"

namespace openblack::debug::gui
{

class PathFinding final: public Window
{
public:
	PathFinding() noexcept;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;

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
