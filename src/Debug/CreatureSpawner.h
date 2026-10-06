/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <random>

#include <entt/entity/fwd.hpp>
#include <glm/vec2.hpp>

#include "Enums.h"
#include "Window.h"

namespace openblack::debug::gui
{

/// Spawns creatures for trying them out: pick the species, owner, alignment, physique and size, then right click on
/// the land to place one there, until placing is stopped. Lists the creatures on the land, to remove them again.
class CreatureSpawner final: public Window
{
public:
	CreatureSpawner() noexcept;

	void Close() noexcept override;

protected:
	void Draw() noexcept override;
	void Update() noexcept override;
	void ProcessEventOpen(const SDL_Event& event) noexcept override;
	void ProcessEventAlways(const SDL_Event& event) noexcept override;
	[[nodiscard]] bool TakesEvent(const SDL_Event& event) const noexcept override;

private:
	void DrawSettings() noexcept;
	/// The creature picked in the list: its alignment, physique and size, changed as it stands
	void DrawSelected() noexcept;
	/// The picked creature's mind: what it is doing, wants and looks at, and buttons to make it do things
	void DrawMind(entt::entity entity) noexcept;
	void DrawPlacing() noexcept;
	void DrawCreatures() noexcept;
	void Spawn(glm::vec2 screenCoord) noexcept;
	/// Starts the body as a new creature of the species is: its size, fatness and strength
	void UseSpeciesDefaults() noexcept;

	CreatureType _species {CreatureType::Tiger};
	PlayerNames _owner {PlayerNames::PLAYER_ONE};
	float _alignment {0.0f};
	float _fatness {0.5f};
	float _strength {0.5f};
	float _scale {1.0f};
	/// The species the body was last started for, once the game's creature tables are loaded
	std::optional<CreatureType> _defaultsFor;
	float _facingDegrees {180.0f};
	bool _randomFacing {false};

	bool _placing {false};
	/// Where on the screen, 0 to 1, the land was right clicked to place a creature, until it is placed
	std::optional<glm::vec2> _placeAt;
	std::mt19937 _random {std::random_device {}()};
	std::optional<entt::entity> _selected;
	/// The action and gesture picked to play on the selected creature
	size_t _action {0};
	size_t _gesture {0};
};

} // namespace openblack::debug::gui
