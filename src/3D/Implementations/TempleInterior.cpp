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

#include <cstdio>

#include <array>
#include <unordered_map>

#include <fmt/format.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "3D/CameraPath.h"
#include "3D/CreatureCaveEffects.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/TempleScrolls.h"
#include "3D/TempleSigns.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Camera/TempleCameraModel.h"
#include "Common/EventManager.h"
#include "ECS/Archetypes/GlowArchetype.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/Implementations/CameraPathSystem.h"
#include "ECS/Systems/Implementations/RenderingSystem.h"
#include "ECS/Systems/Implementations/RenderingSystemTemple.h"
#include "EngineConfig.h"
#include "FileSystem/FileSystemInterface.h"
#include "Gui/GameInterface.h"
#include "Gui/GameMenu.h"
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
	else if (assetName == fmt::format("{}water_l3d", roomName) && templeRoom == Indoors::CreatureCave)
	{
		mesh = ecs::components::TempleInteriorMesh::Water;
	}
	else if (assetName == fmt::format("{}water_l3d", roomName) && templeRoom == Indoors::Main)
	{
		mesh = ecs::components::TempleInteriorMesh::Pool;
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
/// How far through its swing the main room's door is before DrawDoors draws the room behind it
constexpr float k_MainRoomOpenSwing = 0.01f;

/// CreatureRoom::Draw slides the waterfall's texture through this much of a slide each millisecond, and the slide
/// across ten of the texture
constexpr float k_WaterfallSlidePerMillisecond = 2.1e-5f;
/// GGame::Loop has the temple's game turn every 100 milliseconds while the player is inside it
constexpr float k_ToolTipTurnMilliseconds = 100.0f;
constexpr float k_WaterfallSlideLength = -10.0f;
/// CreatureRoom::DrawAdditional's sounds: the water at the waterfall's foot, and the fire where the creature stands, the
/// second place movement.l3d marks
constexpr glm::vec3 k_CreatureCaveWaterSound {160.0f, -45.0f, -30.0f};
constexpr size_t k_CreatureCaveFirePlace = 1;

/// The fire of the creature's room, the second place movement.l3d marks (CPController::Init)
std::optional<glm::vec3> CreatureCaveFire()
{
	const entt::id_type movement = entt::hashed_string("temple/interior/movement_l3d").value();
	auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(movement) || meshes.Handle(movement)->GetExtraMetrics().size() <= k_CreatureCaveFirePlace)
	{
		return std::nullopt;
	}
	return glm::vec3(meshes.Handle(movement)->GetExtraMetrics()[k_CreatureCaveFirePlace][3]);
}

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
    , _toggles(PlayDoorSound)
{
}

namespace
{
/// The temple is made as the game starts, which SaveGameRoom's time played counts from
const auto k_GameStarted = std::chrono::steady_clock::now();

/// What the rooms' scrolls tell of the game: made up, but for what openblack keeps
TempleScrolls::Facts GatherScrollFacts()
{
	auto facts = TempleScrolls::Facts::Mock();
	// GGame's count of the people in the world
	facts.population = static_cast<int32_t>(Locator::entitiesRegistry::value().Size<ecs::components::Villager>());
	facts.timePlayed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - k_GameStarted);
	return facts;
}
} // namespace

void TempleInterior::SetInterface(gui::GameInterface* interface)
{
	_interface = interface;
	_scrolls.reset();
	_signs.reset();
	_text.clear();
	if (_active)
	{
		CreateScrolls();
	}
}

void TempleInterior::CreateScrolls()
{
	if (_interface == nullptr)
	{
		return;
	}
	// PictureRoomBase::InitEngine reads the parchment every scroll is written on
	auto& fileSystem = Locator::filesystem::value();
	const auto path = fileSystem.GetPath<filesystem::Path::Textures>() / "ChallengeScroll.raw";
	std::vector<uint8_t> parchment;
	if (fileSystem.Exists(path))
	{
		parchment = fileSystem.ReadAll(path);
	}
	_scrolls = std::make_unique<TempleScrolls>(_interface->GetTexts(), _interface->GetFont(), std::move(parchment));
	_scrolls->Create(GatherScrollFacts());
	_signs = std::make_unique<TempleSigns>(_interface->GetTexts(), _interface->GetFont());

	// WorldRoom::InitEngine finds the buttons of what the map shows
	auto& meshes = Locator::resources::value().GetMeshes();
	const entt::id_type mainMeshId = entt::hashed_string("temple/interior/main_l3d").value();
	std::vector<std::string> names;
	if (meshes.Contains(mainMeshId))
	{
		for (const auto& subMesh : meshes.Handle(mainMeshId)->GetSubMeshes())
		{
			names.push_back(subMesh->GetName());
		}
	}
	_toggles.Find(names);
}

bool TempleInterior::HoldControl(bool pressed, float mouseY)
{
	const auto hit = GetCursorHit();
	const auto hoveredMainSubMesh =
	    hit.has_value() && hit->room == TempleRoom::Main ? hit->subMesh : std::optional<uint32_t> {};
	bool held = _toggles.Hold(pressed, hoveredMainSubMesh);
	if (_scrolls != nullptr)
	{
		std::optional<TempleScrolls::Focus> focus;
		held = _scrolls->Hold(pressed, mouseY, hit, GatherScrollFacts(), focus) || held;
		if (focus.has_value() && _cameraModel != nullptr)
		{
			_cameraModel->LookAtSubMesh(focus->position, focus->lookAt);
		}
	}
	return held;
}

std::vector<uint32_t> TempleInterior::GetHiddenSubMeshes(TempleRoom room) const
{
	return room == TempleRoom::Main ? _toggles.GetHidden() : std::vector<uint32_t> {};
}

bool TempleInterior::IsControl(TempleRoom room, uint32_t subMesh) const
{
	return (room == TempleRoom::Main && _toggles.IsControl(subMesh)) ||
	       (_scrolls != nullptr && _scrolls->IsControl(room, subMesh));
}

bool TempleInterior::IsControlHeld() const
{
	return _toggles.IsHeld() || (_scrolls != nullptr && _scrolls->IsHeld());
}

std::vector<TempleSubMeshTexture> TempleInterior::GetScrollTextures(TempleRoom room) const
{
	return _scrolls != nullptr ? _scrolls->GetTextures(room) : std::vector<TempleSubMeshTexture> {};
}

std::vector<TempleSubMeshGlow> TempleInterior::GetControlGlows(TempleRoom room) const
{
	// SubOptionEntry::GetSubMeshData: the control under the cursor brightens by up to 12, 12 and 20 of 255 as the glow
	// runs out
	if (!_hovered.has_value() || _hovered->first != room || !IsControl(room, _hovered->second) || _hoverGlow <= 0.0f)
	{
		return {};
	}
	const auto channel = [this](float strength) {
		return static_cast<float>(static_cast<int32_t>(_hoverGlow * strength * 0.002f)) / 255.0f;
	};
	return {{_hovered->second, glm::vec3(channel(12.0f), channel(12.0f), channel(20.0f))}};
}

const graphics::Texture2D* TempleInterior::GetTextTexture() const
{
	return _interface != nullptr ? &_interface->GetFontTexture() : nullptr;
}

void TempleInterior::UpdateOptionsAndFutureRooms(float seconds)
{
	const bool arrived = _cameraModel != nullptr && _cameraModel->IsInControl() && !_transitionRoom.has_value();
	if (_interface != nullptr)
	{
		auto& menu = _interface->GetMenu();
		menu.SetInsideTemple(_active);
		if (_cameraModel != nullptr)
		{
			_cameraModel->SetDialogOpen(menu.IsOpen());
		}
		// GameOptionsRoom::Update, while no dialog is up: once the camera has come into the room it opens the game's
		// options, and once they are closed it goes back to the main room
		if (_currentRoom == TempleRoom::Options && arrived && !menu.IsOpen())
		{
			if (!_optionsShown)
			{
				menu.Open();
				menu.ShowPage(gui::GameMenu::Page::Options);
				_optionsShown = true;
			}
			else
			{
				_optionsShown = false;
				GoToRoom(TempleRoom::Main);
			}
		}
		else if (_currentRoom != TempleRoom::Options)
		{
			_optionsShown = false;
		}
	}

	// UniverseRoom: once the camera has come into the room, "The future is still uncertain..." fades in to half over two
	// seconds, from the start each time the camera comes in again
	std::optional<gui::GameInterface::Message> message;
	if (_currentRoom == TempleRoom::Multi && _cameraModel != nullptr)
	{
		if (!_cameraModel->IsInControl())
		{
			_futureTime = 0.0f;
		}
		else
		{
			_futureTime += seconds;
			if (_interface != nullptr)
			{
				message = gui::GameInterface::Message {
				    .text = std::u16string(_interface->GetTexts().Get("HELP_TEXT_DIALOG_ADDITION_129")),
				    .alpha = std::min(128.0f, _futureTime * 64.0f) / 255.0f,
				};
			}
		}
	}
	if (_interface != nullptr)
	{
		_interface->SetMessage(std::move(message));
	}
}

void TempleInterior::UpdateToolTips(float milliseconds)
{
	if (_interface == nullptr || _cameraModel == nullptr)
	{
		return;
	}
	// The rooms choose the tooltip as they are drawn
	const auto hit = GetCursorHit();
	const auto scrolls = _scrolls != nullptr ? _scrolls->GetControls(_currentRoom) : std::vector<TempleScrolls::Control> {};
	const auto toggles = _currentRoom == TempleRoom::Main ? _toggles.GetControls() : std::vector<TempleToggles::Control> {};
	const TempleToolTipInput input {
	    .room = _currentRoom,
	    .inControl = _cameraModel->IsInControl() && !_transitionRoom.has_value(),
	    .zoom = _cameraModel->GetSubMeshZoom(),
	    .lookingAtScroll = _cameraModel->IsLookingAtSubMesh(),
	    .controlHeld = IsControlHeld(),
	    .overPool = _cameraModel->IsOverPool(),
	    .pressingPool = _cameraModel->IsPressingPool(),
	    .hoveredDoor = _cameraModel->GetHoveredDoor(),
	    .overWayBack = _cameraModel->IsOverWayBack(),
	    .hoveredSubMesh = hit.has_value() && hit->room == _currentRoom ? hit->subMesh : std::nullopt,
	    .scrolls = scrolls,
	    .toggles = toggles,
	};
	UpdateTempleToolTip(_toolTip, input);

	// Temple::ProcessGameTurn every 100 milliseconds: TempleRoom::CalculateTooltipsInsideCitadel submits the tooltip,
	// or none under a dialog, and the help system keeps it or ends it
	auto& toolTips = _interface->GetToolTips();
	_toolTipTurnTime += milliseconds;
	for (; _toolTipTurnTime >= k_ToolTipTurnMilliseconds; _toolTipTurnTime -= k_ToolTipTurnMilliseconds)
	{
		if (!_interface->GetMenu().IsOpen() && _toolTip.index.has_value())
		{
			toolTips.Submit(*_toolTip.index, _toolTip.action, _toolTip.arrows);
		}
		toolTips.ProcessTurn();
	}

	// CameraHelp::DrawKeyOrMouse puts the tooltip by the hand, which is where the cursor meets the room
	std::optional<glm::vec2> onScreen;
	if (hit.has_value() && Locator::windowing::has_value())
	{
		const auto size = glm::vec2(Locator::windowing::value().GetSize());
		glm::vec3 screen;
		if (Locator::camera::value().ProjectWorldToScreen(hit->point, glm::vec4(0.0f, 0.0f, size), screen))
		{
			onScreen = glm::vec2(screen);
		}
	}
	_interface->SetHandOnScreen(onScreen);
}

void TempleInterior::LeaveForMapPoint(glm::vec3 point)
{
	// Only for a point near the middle of the map, or on land above the sea
	constexpr float k_NearMiddle = 10.0f;
	constexpr float k_AboveSea = 1.0f;
	if (!Locator::terrainSystem::has_value())
	{
		return;
	}
	const auto onMap = point - _templePosition;
	const auto world = _map.ToWorld(onMap);
	const float altitude = Locator::terrainSystem::value().GetHeightAt(world);
	if (glm::length(glm::vec2(onMap.x, onMap.z)) >= k_NearMiddle && altitude <= k_AboveSea)
	{
		return;
	}
	// GGame::LeaveInsideCitadel puts the camera 50 above the place and 70 along z from it, looking at it
	// TODO(raffclar): the hand feels the click
	_leaveTo = std::make_pair(glm::vec3(world.x, altitude + 50.0f, world.y + 70.0f), glm::vec3(world.x, altitude, world.y));
	// WorldRoomCamera::UpdateMain fades the temple out to white, and WorldRoom::Update leaves once it is
	if (_interface != nullptr)
	{
		_interface->GetScreenFade().FadeThrough(glm::vec3(1.0f));
		_leavingForMapPoint = true;
	}
	else
	{
		RequestLeave();
	}
}

void TempleInterior::FadeIntoRoom()
{
	// Temple::GoToRoom and Temple::Update cover the cut from 1.2, in whatever colour is still fading
	if (_interface != nullptr)
	{
		_interface->GetScreenFade().FadeFrom(1.2f);
	}
}

void TempleInterior::UpdateMapMarkers(float seconds)
{
	_mapMarkers.clear();
	if (_mapTriangles.empty())
	{
		return;
	}
	using namespace ecs::components;
	auto& registry = Locator::entitiesRegistry::value();
	if (_toggles.IsShown(TempleToggles::Display::Temples))
	{
		// fn_0079D830: every player's temple
		registry.Each<const Temple, const Transform>([this](const Temple& temple, const Transform& transform) {
			_mapMarkers.push_back({.kind = TempleMapMarkerKind::Temple,
			                       .position = _map.MarkerPosition(glm::vec2(transform.position.x, transform.position.z)),
			                       .colour = TempleMap::MarkerColour(temple.owner)});
		});
	}
	if (_toggles.IsShown(TempleToggles::Display::Creatures))
	{
		// fn_0079DAB0: every creature, in its player's colour
		registry.Each<const Creature, const Transform>([this](const Creature& creature, const Transform& transform) {
			_mapMarkers.push_back({.kind = TempleMapMarkerKind::Creature,
			                       .position = _map.MarkerPosition(glm::vec2(transform.position.x, transform.position.z)),
			                       .colour = TempleMap::MarkerColour(creature.owner)});
		});
	}
	// TODO(raffclar): WorldRoom::DrawChallenges marks the challenges not yet done, DrawWorldMapSpells the miracles being
	// cast, and InfluenceCircle::Draw the players' influence on the map, as their buttons show them
	// The markers turn a radian a second while the map is drawn
	_mapMarkerTurn += seconds;
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

bool TempleInterior::IsRoomDrawn(TempleRoom room) const
{
	if (room == _currentRoom || room == _transitionRoom)
	{
		return true;
	}
	// From another room, WorldRoom::DrawDoors draws the main room whole only while its door is open, and otherwise just
	// its doors (the submeshes with joints)
	return room == TempleRoom::Main && !_transitionRoom.has_value() && _doors.GetSwing() > k_MainRoomOpenSwing;
}

void TempleInterior::GoToRoom(TempleRoom room)
{
	if (_active && _cameraModel != nullptr)
	{
		if (room != _currentRoom)
		{
			FadeIntoRoom();
		}
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
	if (!_active || _cameraModel == nullptr)
	{
		return std::nullopt;
	}
	// TempleRoom::Draw: the cursor is over the submesh LH3D picks while drawing the room (fn_00795310), and the hand goes
	// where the pick meets it, or onto the floor where the cursor meets that this frame
	const auto& hit = _cameraModel->GetCursorHit();
	const bool onFloor = hit.has_value() && hit->floor;
	const auto& ray = _cameraModel->GetCursorRay();
	if (ray.has_value())
	{
		// LH3D picks along the ray through every room's mesh it draws, and from another room just the main room's doors
		// (WorldRoom::DrawDoors)
		const auto toRoom =
		    glm::inverse(glm::translate(glm::mat4(1.0f), _templePosition) * glm::eulerAngleY(_templeRotation.y));
		const auto origin = glm::vec3(toRoom * glm::vec4(ray->origin, 1.0f));
		const auto direction = glm::vec3(toRoom * glm::vec4(ray->focus - ray->origin, 0.0f));
		auto& meshes = Locator::resources::value().GetMeshes();
		std::optional<graphics::L3DMesh::PickHit> nearest;
		TempleRoom nearestRoom = TempleRoom::Unknown;
		for (const auto& [room, name] : k_TempleInteriorGlows)
		{
			const bool drawn = IsRoomDrawn(room);
			if (!drawn && room != TempleRoom::Main)
			{
				continue;
			}
			const entt::id_type meshId = entt::hashed_string(fmt::format("temple/interior/{}_l3d", name).c_str()).value();
			if (!meshes.Contains(meshId))
			{
				continue;
			}
			// The side rooms' doors aren't drawn shut (see RenderingSystemTemple::PrepareDrawDescs), so nor are they
			// picked
			const auto hidden = GetHiddenSubMeshes(room);
			if (const auto pick = meshes.Handle(meshId)->Pick(origin, direction, !drawn, room != TempleRoom::Main, hidden);
			    pick.has_value() && (!nearest.has_value() || pick->distance < nearest->distance))
			{
				nearest = pick;
				nearestRoom = room;
			}
		}
		if (nearest.has_value())
		{
			return TempleCursorHit {
			    .point = onFloor ? hit->point : ray->origin + (ray->focus - ray->origin) * nearest->distance,
			    .normal = onFloor ? hit->normal : -glm::normalize(ray->focus - ray->origin),
			    .room = nearestRoom,
			    .subMesh = nearest->subMesh,
			};
		}
	}
	if (!hit.has_value())
	{
		return std::nullopt;
	}
	return TempleCursorHit {.point = hit->point, .normal = hit->normal};
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

void TempleInterior::StopCreatureCaveSounds()
{
	if (Locator::audio::has_value())
	{
		auto& audio = Locator::audio::value();
		audio.StopSoundEffect(static_cast<entt::id_type>(audio::SoundId::G_WaterCreatureCave_01));
		audio.StopSoundEffect(static_cast<entt::id_type>(audio::SoundId::G_FireCreatureCave_01));
	}
}

glm::vec2 TempleInterior::GetWaterfallSlide() const
{
	return {0.0f, _waterfallSlide * k_WaterfallSlideLength};
}

void TempleInterior::Update(std::chrono::microseconds dt)
{
	if (_active)
	{
		const float milliseconds = std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(dt).count();
		_waterfallSlide += milliseconds * k_WaterfallSlidePerMillisecond;
		_waterfallSlide -= std::floor(_waterfallSlide);

		// While the player is in it, the creature's room plays the sounds of its water and fire every frame, which
		// carry on as they are when they are playing already. They loop forever, so they stop as the player leaves.
		const bool inCreatureCave = _currentRoom == TempleRoom::CreatureCave;
		if (_soundsOfCreatureCave && !inCreatureCave)
		{
			StopCreatureCaveSounds();
		}
		_soundsOfCreatureCave = inCreatureCave;
		if (inCreatureCave && Locator::audio::has_value())
		{
			auto& audio = Locator::audio::value();
			audio.PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_WaterCreatureCave_01), k_CreatureCaveWaterSound);
			if (const auto fire = CreatureCaveFire(); fire.has_value())
			{
				audio.PlaySoundEffect(static_cast<entt::id_type>(audio::SoundId::G_FireCreatureCave_01), *fire);
			}
		}

		UpdateOptionsAndFutureRooms(milliseconds / 1000.0f);

		// TempleRoom::Update lets the glow of the control under the cursor run out, and TempleRoom::Draw sets it off
		// again as the cursor moves onto another submesh, while no control is held
		_hoverGlow = std::max(0.0f, _hoverGlow - std::floor(milliseconds));
		if (!IsControlHeld())
		{
			const auto hit = GetCursorHit();
			std::optional<std::pair<TempleRoom, uint32_t>> hovered;
			if (hit.has_value() && hit->subMesh.has_value())
			{
				hovered = std::make_pair(hit->room, *hit->subMesh);
			}
			if (hovered != _hovered)
			{
				_hovered = hovered;
				_hoverGlow = 500.0f;
			}
		}

		// The rooms' Draw: the scroll the camera is close to has its text drawn in front of it, and the signs their
		// labels
		_text.clear();
		if (_scrolls != nullptr && _cameraModel != nullptr)
		{
			const auto facts = GatherScrollFacts();
			_scrolls->SetFocus(_cameraModel->GetSubMeshZoom(), facts);
			_scrolls->AppendFocusedText(_text, facts);
		}
		if (_signs != nullptr)
		{
			const auto ticks = static_cast<uint32_t>(
			    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
			        .count());
			for (const auto room : {TempleRoom::Main, TempleRoom::CreatureCave, TempleRoom::Credits})
			{
				if (IsRoomDrawn(room))
				{
					// WorldRoom::Draw lights the label of the door the cursor is over
					const auto highlighted = room == TempleRoom::Main && _cameraModel != nullptr
					                             ? TempleSigns::MainRoomSignOfDoor(_cameraModel->GetHoveredDoor())
					                             : std::nullopt;
					_signs->Append(_text, room, highlighted, ticks);
				}
			}
		}

		UpdateToolTips(milliseconds);

		// WorldRoom::Draw takes the land's heights and brightness for the map every frame the main room is drawn
		_mapTriangles.clear();
		if (IsRoomDrawn(TempleRoom::Main) && Locator::terrainSystem::has_value())
		{
			const auto& island = Locator::terrainSystem::value();
			_map.Build([&island](glm::u16vec2 cell) { return island.FindCell(cell); }, _mapTriangles);
		}
		UpdateMapMarkers(milliseconds / 1000.0f);
		// WorldRoom::Draw moves the pool's shimmer on while the main room is drawn
		if (IsRoomDrawn(TempleRoom::Main))
		{
			_poolTime += milliseconds / 1000.0f;
		}
		if (_cameraModel != nullptr)
		{
			if (const auto point = _cameraModel->TakeMapDoubleClick(); point.has_value())
			{
				LeaveForMapPoint(*point);
			}
		}

		// CreatureRoom::Draw moves the room's effects on while the room is drawn
		if (_creatureCaveEffects != nullptr && IsRoomDrawn(TempleRoom::CreatureCave))
		{
			const auto tickCount = static_cast<uint32_t>(
			    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
			        .count());
			_creatureCaveEffects->Update(static_cast<uint32_t>(milliseconds), tickCount);
		}
	}

	if (_leavingForMapPoint && _interface != nullptr && _interface->GetScreenFade().GetTurns() > 0)
	{
		RequestLeave();
	}
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

	// WorldRoom::InitEngine frames the map on the island as it is each visit, and draws its texture afresh
	++_visits;
	if (Locator::terrainSystem::has_value())
	{
		const auto& island = Locator::terrainSystem::value();
		_map.Frame([&island](glm::u16vec2 cell) { return island.FindCell(cell); });
	}

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
	_leavingForMapPoint = false;
	// Temple::InitEngine covers the way in from 1.2
	FadeIntoRoom();
	_transitionRoom.reset();
	_doors = TempleDoors(PlayDoorSound);
	// CreatureRoom::InitEngine
	if (const auto fire = CreatureCaveFire(); fire.has_value())
	{
		_creatureCaveEffects = std::make_unique<CreatureCaveEffects>(*fire);
	}
	_currentRoom = room;
	auto model = std::make_unique<TempleCameraModel>(LoadCameraPaths(), _currentRoom);
	_cameraModel = model.get();
	_outsideCameraModel = camera.SetModel(std::move(model));
	_cameraModel->StartIntro(_currentRoom, false);
	camera.SetOrigin(_cameraModel->GetTargetOrigin());
	camera.SetFocus(_cameraModel->GetTargetFocus());
	ApplyLens();
	CreateScrolls();
}

void TempleInterior::Deactivate()
{
	if (!_active)
	{
		return;
	}
	if (_soundsOfCreatureCave)
	{
		StopCreatureCaveSounds();
		_soundsOfCreatureCave = false;
	}
	_creatureCaveEffects.reset();
	_scrolls.reset();
	_signs.reset();
	_text.clear();
	_optionsShown = false;
	if (_interface != nullptr)
	{
		_interface->SetMessage(std::nullopt);
		_interface->GetMenu().SetInsideTemple(false);
		// The tooltip ends with nothing to keep it
		_interface->SetHandOnScreen(std::nullopt);
		_interface->GetToolTips().ProcessTurn();
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
	_leavingForMapPoint = false;
	// GGame::LeaveInsideCitadel goes out to the island all white, which fades over a second
	if (_interface != nullptr)
	{
		_interface->GetScreenFade().FadeFrom(1.0f, glm::vec3(1.0f));
	}
	ApplyLens();
	camera.SetOrigin(_playerPositionOutside);
	camera.SetFocus(_playerPositionOutside + glm::quat(_playerRotationOutside) * glm::vec3(0.0f, 0.0f, 1.0f));
	if (_leaveTo.has_value())
	{
		camera.SetOrigin(_leaveTo->first);
		camera.SetFocus(_leaveTo->second);
		_leaveTo.reset();
	}
	if (Locator::cameraPathSystem::value().IsPathing())
	{
		Locator::cameraPathSystem::value().Stop();
	}
	_active = false;
}