/******************************************************************************
 * Copyright (c) 2018-2024 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TempleInterior.h"

#include <unordered_map>

#include <glm/gtx/euler_angles.hpp>

#include "Camera/Camera.h"
#include "Common/EventManager.h"
#include "ECS/Archetypes/GlowArchetype.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Temple.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/RenderingSystemTemple.h"
#include "EngineConfig.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;

using Indoors = ecs::components::TempleRoom;
const std::unordered_multimap<Indoors, std::string_view> k_TempleInteriorParts {
    {Indoors::ChallengeRoom, "challenge_l3d"},
    // {"challengelo_l3d", Indoors::ChallengeLO},
    {Indoors::ChallengeRoom, "challengedome_l3d"},
    {Indoors::ChallengeRoom, "challengefloor_l3d"},
    // {"challengefloorlo_l3d", Indoors::ChallengeFloorLO},
    {
        Indoors::CreatureCave,
        "creature_l3d",
    },
    // {"creaturelo_l3d", Indoors::CreatureCaveLO},
    {Indoors::CreatureCave, "creaturewater_l3d"},
    // {"creaturewaterlo_l3d", Indoors::CreatureCaveWaterLO},
    {Indoors::CreditsRoom, "credits_l3d"},
    // {"creditslo_l3d", Indoors::CreditsLO},
    {Indoors::CreditsRoom, "creditsdome_l3d"},
    {Indoors::CreditsRoom, "creditsfloor_l3d"},
    // {"creditsfloorlo_l3d", Indoors::CreditsFloorLO},
    {Indoors::MainRoom, "main_l3d"},
    // {"mainlo_l3d", Indoors::MainLO},
    {Indoors::MainRoom, "mainfloor_l3d"},
    // {"mainfloorlo_l3d", Indoors::MainFloorLO},
    {Indoors::MainRoom, "mainwater_l3d"},
    // {"mainwaterlo_l3d", Indoors::MainWaterLO},
    // {"movement_l3d", Indoors::Movement}, // Navmesh
    {Indoors::MultiplayerRoom, "multi_l3d"},
    // {"multilo_l3d", Indoors::MultiLO},
    {Indoors::MultiplayerRoom, "multidome_l3d"},
    {Indoors::MultiplayerRoom, "multifloor_l3d"},
    // {"multifloorlo_l3d", Indoors::MultiFloorLO},
    {Indoors::OptionsRoom, "options_l3d"},
    // {"optionslo_l3d", Indoors::OptionsLO},
    {Indoors::OptionsRoom, "optionsdome_l3d"},
    {Indoors::OptionsRoom, "optionsfloor_l3d"},
    // {"optionsfloorlo_l3d", Indoors::OptionsFloorLO},
    {Indoors::SaveGameRoom, "savegame_l3d"},
    // {"savegamelo_l3d", Indoors::SaveGameLO},
    {Indoors::SaveGameRoom, "savegamedome_l3d"},
    {Indoors::SaveGameRoom, "savegamefloor_l3d"},
    // {"savegamefloorlo_l3d", Indoors::SaveGameFloorLO},
};

const std::unordered_map<Indoors, std::string_view> k_TempleInteriorGlows {
    {Indoors::ChallengeRoom, "challenge"}, //
    {Indoors::CreatureCave, "creature"},   //
    {Indoors::CreditsRoom, "credits"},     //
    {Indoors::MainRoom, "main"},           //
    {Indoors::MultiplayerRoom, "multi"},   //
    {Indoors::OptionsRoom, "options"},     //
    {Indoors::SaveGameRoom, "savegame"},   //
};

inline void addRoomToRegistry(std::string_view assetName, Indoors templeRoom, glm::vec3 position, glm::mat3 rotation,
                              glm::vec3 scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto meshId = entt::hashed_string(fmt::format("temple/interior/{}", assetName).c_str());
	auto entity = registry.Create();
	registry.Assign<ecs::components::TempleInteriorPart>(entity, templeRoom);
	registry.Assign<ecs::components::Transform>(entity, position, rotation, scale);
	registry.Assign<ecs::components::Mesh>(entity, meshId, static_cast<int8_t>(0), static_cast<int8_t>(0));
}

inline void addGlowsToRegistry(Indoors templeRoom)
{
	const auto& glowManager = Locator::resources::value().GetGlows();
	const auto glowId =
	    entt::hashed_string(fmt::format("temple/interior/glow/{}", k_TempleInteriorGlows.at(templeRoom)).c_str());
	const auto glows = glowManager.Handle(glowId);
	for (const auto& glow : glows->emitters)
	{
		ecs::archetypes::GlowArchetype::Create(glow, templeRoom);
	}
}

void TempleInterior::Activate()
{
	if (_active)
	{
		return;
	}

	auto& config = Locator::config::value();
	auto& camera = Locator::camera::value();

	_playerPositionOutside = camera.GetOrigin();
	_playerRotationOutside = camera.GetRotation();

	config.drawIsland = false;
	config.drawWater = false;

	// Create temple entities
	auto rotation = glm::eulerAngleY(_templeRotation.y);
	auto scale = glm::vec3(1.0f);

	for (const auto& [roomType, assetName] : k_TempleInteriorParts)
	{
		addRoomToRegistry(assetName, roomType, _templePosition, rotation, scale);
	}
	for (const auto& [roomType, assetName] : k_TempleInteriorGlows)
	{
		addGlowsToRegistry(roomType);
	}

	Locator::rendereringSystem::emplace<ecs::systems::RenderingSystemTemple>();
	camera.SetOrigin(_templePosition);
	camera.SetFocus(_templePosition + glm::quat(_templeRotation) * glm::vec3(0.0f, 0.0f, 1.0f));
	_active = true;
}

void TempleInterior::Deactivate()
{
	if (!_active)
	{
		return;
	}

	auto& registry = Locator::entitiesRegistry::value();
	auto& config = Locator::config::value();
	config.drawIsland = true;
	config.drawWater = true;
	registry.Each<const ecs::components::TempleInteriorPart>(
	    [&registry](const entt::entity entity, auto&&...) { registry.Destroy(entity); });

	auto& camera = Locator::camera::value();
	Locator::rendereringSystem::emplace<ecs::systems::RenderingSystem>();
	camera.SetOrigin(_playerPositionOutside);
	camera.SetFocus(_playerPositionOutside + glm::quat(_playerRotationOutside) * glm::vec3(0.0f, 0.0f, 1.0f));
	_active = false;
}
