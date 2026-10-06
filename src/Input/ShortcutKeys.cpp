/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ShortcutKeys.h"

#include <chrono>
#include <optional>
#include <stdexcept>

#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "3D/TempleInteriorInterface.h"
#include "Camera/Camera.h"
#include "Camera/CameraModel.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CinematicDirectorSystemInterface.h"
#include "EngineConfig.h"
#include "GameActionMapInterface.h"
#include "Locator.h"

namespace openblack::input
{
namespace
{
/// The ground under a point
[[nodiscard]] glm::vec3 GroundUnder(const LandIslandInterface& land, glm::vec3 point)
{
	return {point.x, land.GetHeightAt({point.x, point.z}), point.z};
}

/// Where the player's temple stands, if they have one
[[nodiscard]] std::optional<glm::vec3> PlayerTemplePosition()
{
	std::optional<glm::vec3> position;
	Locator::entitiesRegistry::value().Each<const ecs::components::Temple, const ecs::components::Transform>(
	    [&position](const ecs::components::Temple& temple, const ecs::components::Transform& transform) {
		    if (temple.owner == PlayerNames::PLAYER_ONE)
		    {
			    position = transform.position;
		    }
	    });
	return position;
}
} // namespace

void ShortcutKeys::Update()
{
	if (!Locator::gameActionSystem::has_value())
	{
		return;
	}
	const auto& actions = Locator::gameActionSystem::value();
	const auto pressed = [&actions](BindableActionMap action) { return actions.GetChanged(action) && actions.Get(action); };

	// Pressing the key of an action whose feature isn't built yet says so, so the press is seen to be taken
	for (const auto& binding : actions.GetKeyBindings())
	{
		if (binding.status == BindStatus::NotYetImplemented && pressed(binding.action))
		{
			SPDLOG_LOGGER_INFO(spdlog::get("input"), "{} ({}) is not implemented yet", binding.name,
			                   binding.key.has_value() ? KeyChordName(*binding.key)
			                                           : std::string(MouseInputName(binding.mouse)));
		}
	}

	// TODO(raffclar): Zoom To Creature and the creature's room key belong to the creature mode work, and the leash keys
	// to the leash work, which read them themselves.

	// The villagers' names and details are toggled, as the game does
	auto& config = Locator::config::value();
	if (pressed(BindableActionMap::SHOW_VILLAGER_NAMES))
	{
		config.showVillagerNames = !config.showVillagerNames;
	}
	if (pressed(BindableActionMap::SHOW_VILLAGER_DETAILS))
	{
		config.showVillagerDetails = !config.showVillagerDetails;
	}

	const bool temple = pressed(BindableActionMap::ZOOM_TO_TEMPLE);
	const bool realm = pressed(BindableActionMap::ZOOM_TO_REALM);
	if (!temple && !realm)
	{
		return;
	}
	// The camera isn't the player's while a script has the cinema bars in, nor inside the temple
	if (!Locator::cinematicDirectorSystem::value().IsInterfaceActive() ||
	    (Locator::temple::has_value() && Locator::temple::value().Active()))
	{
		return;
	}
	auto& camera = Locator::camera::value();
	const auto& land = Locator::terrainSystem::value();
	glm::vec3 realmGround;
	try
	{
		// Over the middle of the island's land
		const auto extent = land.GetExtent();
		realmGround = GroundUnder(
		    land, glm::vec3((extent.minimum.x + extent.maximum.x) * 0.5f, 0.0f, (extent.minimum.y + extent.maximum.y) * 0.5f));
	}
	catch (const std::runtime_error&)
	{
		// No land is loaded
		return;
	}
	const zoom_to::CameraView current {.origin = camera.GetOrigin(), .focus = camera.GetFocus()};
	std::optional<zoom_to::CameraView> flight;
	if (temple)
	{
		const auto now =
		    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch());
		const auto templePosition = PlayerTemplePosition();
		flight = _zoomTo.PressTemple(
		    now, current, GroundUnder(land, current.focus),
		    templePosition.has_value() ? std::optional(GroundUnder(land, *templePosition)) : std::nullopt, realmGround);
	}
	else
	{
		flight = _zoomTo.PressRealm(current, realmGround);
	}
	if (flight.has_value())
	{
		camera.GetModel().SetFlight(flight->origin, flight->focus);
	}
}

} // namespace openblack::input
