/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TempleInterior.h"

#include <glm/gtx/euler_angles.hpp>

#include "3D/Camera.h"
#include "Common/EventManager.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Registry.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace ecs::components;

using Indoors = TempleInterior::TempleIndoorPart;
const std::map<std::string_view, Indoors> k_TempleInteriorParts {
    {"challenge_l3d", Indoors::Challenge},
    // {"challengelo_l3d", Indoors::ChallengeLO},
    {"challengedome_l3d", Indoors::ChallengeDome},
    {"challengefloor_l3d", Indoors::ChallengeFloor},
    // {"challengefloorlo_l3d", Indoors::ChallengeFloorLO},
    {"creature_l3d", Indoors::CreatureCave},
    // {"creaturelo_l3d", Indoors::CreatureCaveLO},
    {"creaturewater_l3d", Indoors::CreatureCaveWater},
    // {"creaturewaterlo_l3d", Indoors::CreatureCaveWaterLO},
    {"credits_l3d", Indoors::Credits},
    // {"creditslo_l3d", Indoors::CreditsLO},
    {"creditsdome_l3d", Indoors::CreditsDome},
    {"creditsfloor_l3d", Indoors::CreditsFloor},
    // {"creditsfloorlo_l3d", Indoors::CreditsFloorLO},
    {"main_l3d", Indoors::Main},
    // {"mainlo_l3d", Indoors::MainLO},
    {"mainfloor_l3d", Indoors::MainFloor},
    // {"mainfloorlo_l3d", Indoors::MainFloorLO},
    {"mainwater_l3d", Indoors::MainWater},
    // {"mainwaterlo_l3d", Indoors::MainWaterLO},
    // {"movement_l3d", Indoors::Movement}, // Navmesh
    {"multi_l3d", Indoors::Multi},
    // {"multilo_l3d", Indoors::MultiLO},
    {"multidome_l3d", Indoors::MultiDome},
    {"multifloor_l3d", Indoors::MultiFloor},
    // {"multifloorlo_l3d", Indoors::MultiFloorLO},
    {"options_l3d", Indoors::Options},
    // {"optionslo_l3d", Indoors::OptionsLO},
    {"optionsdome_l3d", Indoors::OptionsDome},
    {"optionsfloor_l3d", Indoors::OptionsFloor},
    // {"optionsfloorlo_l3d", Indoors::OptionsFloorLO},
    {"savegame_l3d", Indoors::SaveGame},
    // {"savegamelo_l3d", Indoors::SaveGameLO},
    {"savegamedome_l3d", Indoors::SaveGameDome},
    {"savegamefloor_l3d", Indoors::SaveGameFloor},
    // {"savegamefloorlo_l3d", Indoors::SaveGameFloorLO},
};

void TempleInterior::Activate()
{
	if (_active)
	{
		return;
	}

	auto& registry = Locator::entitiesRegistry::value();
	auto& config = Game::Instance()->GetConfig();
	auto& camera = Game::Instance()->GetCamera();

	_playerPositionOutside = camera.GetPosition();
	_playerRotationOutside = camera.GetRotation();

	config.drawIsland = false;
	config.drawWater = false;

	// Create temple entities
	auto rotation = glm::eulerAngleY(_templeRotation.y);
	auto scale = glm::vec3(1.0f);

	for (const auto& [assetName, indoorPartType] : k_TempleInteriorParts)
	{
		auto meshId = entt::hashed_string(fmt::format("temple/interior/{}", assetName).c_str());
		auto entity = registry.Create();
		auto roomType = ecs::components::TempleRoom::MainRoom;
		registry.Assign<ecs::components::TempleInteriorPart>(entity, roomType);
		registry.Assign<ecs::components::Transform>(entity, _templePosition, rotation, scale);
		registry.Assign<ecs::components::Mesh>(entity, meshId, static_cast<int8_t>(0), static_cast<int8_t>(0));
	}

	camera.SetPosition(_templePosition + glm::vec3(0.0f, 1.0f, 0.0f));
	camera.SetRotation(_templeRotation);
	_active = true;
}

void TempleInterior::Deactivate()
{
	if (!_active)
	{
		return;
	}

	auto& registry = Locator::entitiesRegistry::value();
	auto& config = Game::Instance()->GetConfig();
	config.drawIsland = true;
	config.drawWater = true;
	registry.Each<const TempleInteriorPart>([&registry](const entt::entity entity, auto&&...) { registry.Destroy(entity); });

	auto& camera = Game::Instance()->GetCamera();
	camera.SetPosition(_playerPositionOutside);
	camera.SetRotation(_playerRotationOutside);
	_active = false;
}
