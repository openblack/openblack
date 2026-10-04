/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "TempleInterior.h"

#include <array>
#include <unordered_map>

#include <fmt/format.h>
#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "3D/CameraPath.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera/Camera.h"
#include "Camera/TempleCameraModel.h"
#include "Common/EventManager.h"
#include "ECS/Archetypes/GlowArchetype.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Temple.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Implementations/CameraPathSystem.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/RenderingSystemTemple.h"
#include "EngineConfig.h"
#include "FileSystem/FileSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;

using Indoors = TempleRoom;
const std::unordered_multimap<Indoors, std::string_view> k_TempleInteriorParts {
    {Indoors::Challenge, "challenge_l3d"},
    // {"challengelo_l3d", Indoors::ChallengeLO},
    {Indoors::Challenge, "challengedome_l3d"},
    {Indoors::Challenge, "challengefloor_l3d"},
    // {"challengefloorlo_l3d", Indoors::ChallengeFloorLO},
    {
        Indoors::CreatureCave,
        "creature_l3d",
    },
    // {"creaturelo_l3d", Indoors::CreatureCaveLO},
    {Indoors::CreatureCave, "creaturewater_l3d"},
    // {"creaturewaterlo_l3d", Indoors::CreatureCaveWaterLO},
    {Indoors::Credits, "credits_l3d"},
    // {"creditslo_l3d", Indoors::CreditsLO},
    {Indoors::Credits, "creditsdome_l3d"},
    {Indoors::Credits, "creditsfloor_l3d"},
    // {"creditsfloorlo_l3d", Indoors::CreditsFloorLO},
    {Indoors::Main, "main_l3d"},
    // {"mainlo_l3d", Indoors::MainLO},
    {Indoors::Main, "mainfloor_l3d"},
    // {"mainfloorlo_l3d", Indoors::MainFloorLO},
    {Indoors::Main, "mainwater_l3d"},
    // {"mainwaterlo_l3d", Indoors::MainWaterLO},
    // {"movement_l3d", Indoors::Movement}, // Navmesh
    {Indoors::Multi, "multi_l3d"},
    // {"multilo_l3d", Indoors::MultiLO},
    {Indoors::Multi, "multidome_l3d"},
    {Indoors::Multi, "multifloor_l3d"},
    // {"multifloorlo_l3d", Indoors::MultiFloorLO},
    {Indoors::Options, "options_l3d"},
    // {"optionslo_l3d", Indoors::OptionsLO},
    {Indoors::Options, "optionsdome_l3d"},
    {Indoors::Options, "optionsfloor_l3d"},
    // {"optionsfloorlo_l3d", Indoors::OptionsFloorLO},
    {Indoors::SaveGame, "savegame_l3d"},
    // {"savegamelo_l3d", Indoors::SaveGameLO},
    {Indoors::SaveGame, "savegamedome_l3d"},
    {Indoors::SaveGame, "savegamefloor_l3d"},
    // {"savegamefloorlo_l3d", Indoors::SaveGameFloorLO},
};

const std::unordered_map<Indoors, std::string_view> k_TempleInteriorGlows {
    {Indoors::Challenge, "challenge"},   //
    {Indoors::CreatureCave, "creature"}, //
    {Indoors::Credits, "credits"},       //
    {Indoors::Main, "main"},             //
    {Indoors::Multi, "multi"},           //
    {Indoors::Options, "options"},       //
    {Indoors::SaveGame, "savegame"},     //
};

inline void addRoomToRegistry(std::string_view assetName, Indoors templeRoom, glm::vec3 position, glm::mat3 rotation,
                              glm::vec3 scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto meshId = entt::hashed_string(fmt::format("temple/interior/{}", assetName).c_str());
	// "<room>_l3d" is the room itself and "<room>floor_l3d" its floor
	const auto roomName = k_TempleInteriorGlows.at(templeRoom);
	auto mesh = ecs::components::TempleInteriorMesh::Other;
	if (assetName == fmt::format("{}_l3d", roomName))
	{
		mesh = ecs::components::TempleInteriorMesh::Room;
	}
	else if (assetName == fmt::format("{}floor_l3d", roomName))
	{
		mesh = ecs::components::TempleInteriorMesh::Floor;
	}
	auto entity = registry.Create();
	registry.Assign<ecs::components::TempleInteriorPart>(entity, templeRoom, mesh);
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

namespace
{
void PlayDoorSound(entt::id_type sound)
{
	// GAudio plays the doors' sounds without a place
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(sound, std::nullopt);
	}
}
} // namespace

TempleInterior::TempleInterior()
    : _doors(PlayDoorSound)
{
}

TempleInterior::~TempleInterior() = default;

namespace
{
/// Each room's path in, data/citadel/engine/<room>.cam, which the game loads as "temple/<room>"
TempleCameraModel::Paths LoadCameraPaths()
{
	TempleCameraModel::Paths paths;
	auto& cameraPaths = Locator::resources::value().GetCameraPaths();
	for (const auto& [room, name] : k_TempleInteriorGlows)
	{
		const auto id = fmt::format("temple/{}", name);
		if (!cameraPaths.Contains(id))
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Temple camera path {} isn't loaded", id);
			continue;
		}
		paths.at(static_cast<size_t>(room)) = cameraPaths.Handle(entt::hashed_string(id.c_str()));
	}
	return paths;
}
} // namespace

void TempleInterior::ApplyLens() const
{
	const auto& config = Locator::config::value();
	auto& camera = Locator::camera::value();
	const auto aspect = Locator::windowing::has_value() ? Locator::windowing::value().GetAspectRatio() : 1.0f;
	const auto lens = camera.GetModel().GetLens();
	camera.SetProjectionMatrixPerspective(lens.has_value() ? lens->horizontalFieldOfView : config.cameraXFov, aspect,
	                                      lens.has_value() ? lens->nearClip : config.cameraNearClip, config.cameraFarClip);
}

void TempleInterior::GoToRoom(TempleRoom room)
{
	if (_active && _cameraModel != nullptr)
	{
		_cameraModel->GoToRoom(room);
	}
}

void TempleInterior::EnterRoom(TempleRoom room)
{
	if (_active && _cameraModel != nullptr && !_cameraModel->GoThroughDoorTo(room))
	{
		GoToRoom(room);
	}
}

std::optional<TempleCursorHit> TempleInterior::GetCursorHit() const
{
	if (!_active || _cameraModel == nullptr || !_cameraModel->GetCursorHit().has_value())
	{
		return std::nullopt;
	}
	const auto& hit = *_cameraModel->GetCursorHit();
	return TempleCursorHit {.point = hit.point, .normal = hit.normal};
}

void TempleInterior::Escape()
{
	if (_currentRoom != Indoors::Main)
	{
		GoToRoom(Indoors::Main);
	}
	else
	{
		RequestLeave();
	}
}

void TempleInterior::Update()
{
	if (_leaveRequested)
	{
		_leaveRequested = false;
		Deactivate();
	}
}

void TempleInterior::Activate(TempleRoom room)
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

	// The temple's camera takes over from the island's, coming into the room along its path
	_active = true;
	_leaveRequested = false;
	_transitionRoom.reset();
	_doors = TempleDoors(PlayDoorSound);
	_currentRoom = room;
	auto model = std::make_unique<TempleCameraModel>(LoadCameraPaths(), _currentRoom);
	_cameraModel = model.get();
	_outsideCameraModel = camera.SetModel(std::move(model));
	_cameraModel->StartIntro(_currentRoom, false);
	camera.SetOrigin(_cameraModel->GetTargetOrigin());
	camera.SetFocus(_cameraModel->GetTargetFocus());
	ApplyLens();
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
	if (_outsideCameraModel != nullptr)
	{
		camera.SetModel(std::move(_outsideCameraModel));
	}
	_cameraModel = nullptr;
	_transitionRoom.reset();
	ApplyLens();
	camera.SetOrigin(_playerPositionOutside);
	camera.SetFocus(_playerPositionOutside + glm::quat(_playerRotationOutside) * glm::vec3(0.0f, 0.0f, 1.0f));
	if (Locator::cameraPathSystem::value().IsPathing())
	{
		Locator::cameraPathSystem::value().Stop();
	}
	_active = false;
}