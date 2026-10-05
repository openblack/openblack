/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "TempleCameraModel.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <utility>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/trigonometric.hpp>

#include "3D/TempleDoors.h"
#include "3D/TempleInteriorInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Windowing/WindowingInterface.h"

using namespace openblack;
using input::BindableActionMap;
using input::UnbindableActionMap;

namespace
{
// LH3DTech casts rays through its near plane, which InnerCamera::PreDraw puts this far out
constexpr float k_NearClip = 0.2f;
constexpr float k_FieldOfView = 90.0f;

/// Hits within this of the room's axis are on the pool, and doors only beyond it
constexpr float k_PoolRadius = 16.0f;

// WorldRoomCamera's views of the main room, looking down at the pool and up at the room
constexpr glm::vec3 k_LowOrigin {0.0f, 14.4f, 14.4f};
constexpr glm::vec3 k_HighOrigin {0.0f, 8.0f, 20.0f};
constexpr glm::vec3 k_LowFocus {0.0f, 0.0f, 4.0f};
constexpr glm::vec3 k_HighFocus {0.0f, 12.0f, 0.0f};
/// Where the main room's camera leans when the player takes it over
constexpr float k_StartLean = 0.7f;
/// The rooms' cameras and doors keep time at twice the real rate: TempleRoom::UpdateMouse and Temple::Update hand them
/// LH3DTech::g_delta_time's milliseconds times 0.002. The times and rates here are in that time, other than the paths,
/// which InnerCamera samples at 500 of their milliseconds to each unit of it, so at their own pace.
constexpr float k_CameraTimePerSecond = 2.0f;
constexpr float k_PathMillisecondsPerCameraTime = 500.0f;
constexpr float k_MaxLean = 1.5f;
/// How long the turn and lean take to catch up with the player
constexpr float k_OrbitEaseTime = 0.3f;
/// Holding the press on the pool's map: how far from the point pressed the camera goes, how fast it gets there, how far
/// it turns and tilts as the mouse crosses the screen, and between which tilts
constexpr float k_MapDistance = 6.0f;
constexpr float k_MapZoomSpeed = 2.0f;
constexpr float k_MapTurnPerScreen = 6.0f;
constexpr float k_MapTiltPerScreen = 2.0f;
constexpr float k_MapMinPitch = 0.62831855f;
constexpr float k_MapMaxPitch = 1.1780972f;

// The way through a door of the main room, facing the door ahead along x: up to the door, then through it
constexpr TempleCameraModel::Pose k_DoorApproach {{0.0f, 25.0f, 0.0f}, {15.0f, 20.0f, 0.0f}};
constexpr TempleCameraModel::Pose k_DoorEntry {{100.0f, 8.0f, 0.0f}, {120.0f, 10.0f, 0.0f}};
/// The door to the room of scrolls isn't one the camera goes through
constexpr uint32_t k_ScrollWall = 6;
/// Temple::Update starts the doorway the camera walks through this far before the start of its swing, at this rate a
/// unit of camera time: it waits a little over a unit, then is open as the camera reaches it
constexpr float k_DoorOpenFrom = -0.5f;
constexpr float k_DoorOpenRate = 0.4f;
/// Walking back to the main room from another room, the doorway opens from the start of its swing, at this rate
constexpr float k_DoorBackOpenFrom = 0.0f;
constexpr float k_DoorBackOpenRate = 2.0f;
/// InnerCamera turns any doorway left open round to close, this long into a room's path, at this rate
constexpr float k_IntroDoorCloseTime = 0.4f;
constexpr float k_IntroDoorCloseRate = 1.6f;

/// ChallengeRoomCamera's views of a room of pictures, from PictureRoomBase and the rooms' constructors: the camera and
/// where it looks, low and looking up at lean 0 and high and looking down at lean 1, about the room's centre, turned by
/// the camera's yaw and raised by its height
struct PictureRoom
{
	glm::vec3 centre;
	glm::vec3 lowOrigin;
	glm::vec3 highOrigin;
	glm::vec3 lowFocus;
	glm::vec3 highFocus;
	float maxHeight;
	/// The credits room turns about where its path ends instead
	bool aroundPathEnd;
	/// The door of the main room the room is behind, which the camera walks back through
	uint32_t door;
};
constexpr float k_PictureMinHeight = 9.0f;
constexpr PictureRoom k_ChallengeRoom {{87.0f, 0.0f, -87.0f},
                                       {0.0f, -7.0f, 8.0f},
                                       {0.0f, 7.0f, 10.0f},
                                       {0.0f, 6.0f, -4.0f},
                                       {0.0f, -6.0f, -4.0f},
                                       80.0f,
                                       false,
                                       7};
constexpr PictureRoom k_SaveGameRoom {{-87.0f, 0.0f, -87.0f},
                                      {0.0f, -7.0f, 8.0f},
                                      {0.0f, 7.0f, 10.0f},
                                      {0.0f, 6.0f, -4.0f},
                                      {0.0f, -6.0f, -4.0f},
                                      80.0f,
                                      false,
                                      5};
constexpr PictureRoom k_OptionsRoom {{87.0f, 0.0f, 87.0f},
                                     {0.0f, -7.0f, 20.0f},
                                     {0.0f, 7.0f, 20.0f},
                                     {0.0f, 6.0f, -4.0f},
                                     {0.0f, -6.0f, -4.0f},
                                     30.0f,
                                     false,
                                     1};
constexpr PictureRoom k_MultiRoom {{-87.0f, 0.0f, 87.0f},
                                   {0.0f, -7.0f, 20.0f},
                                   {0.0f, 7.0f, 20.0f},
                                   {0.0f, 6.0f, -4.0f},
                                   {0.0f, -6.0f, -4.0f},
                                   30.0f,
                                   false,
                                   3};
constexpr PictureRoom k_CreditsRoom {
    {-123.0f, 0.0f, 0.0f}, {0.0f, -0.1f, 0.1f}, {0.0f, 0.1f, 0.1f}, {0.0f, 0.5f, -1.0f}, {0.0f, -0.5f, -1.0f}, 30.0f, true, 4};
/// The rooms of pictures' walls, about their centres (PictureRoomBase's InnerRoom)
constexpr float k_PictureRoomRadius = 37.0f;
/// ChallengeRoomCamera::Init's lean and height
constexpr float k_PictureStartLean = 0.6f;
constexpr float k_PictureStartHeight = 6.0f;
/// How long the turn, lean and height take to catch up with the player
constexpr float k_PictureYawEaseTime = 0.2f;
constexpr float k_PictureLeanEaseTime = 0.3f;
constexpr float k_PictureHeightEaseTime = 0.15f;
/// The lean follows the height, from this at the lowest the camera goes, rising by this for each unit up
constexpr float k_PictureLowLean = 0.2f;
constexpr float k_PictureLeanPerHeight = 0.0084507046f;
/// How fast the arrow keys lean and raise the camera
constexpr float k_PictureLeanSpeed = 1.85f;
constexpr float k_PictureRiseSpeed = 30.0f;
/// How fast a room of pictures turns by itself under a dialog, in radians a unit of camera time, once it has sped up
/// over one
constexpr float k_PictureDialogTurnSpeed = 0.25f;
/// CalcDoorHit's ring the rooms' doors back to the main room are on, about the main room's centre
constexpr float k_PictureDoorRadius = 87.0f;
/// CreatureRoomCamera's ring, and the main room's door the creature's room is behind
constexpr float k_CreatureDoorRadius = 78.0f;
constexpr uint32_t k_CreatureDoor = 0;
// The way back to the main room through a room's door, facing the main room along x: halfway from the camera to the
// door, then through it
constexpr TempleCameraModel::Pose k_DoorBack {{40.0f, 16.0f, 0.0f}, {20.0f, 10.0f, 0.0f}};

// The creature's room: CreatureRoom::InitEngine's InnerRoom, and CreatureRoomCamera's look
constexpr TempleCameraModel::Cylinder k_CreatureRoomCylinder {{120.0f, 0.0f, -120.0f}, TempleCameraModel::k_RoomRadius, false};
/// How far ahead the look's point is
constexpr float k_LookDistance = 20.0f;
/// How far up and down the look turns
constexpr float k_LookMaxPitch = 1.4959966f;
/// How fast the arrow keys speed up the look's turn, and how quickly it slows
constexpr float k_LookAcceleration = 0.3f;
constexpr float k_LookSlowing = 10.0f;
/// How long the camera takes to catch up with the look
constexpr float k_LookEaseTime = 0.15f;

const PictureRoom* PictureRoomOf(TempleRoom room)
{
	switch (room)
	{
	case TempleRoom::Challenge:
		return &k_ChallengeRoom;
	case TempleRoom::SaveGame:
		return &k_SaveGameRoom;
	case TempleRoom::Options:
		return &k_OptionsRoom;
	case TempleRoom::Multi:
		return &k_MultiRoom;
	case TempleRoom::Credits:
		return &k_CreditsRoom;
	case TempleRoom::Main:
	case TempleRoom::CreatureCave:
	case TempleRoom::Unknown:
		break;
	}
	return nullptr;
}

/// The kind of thing the mouse is over (InnerCamera::Update): the pool, the floor or a wall
enum class HitKind : uint8_t
{
	Pool,
	Floor,
	Wall,
};

std::optional<HitKind> KindOf(const std::optional<TempleCameraModel::RoomHit>& hit)
{
	if (!hit.has_value())
	{
		return std::nullopt;
	}
	if (hit->distance < k_PoolRadius)
	{
		return HitKind::Pool;
	}
	return hit->floor ? HitKind::Floor : HitKind::Wall;
}

/// Turns a point about the room's axis, as the main room's camera does by its matrix
glm::vec3 TurnAbout(glm::vec3 point, float angle)
{
	const float cosine = std::cos(angle);
	const float sine = std::sin(angle);
	return {point.x * cosine - point.z * sine, point.y, point.x * sine + point.z * cosine};
}

TempleCameraModel::Pose Mix(const TempleCameraModel::Pose& from, const TempleCameraModel::Pose& to, float amount)
{
	return {glm::mix(from.origin, to.origin, amount), glm::mix(from.focus, to.focus, amount)};
}
} // namespace

TempleCameraModel::Cylinder TempleCameraModel::CylinderOf(Room room)
{
	if (const auto* picture = PictureRoomOf(room); picture != nullptr)
	{
		return {picture->centre, k_PictureRoomRadius};
	}
	if (room == Room::CreatureCave)
	{
		return k_CreatureRoomCylinder;
	}
	return {glm::vec3(0.0f), k_RoomRadius};
}

std::optional<TempleCameraModel::RoomHit> TempleCameraModel::RayCastRoom(glm::vec3 origin, glm::vec3 direction)
{
	return RayCastRoom(origin, direction, Cylinder {glm::vec3(0.0f), k_RoomRadius});
}

std::optional<TempleCameraModel::RoomHit> TempleCameraModel::RayCastRoom(glm::vec3 origin, glm::vec3 direction,
                                                                         const Cylinder& room)
{
	// InnerCamera::RayCast works about the room's centre
	origin -= room.centre;
	const float radius = room.radius;

	// The floor, when the ray points at it enough
	std::optional<float> floorDistance;
	if (room.floor && std::abs(direction.y) > 0.01f && -origin.y / direction.y > 0.0f)
	{
		floorDistance = -origin.y / direction.y;
	}

	// The walls, from outside them or within
	const float a = direction.x * direction.x + direction.z * direction.z;
	const float b = 2.0f * (origin.x * direction.x + origin.z * direction.z);
	const float c = origin.x * origin.x + origin.z * origin.z - radius * radius;
	const float discriminant = b * b - c * a * 4.0f;
	std::optional<float> wallDistance;
	if (discriminant > 0.0f)
	{
		const float root = std::sqrt(discriminant);
		const float t = c >= 0.0f ? (-b - root) / (a + a) : (root - b) / (a + a);
		if (t > 0.0f)
		{
			wallDistance = t;
		}
	}

	bool onFloor = false;
	float t = 0.0f;
	if (wallDistance.has_value() && (!floorDistance.has_value() || *wallDistance < *floorDistance))
	{
		t = *wallDistance;
	}
	else if (floorDistance.has_value())
	{
		t = *floorDistance;
		onFloor = true;
	}
	else
	{
		return std::nullopt;
	}

	const auto point = origin + direction * t;
	return RoomHit {
	    .distance = std::sqrt(point.x * point.x + point.z * point.z),
	    .angle = std::atan2(point.z, point.x),
	    .height = point.y + room.centre.y,
	    .floor = onFloor,
	    .point = point + room.centre,
	    .normal = onFloor ? glm::vec3(0.0f, 1.0f, 0.0f) : -glm::normalize(direction),
	};
}

std::optional<uint32_t> TempleCameraModel::DoorAt(const RoomHit& hit)
{
	// A door is in the walls between these heights, in the middle of an eighth of the room
	constexpr float k_DoorBottom = 7.0f;
	constexpr float k_DoorTop = 23.0f;
	constexpr float k_DoorStart = 6.4581857f;
	constexpr float k_DoorWidth = 0.35f;
	constexpr float k_DoorMargin = 0.1f;
	if (hit.distance <= k_PoolRadius || hit.floor || hit.height <= k_DoorBottom || hit.height >= k_DoorTop)
	{
		return std::nullopt;
	}
	float within = hit.angle + k_DoorStart;
	uint32_t door = 0;
	while (within > glm::quarter_pi<float>())
	{
		within -= glm::quarter_pi<float>();
		++door;
	}
	if (within >= k_DoorWidth - k_DoorMargin || within <= k_DoorMargin)
	{
		return std::nullopt;
	}
	return door & 7u;
}

std::optional<TempleCameraModel::Room> TempleCameraModel::RoomBehindDoor(uint32_t door)
{
	switch (door)
	{
	case 0:
		return Room::CreatureCave;
	case 1:
		return Room::Options;
	case 3:
		return Room::Multi;
	case 4:
		return Room::Credits;
	case 5:
		return Room::SaveGame;
	case 7:
		return Room::Challenge;
	default:
		// The way out
		return std::nullopt;
	}
}

TempleCameraModel::Pose TempleCameraModel::OrbitPose(float yaw, float lean)
{
	return {TurnAbout(glm::mix(k_LowOrigin, k_HighOrigin, lean), yaw), TurnAbout(glm::mix(k_LowFocus, k_HighFocus, lean), yaw)};
}

TempleCameraModel::Pose TempleCameraModel::TurnToDoor(const Pose& pose, uint32_t door)
{
	const float angle = static_cast<float>(door) * glm::quarter_pi<float>();
	return {TurnAbout(pose.origin, angle), TurnAbout(pose.focus, angle)};
}

bool TempleCameraModel::IsPictureRoom(Room room)
{
	return PictureRoomOf(room) != nullptr;
}

TempleCameraModel::Pose TempleCameraModel::PictureOrbitPose(Room room, float yaw, float lean, float height, glm::vec3 pathEnd)
{
	// ChallengeRoomCamera's fn_00785580
	const auto& picture = *PictureRoomOf(room);
	const auto centre = picture.aroundPathEnd ? glm::vec3(pathEnd.x, 0.0f, pathEnd.z) : picture.centre;
	const glm::vec3 raise {0.0f, height, 0.0f};
	return {
	    TurnAbout(glm::mix(picture.lowOrigin, picture.highOrigin, lean), yaw) + centre + raise,
	    TurnAbout(glm::mix(picture.lowFocus, picture.highFocus, lean), yaw) + centre + raise,
	};
}

float TempleCameraModel::PictureStartYaw(const Pose& pathEnd)
{
	// A quarter turn on from the way the path ends looking, which the orbit's views look along unturned
	const auto along = pathEnd.focus - pathEnd.origin;
	return std::atan2(along.z, along.x) + glm::half_pi<float>();
}

TempleCameraModel::Pose TempleCameraModel::LookPose(glm::vec3 from, float heading, float pitch)
{
	// AdjustBubbleZForPitch: GCamera::SetPointFromPointDistanceHeadingAndPitch's point, mirrored through the camera
	const glm::vec3 behind {std::sin(heading) * std::cos(pitch), std::sin(pitch), std::cos(heading) * std::cos(pitch)};
	return {from, from - behind * k_LookDistance};
}

glm::vec2 TempleCameraModel::HeadingAndPitch(const Pose& look)
{
	const auto behind = look.origin - look.focus;
	const float across = std::sqrt(behind.x * behind.x + behind.z * behind.z);
	if (across < 1e-4f)
	{
		// Straight up or down
		return {0.0f, 1.5393804f};
	}
	return {std::atan2(behind.x, behind.z), std::atan2(behind.y, across)};
}

TempleCameraModel::TempleCameraModel(Paths paths, Room room)
    : _paths(std::move(paths))
{
	// ChallengeRoomCamera::Init for each room of pictures
	for (size_t i = 0; i < k_RoomCount; ++i)
	{
		const auto pictureRoom = static_cast<Room>(i);
		if (IsPictureRoom(pictureRoom))
		{
			ResetPictureCamera(pictureRoom);
			auto& picture = _pictures.at(i);
			picture.yaw.Reset(picture.yawTarget);
			picture.lean.Reset(picture.leanTarget);
			picture.height.Reset(picture.heightTarget);
		}
	}

	// InnerCamera::Init puts the camera at the start of its path
	if (const auto& path = _paths.at(static_cast<size_t>(room)); path)
	{
		const auto start = path->SampleAt(std::chrono::milliseconds::zero());
		_origin.Reset(start.position);
		_focus.Reset(start.focus);
		_target = {start.position, start.focus};
	}
}

TempleCameraModel::~TempleCameraModel() = default;

TempleCameraModel::Room TempleCameraModel::GetRoom() const
{
	return Locator::temple::value().GetCurrentRoom();
}

void TempleCameraModel::StartIntro(Room room, bool blendFromCurrent)
{
	// TempleRoom::TriggerIntroCamera
	Locator::temple::value().SetCurrentRoom(room);
	_state = State::Intro;
	_nextState = State::Intro;
	_introTime = 0.0f;
	_blendTime = 0.0f;
	_blendFromPrevious = blendFromCurrent;
	if (blendFromCurrent)
	{
		_previousOrigin = _origin;
		_previousFocus = _focus;
	}
}

void TempleCameraModel::GoToRoom(Room room)
{
	Locator::temple::value().GetDoors().FastClose();
	if (room == GetRoom())
	{
		return;
	}
	// Temple::GoToRoom skips the room's path: the room's camera starts over (Reinit), takes over from the camera, and
	// settles where the player has it
	if (IsPictureRoom(room))
	{
		ResetPictureCamera(room);
	}
	StartIntro(room, true);
	_nextState = ControlOf(room);
	ChangeState();
	const Input still {.button = 0,
	                   .mouse = _lastMouse,
	                   .mouseScreen = glm::vec2(0.0f),
	                   .hit = std::nullopt,
	                   .door = std::nullopt,
	                   .doorBack = std::nullopt};
	Step(1.0f, still);
	Step(1.0f, still);
}

bool TempleCameraModel::GoThroughDoorTo(Room room)
{
	if (_state != State::Orbit)
	{
		return false;
	}
	for (uint32_t door = 0; door < 8; ++door)
	{
		if (RoomBehindDoor(door) == room)
		{
			_doorStart = TurnToDoor(k_DoorApproach, door);
			_doorEnd = TurnToDoor(k_DoorEntry, door);
			_doorRoom = room;
			_nextState = State::ThroughDoor;
			ChangeState();
			return true;
		}
	}
	return false;
}

void TempleCameraModel::HandleActions(std::chrono::microseconds /*dt*/)
{
	// The keys for the rooms are the game's (Game::ProcessTempleRoomKeys), as they work outside the temple too
}

std::optional<CameraModel::CameraInterpolationUpdateInfo> TempleCameraModel::Update(std::chrono::microseconds dt,
                                                                                    const Camera& camera)
{
	const float cameraTime = std::chrono::duration_cast<std::chrono::duration<float>>(dt).count() * k_CameraTimePerSecond;
	const auto& actions = Locator::gameActionSystem::value();

	Input input {};
	if (actions.Get(UnbindableActionMap::DOUBLE_CLICK))
	{
		input.button = 2;
	}
	else if (actions.GetAny(BindableActionMap::MOVE, BindableActionMap::ACTION))
	{
		input.button = 1;
	}
	input.mouse = glm::vec2(actions.GetMousePosition());
	// TempleRoom::UpdateMouse gives a press on a control to the control, which the room's camera then doesn't see
	if (Locator::temple::value().HoldControl(input.button != 0, input.mouse.y))
	{
		input.button = 0;
	}
	if (Locator::windowing::has_value())
	{
		const auto size = glm::vec2(Locator::windowing::value().GetSize());
		input.mouseScreen = input.mouse / (size * 0.5f) - 1.0f;
		input.screen = size;

		// LH3DTech casts from the camera through the mouse on its near plane
		glm::vec3 rayOrigin;
		glm::vec3 rayDirection;
		camera.DeprojectScreenToWorld(input.mouse / size, rayOrigin, rayDirection);
		const auto forward = camera.GetForward();
		if (const float along = glm::dot(rayDirection, forward); along > 0.0f)
		{
			const auto ray = rayDirection * (k_NearClip / along);
			_cursorRay = Pose {camera.GetOrigin(), camera.GetOrigin() + rayDirection};
			input.hit = RayCastRoom(camera.GetOrigin(), ray, CylinderOf(GetRoom()));
			// CalcDoorHit: the doors back to the main room are on a ring about its centre
			if (GetRoom() != Room::Main)
			{
				const float ringRadius = GetRoom() == Room::CreatureCave ? k_CreatureDoorRadius : k_PictureDoorRadius;
				if (const auto ring = RayCastRoom(camera.GetOrigin(), ray,
				                                  Cylinder {glm::vec3(0.0f), ringRadius, CylinderOf(GetRoom()).floor});
				    ring.has_value())
				{
					input.doorBack = DoorAt(*ring);
				}
			}
		}
	}
	if (input.hit.has_value())
	{
		input.door = DoorAt(*input.hit);
		_cursorHit = input.hit;
	}

	Step(cameraTime, input);
	FollowTemple();
	Locator::temple::value().GetDoors().Update(cameraTime);

	auto origin = _origin.GetValue();
	auto focus = _focus.GetValue();
	// ChallengeRoomCamera's pose, and the others': the camera goes over to the scroll it looks at along a cosine
	if (_subMeshLook.has_value() && _subMeshZoom > 0.0f)
	{
		const float along = (1.0f - std::cos(_subMeshZoom * glm::pi<float>())) * 0.5f;
		origin = glm::mix(origin, _subMeshLook->origin, along);
		focus = glm::mix(focus, _subMeshLook->focus, along);
	}
	return CameraInterpolationUpdateInfo {origin, focus, std::chrono::microseconds::zero()};
}

void TempleCameraModel::Step(float dt, const Input& input)
{
	// InnerCamera::Update: the room's camera aims, then the camera eases after its aim
	_originTime = 0.5f;
	_focusTime = 0.5f;
	_easeToTarget = true;
	if (input.hit.has_value())
	{
		_lastHitAngle = input.hit->angle;
	}
	if (input.button != 0 && !_wasPressed)
	{
		_pressHit = input.hit;
		_lastMouse = input.mouse;
		_pressMouse = input.mouse;
	}
	if (IsPictureRoom(GetRoom()))
	{
		UpdatePictureCamera(dt);
	}

	// InnerCamera's state 4, looking at a scroll: the room's camera keeps still, and a press elsewhere or the arrow
	// keys send it back (CreatureRoomCamera::UpdateMain)
	const bool inControl = _state == ControlOf(GetRoom());
	if (_lookingAtSubMesh)
	{
		const auto& actions = Locator::gameActionSystem::value();
		const bool turning = actions.GetAny(BindableActionMap::MOVE_LEFT, BindableActionMap::MOVE_RIGHT,
		                                    BindableActionMap::MOVE_FORWARDS, BindableActionMap::MOVE_BACKWARDS);
		if ((input.button != 0 && !_wasPressed) || turning || !inControl)
		{
			_lookingAtSubMesh = false;
		}
	}
	_subMeshZoom = std::clamp(_subMeshZoom + ((_lookingAtSubMesh ? 0.75f : -0.7f) * dt), 0.0f, 1.0f);
	if (!_lookingAtSubMesh && _subMeshZoom <= 0.0f)
	{
		_subMeshLook.reset();
	}
	_hoveredDoor = _state == State::Orbit ? input.door : std::nullopt;
	_overPool = _state == State::Orbit && KindOf(input.hit) == HitKind::Pool;
	_pressingPool = _state == State::Orbit && input.button != 0 && KindOf(_pressHit) == HitKind::Pool;
	_overWayBack = false;
	if (input.doorBack.has_value() && _state == ControlOf(GetRoom()))
	{
		const auto* picture = PictureRoomOf(GetRoom());
		_overWayBack = GetRoom() == Room::CreatureCave ? *input.doorBack == k_CreatureDoor
		                                               : picture != nullptr && *input.doorBack == picture->door;
	}

	if (!_lookingAtSubMesh)
	{
		switch (_state)
		{
		case State::Intro:
			UpdateIntro(dt, input);
			break;
		case State::Orbit:
			UpdateOrbit(dt, input);
			break;
		case State::Look:
			UpdateLook(dt, input);
			break;
		case State::PictureOrbit:
			UpdatePictureOrbit(dt, input);
			break;
		case State::ThroughDoor:
			UpdateThroughDoor(dt, input);
			break;
		}
	}
	ChangeState();

	if (_easeToTarget)
	{
		_origin.SetDestination(_target.origin, _originTime);
		_focus.SetDestination(_target.focus, _focusTime);
	}
	_origin.Update(dt);
	_focus.Update(dt);
	_wasPressed = input.button != 0;
}

void TempleCameraModel::UpdateIntro(float dt, const Input& input)
{
	const auto control = ControlOf(GetRoom());
	_introTime += dt;
	_blendTime += dt;
	const auto& path = _paths.at(static_cast<size_t>(GetRoom()));
	if (!path)
	{
		_nextState = control;
		return;
	}

	const auto key =
	    path->SampleAt(std::chrono::milliseconds(static_cast<int64_t>(_introTime * k_PathMillisecondsPerCameraTime)));
	_target = {key.position, key.focus};
	const float duration = static_cast<float>(path->GetDuration().count()) / k_PathMillisecondsPerCameraTime;
	// The player takes over half a unit of camera time after the path ends, or with a click
	if (_introTime > duration + 0.5f || (input.button != 0 && !_wasPressed))
	{
		_nextState = control;
	}

	// The path eases into where the player takes over over its last unit of camera time
	if (const float intoControl = _introTime - (duration - 1.0f); intoControl > 0.0f)
	{
		_target = Mix(_target, ControlPose(GetRoom()), std::min(intoControl, 1.0f));
	}
	// Coming through a door, the camera eases off the way it was going over the first two units of camera time
	if (const float fromPrevious = (2.0f - _blendTime) * 0.5f; fromPrevious > 0.0f && _blendFromPrevious)
	{
		_previousOrigin.Update(dt);
		_previousFocus.Update(dt);
		const float amount = std::min(fromPrevious, 1.0f);
		_target = Mix(_target, {_previousOrigin.GetValue(), _previousFocus.GetValue()}, amount);
		_originTime *= 1.0f - amount;
		_focusTime *= 1.0f - amount;
	}
}

void TempleCameraModel::UpdateOrbit(float dt, const Input& input)
{
	const auto& actions = Locator::gameActionSystem::value();
	const auto pressKind = KindOf(_pressHit);

	// Dragging the floor or the walls turns the room to keep them under the mouse, and up and down leans
	if (input.button != 0 && (pressKind == HitKind::Floor || pressKind == HitKind::Wall))
	{
		float turn = _pressHit->angle - _lastHitAngle;
		if (turn < -glm::pi<float>())
		{
			turn += glm::two_pi<float>();
		}
		if (turn > glm::pi<float>())
		{
			turn -= glm::two_pi<float>();
		}
		if (_wasPressed)
		{
			_yawTarget = _yaw.GetValue() + turn;
			_leanTarget += (input.mouse.y - _lastMouse.y) * dt * 0.03f * (1.0f - std::abs(input.mouseScreen.x));
		}
		_lastMouse = input.mouse;
	}
	// Holding the press on the pool draws the camera to the point pressed on the island's map, 6 units from it, and
	// dragging turns about it and tilts.
	if (input.button != 0 && pressKind == HitKind::Pool && _pressHit.has_value())
	{
		if (!_wasPressed)
		{
			// The turn and tilt from the point to where the camera is
			_mapFocus = _pressHit->point;
			const auto away = _target.origin - _mapFocus;
			const auto direction = glm::length(away) > 0.0f ? glm::normalize(away) : away;
			_mapYaw = std::atan2(direction.z, direction.x);
			_mapPitch = std::atan2(direction.y, glm::length(glm::vec2(direction.x, direction.z)));
		}
		const auto moved = (input.mouse - _pressMouse) / glm::max(input.screen, glm::vec2(1.0f));
		const float yaw = _mapYaw - (moved.x * k_MapTurnPerScreen);
		const float pitch = std::clamp(_mapPitch + (moved.y * k_MapTiltPerScreen), k_MapMinPitch, k_MapMaxPitch);
		const auto around = glm::vec3(std::cos(pitch) * std::cos(yaw), std::sin(pitch), std::cos(pitch) * std::sin(yaw));
		_subMeshLook = Pose {_mapFocus + (around * k_MapDistance), _mapFocus};
		_subMeshZoom = std::min(_subMeshZoom + (dt * k_MapZoomSpeed), 1.0f);
	}

	// With the mouse free, the screen's top and bottom edges lean, and the pool slowly draws the camera down to it
	float leanSpeed = 0.0f;
	if (input.button == 0)
	{
		const float edge = input.mouseScreen.y;
		if (edge > 0.9f)
		{
			leanSpeed = (edge - 0.9f) * -0.9f;
			_edgeRamp += dt * 0.2f;
		}
		else if (edge < -0.9f)
		{
			leanSpeed = (-edge - 0.9f) * 0.9f;
			_edgeRamp += dt * 0.2f;
		}
		else if (KindOf(input.hit) == HitKind::Pool)
		{
			leanSpeed = _leanTarget < 0.333f ? _leanTarget * -0.3f : -0.1f;
			_edgeRamp += dt * 0.1f;
		}
		else
		{
			_edgeRamp = 0.0f;
		}
		_edgeRamp = std::clamp(_edgeRamp, 0.0f, 1.0f);
	}
	_leanTarget += leanSpeed * dt * 1.5f * (1.0f - std::cos(_edgeRamp * glm::pi<float>()));

	// The arrow keys turn and lean
	if (actions.Get(BindableActionMap::MOVE_LEFT))
	{
		_yawTarget += dt * 2.0f;
	}
	if (actions.Get(BindableActionMap::MOVE_RIGHT))
	{
		_yawTarget -= dt * 2.0f;
	}
	if (actions.Get(BindableActionMap::MOVE_FORWARDS))
	{
		_leanTarget += dt * 0.85f;
	}
	if (actions.Get(BindableActionMap::MOVE_BACKWARDS))
	{
		_leanTarget -= dt * 0.85f;
	}
	_leanTarget = std::clamp(_leanTarget, 0.0f, k_MaxLean);

	_yaw.SetDestination(_yawTarget, k_OrbitEaseTime);
	_lean.SetDestination(_leanTarget, k_OrbitEaseTime);
	_yaw.Update(dt);
	_lean.Update(dt);
	_target = OrbitPose(_yaw.GetValue(), _lean.GetValue());
	_originTime = 0.0f;
	_focusTime = 0.0f;

	// Clicking a door goes through it
	if (input.button != 0 && !_wasPressed && input.door.has_value() && *input.door != k_ScrollWall)
	{
		_doorStart = TurnToDoor(k_DoorApproach, *input.door);
		_doorEnd = TurnToDoor(k_DoorEntry, *input.door);
		_doorRoom = RoomBehindDoor(*input.door);
		_nextState = State::ThroughDoor;
	}
}

void TempleCameraModel::UpdateLook(float dt, const Input& input)
{
	// CreatureRoomCamera's state 1: the camera stays where the room's path ends, and dragging turns the look with the
	// mouse, by as much as the lens sees across the screen
	const auto& actions = Locator::gameActionSystem::value();
	if (input.button != 0 && !_wasPressed)
	{
		_lookAtPress = _look;
	}
	if ((input.button == 0 && _wasPressed) || input.button == 2)
	{
		_lookAtPress = _look;
	}
	if (input.button != 0 && Locator::windowing::has_value())
	{
		const float width = static_cast<float>(Locator::windowing::value().GetSize().x);
		const float across = 2.0f * std::tan(glm::radians(k_FieldOfView) * 0.5f);
		_look = _lookAtPress + across * (_pressMouse - input.mouse) / width;
	}

	// The arrow keys speed up its turn, which slows as they're let go
	if (actions.Get(BindableActionMap::MOVE_LEFT))
	{
		_lookSpeed.x -= dt * k_LookAcceleration;
	}
	if (actions.Get(BindableActionMap::MOVE_RIGHT))
	{
		_lookSpeed.x += dt * k_LookAcceleration;
	}
	if (actions.Get(BindableActionMap::MOVE_FORWARDS))
	{
		_lookSpeed.y -= dt * k_LookAcceleration;
	}
	if (actions.Get(BindableActionMap::MOVE_BACKWARDS))
	{
		_lookSpeed.y += dt * k_LookAcceleration;
	}
	_look += _lookSpeed;
	_lookAtPress += _lookSpeed;
	// It never looks straight up or down
	_look.y = std::clamp(_look.y, -k_LookMaxPitch, k_LookMaxPitch);
	_lookAtPress.y = std::clamp(_lookAtPress.y, -k_LookMaxPitch, k_LookMaxPitch);
	_look.x = std::fmod(_look.x, glm::two_pi<float>());
	if (_look.x < 0.0f)
	{
		_look.x += glm::two_pi<float>();
	}
	_lookSpeed *= std::exp(-k_LookSlowing * dt);

	_target = LookPose(_lookFrom, _look.x, _look.y);
	_originTime = k_LookEaseTime;
	_focusTime = k_LookEaseTime;
	// Clicking the room's door walks back through it, from halfway between the camera and the door
	// TODO(raffclar): CreatureRoomCamera also lets a press held from the door walk back once it is near, and zooms to the
	//                 scrolls of the creature's attributes
	if (input.button != 0 && !_wasPressed && input.doorBack == k_CreatureDoor)
	{
		WalkBackThrough(k_CreatureDoor);
	}
}

void TempleCameraModel::ResetPictureCamera(Room room)
{
	// Looking along the end of the room's path, half leaning and low
	auto& picture = _pictures.at(static_cast<size_t>(room));
	picture.yawTarget = PictureStartYaw(PathEnd(room));
	picture.leanTarget = k_PictureStartLean;
	picture.heightTarget = k_PictureStartHeight;
}

void TempleCameraModel::UpdatePictureCamera(float dt)
{
	const auto room = GetRoom();
	const auto& pictureRoom = *PictureRoomOf(room);
	auto& picture = _pictures.at(static_cast<size_t>(room));
	picture.leanTarget = std::clamp(picture.leanTarget, 0.0f, 1.0f);
	picture.heightTarget = std::clamp(picture.heightTarget, k_PictureMinHeight, pictureRoom.maxHeight);
	picture.yaw.SetDestination(picture.yawTarget, k_PictureYawEaseTime);
	picture.lean.SetDestination(picture.leanTarget, k_PictureLeanEaseTime);
	picture.height.SetDestination(picture.heightTarget, k_PictureHeightEaseTime);
	picture.yaw.Update(dt);
	picture.lean.Update(dt);
	picture.height.Update(dt);

	// The higher the camera, the more it looks down. Under a dialog it levels out, other than in the library, where the
	// story's history is read in a dialog while the player looks about.
	float leanForHeight = (picture.heightTarget - k_PictureMinHeight) * k_PictureLeanPerHeight + k_PictureLowLean;
	if (_dialogOpen && !pictureRoom.aroundPathEnd)
	{
		leanForHeight = 0.0f;
	}
	picture.leanTarget += (leanForHeight - picture.leanTarget) * (1.0f - std::exp(-dt));
}

void TempleCameraModel::UpdatePictureOrbit(float dt, const Input& input)
{
	// ChallengeRoomCamera's state 1
	const auto room = GetRoom();
	const auto& pictureRoom = *PictureRoomOf(room);
	auto& picture = _pictures.at(static_cast<size_t>(room));
	const auto pressKind = KindOf(_pressHit);

	// Dragging the floor or the walls turns the room and raises the camera to keep them under the mouse. A dialog takes
	// the mouse.
	if (!_dialogOpen && input.button != 0 && (pressKind == HitKind::Floor || pressKind == HitKind::Wall) &&
	    input.hit.has_value())
	{
		float turn = _pressHit->angle - input.hit->angle;
		if (turn < -glm::pi<float>())
		{
			turn += glm::two_pi<float>();
		}
		if (turn > glm::pi<float>())
		{
			turn -= glm::two_pi<float>();
		}
		picture.yawTarget = picture.yaw.GetValue() + turn;
		picture.heightTarget = _pressHit->height - input.hit->height + picture.height.GetValue();
	}

	// Under a dialog the room turns slowly by itself, speeding up over a unit of camera time, rather than with the arrow
	// keys. The library keeps still and the player's.
	// TODO(raffclar): in the multiplayer room while it shows the sessions, the room turns slowly by itself too
	const float turnSpeedUp = _pictureOrbitTime > 0.0f ? std::min(_pictureOrbitTime, 1.0f) : 0.0f;
	_pictureOrbitTime += dt;
	if (_dialogOpen)
	{
		if (!pictureRoom.aroundPathEnd)
		{
			picture.yawTarget += turnSpeedUp * dt * k_PictureDialogTurnSpeed;
		}
	}
	else
	{
		UpdatePictureKeys(dt, picture);
	}

	_target = PictureOrbitPose(room, picture.yaw.GetValue(), picture.lean.GetValue(), picture.height.GetValue(),
	                           PathEnd(room).origin);
	_originTime = 0.0f;
	_focusTime = 0.0f;

	// Clicking the room's door walks back through it, from halfway between the camera and the door
	if (input.button != 0 && !_wasPressed && input.doorBack == pictureRoom.door)
	{
		WalkBackThrough(pictureRoom.door);
	}
	// TODO(raffclar): ChallengeRoomCamera's state 4 looks at the room's pictures
}

void TempleCameraModel::UpdatePictureKeys(float dt, PictureCamera& picture)
{
	// The arrow keys turn, and lean and raise or lower the camera, which the lean then follows
	const auto& actions = Locator::gameActionSystem::value();
	if (actions.Get(BindableActionMap::MOVE_LEFT))
	{
		picture.yawTarget += dt;
	}
	if (actions.Get(BindableActionMap::MOVE_RIGHT))
	{
		picture.yawTarget -= dt;
	}
	float tilt = picture.leanTarget * 2.0f - 1.0f;
	if (actions.Get(BindableActionMap::MOVE_FORWARDS))
	{
		tilt -= dt * k_PictureLeanSpeed;
		picture.heightTarget -= tilt * dt * k_PictureRiseSpeed;
	}
	if (actions.Get(BindableActionMap::MOVE_BACKWARDS))
	{
		tilt += dt * k_PictureLeanSpeed;
		picture.heightTarget -= tilt * dt * k_PictureRiseSpeed;
	}
	picture.leanTarget = tilt * 0.5f + 0.5f;
}

void TempleCameraModel::WalkBackThrough(uint32_t door)
{
	// The rooms' cameras head halfway from where they are to the door, at its height, then through it
	const auto through = TurnToDoor(k_DoorBack, door);
	_doorStart = {(through.origin + _target.origin) * 0.5f, (through.focus + _target.focus) * 0.5f};
	_doorStart.origin.y = through.origin.y;
	_doorEnd = through;
	_doorRoom = Room::Main;
	_nextState = State::ThroughDoor;
}

TempleCameraModel::State TempleCameraModel::ControlOf(Room room) const
{
	if (room == Room::Main)
	{
		return State::Orbit;
	}
	if (room == Room::CreatureCave)
	{
		return State::Look;
	}
	return State::PictureOrbit;
}

TempleCameraModel::Pose TempleCameraModel::PathEnd(Room room) const
{
	const auto& path = _paths.at(static_cast<size_t>(room));
	if (!path)
	{
		return {_origin.GetValue(), _focus.GetValue()};
	}
	const auto end = path->SampleAt(path->GetDuration());
	return {end.position, end.focus};
}

TempleCameraModel::Pose TempleCameraModel::ControlPose(Room room) const
{
	switch (ControlOf(room))
	{
	case State::Orbit:
		// WorldRoomCamera eases into its start, unturned
		return OrbitPose(0.0f, k_StartLean);
	case State::PictureOrbit:
	{
		const auto& picture = _pictures.at(static_cast<size_t>(room));
		return PictureOrbitPose(room, picture.yaw.GetValue(), picture.lean.GetValue(), picture.height.GetValue(),
		                        PathEnd(room).origin);
	}
	default:
		// CreatureRoomCamera takes over where its path ends
		return PathEnd(room);
	}
}

void TempleCameraModel::UpdateThroughDoor(float dt, const Input& input)
{
	if (input.button != 0 && !_wasPressed)
	{
		_doorStage = DoorStage::Skipped;
	}
	// The camera waits by the door, then goes through it
	if (_doorStage == DoorStage::Approaching && _doorTime > 1.5f)
	{
		_target = _doorEnd;
		_origin.SetDestination(_doorEnd.origin, 7.0f);
		_focus.SetDestination(_doorEnd.focus, 7.0f);
		_doorStage = DoorStage::Entering;
	}
	_originTime = 3.0f;
	_focusTime = 3.0f;
	_doorTime += dt;
	// Halfway along, with the doorway near open, the camera is into the room
	if (_doorTime > 5.0f)
	{
		_doorStage = DoorStage::Through;
	}
	_easeToTarget = false;
}

void TempleCameraModel::ChangeState()
{
	if (_nextState == _state)
	{
		return;
	}
	switch (_nextState)
	{
	case State::Intro:
		_introTime = 0.0f;
		_blendTime = 0.0f;
		break;
	case State::Orbit:
		// WorldRoomCamera starts the player facing the scroll, leaning a little up
		_yawTarget = 0.0f;
		_leanTarget = k_StartLean;
		_yaw.Reset(0.0f);
		_lean.Reset(k_StartLean);
		break;
	case State::Look:
	{
		// CreatureRoomCamera::UpdateState: the look starts along the end of the room's path
		const auto end = PathEnd(GetRoom());
		_lookFrom = end.origin;
		_look = HeadingAndPitch(end);
		_lookAtPress = _look;
		_lookSpeed = glm::vec2(0.0f);
		break;
	}
	case State::PictureOrbit:
		// ChallengeRoomCamera::UpdateState
		_pictureOrbitTime = 0.0f;
		break;
	case State::ThroughDoor:
		_target = _doorStart;
		_origin.SetDestination(_doorStart.origin, 3.0f);
		_focus.SetDestination(_doorStart.focus, 3.0f);
		_doorTime = 0.0f;
		_doorStage = DoorStage::Approaching;
		_easeToTarget = false;
		break;
	}
	_state = _nextState;
}

void TempleCameraModel::LookAtSubMesh(glm::vec3 position, glm::vec3 lookAt)
{
	// InnerCamera::FocusOnSubMesh wooshes unless the camera is looking at something already
	if (!_lookingAtSubMesh && Locator::audio::has_value())
	{
		constexpr std::array k_Wooshes {audio::SoundId::G_Woosh_01, audio::SoundId::G_Woosh_02, audio::SoundId::G_Woosh_03,
		                                audio::SoundId::G_Woosh_04};
		const auto ticks =
		    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(k_Wooshes.at(static_cast<size_t>(ticks & 3))),
		                                        std::nullopt);
	}
	_subMeshLook = Pose {position, lookAt};
	_lookingAtSubMesh = true;
}

void TempleCameraModel::FollowTemple()
{
	// Temple::Update: draws the room the camera is heading into, and takes the player into it
	auto& temple = Locator::temple::value();
	auto& doors = temple.GetDoors();
	if (_state != State::ThroughDoor)
	{
		temple.SetTransitionRoom(std::nullopt);
		if (_state == State::Orbit || _state == State::Look || _state == State::PictureOrbit)
		{
			// InnerCamera::CalcDoorHit shuts any door the player has the room's camera back by
			doors.FastClose();
		}
		else if (_state == State::Intro && _introTime > k_IntroDoorCloseTime)
		{
			doors.Close(k_IntroDoorCloseRate);
		}
		return;
	}
	switch (_doorStage)
	{
	case DoorStage::Approaching:
		temple.SetTransitionRoom(std::nullopt);
		doors.FastClose();
		break;
	case DoorStage::Entering:
		temple.SetTransitionRoom(InMainRoom() ? _doorRoom : std::optional<Room>(Room::Main));
		if (InMainRoom())
		{
			// The doorway waits a moment, then swings open ahead of the camera
			doors.Open(_doorRoom.has_value() ? TempleDoors::LeafOf(*_doorRoom) : std::nullopt, k_DoorOpenFrom, k_DoorOpenRate);
		}
		else
		{
			// Back into the main room, its doorway to this room swings open quickly ahead of the camera, and the main room's
			// camera turns it round to close behind it
			doors.Open(TempleDoors::LeafOf(GetRoom()), k_DoorBackOpenFrom, k_DoorBackOpenRate);
		}
		break;
	case DoorStage::Through:
	case DoorStage::Skipped:
		// Through, the doorway swings on open behind the camera, until the next room's camera turns it round to close.
		// Skipping cuts to the room, which shuts it.
		temple.SetTransitionRoom(std::nullopt);
		if (!_doorRoom.has_value())
		{
			temple.RequestLeave();
		}
		else if (_doorStage == DoorStage::Through)
		{
			StartIntro(*_doorRoom, true);
		}
		else
		{
			GoToRoom(*_doorRoom);
		}
		break;
	}
}

void TempleCameraModel::SetFlight(glm::vec3 /*origin*/, glm::vec3 /*focus*/) {}

glm::vec3 TempleCameraModel::GetTargetOrigin() const
{
	return _target.origin;
}

glm::vec3 TempleCameraModel::GetTargetFocus() const
{
	return _target.focus;
}

std::chrono::seconds TempleCameraModel::GetIdleTime() const
{
	return std::chrono::seconds::zero();
}

std::optional<CameraModel::Lens> TempleCameraModel::GetLens() const
{
	// InnerCamera::PreDraw
	return Lens {.horizontalFieldOfView = k_FieldOfView, .nearClip = k_NearClip};
}
