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
#include <utility>

#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include "3D/TempleInteriorInterface.h"
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
constexpr float k_MaxLean = 1.5f;
/// How long the turn and lean take to catch up with the player
constexpr float k_OrbitEaseTime = 0.3f;

// The way through a door of the main room, facing the door ahead along x: up to the door, then through it
constexpr TempleCameraModel::Pose k_DoorApproach {{0.0f, 25.0f, 0.0f}, {15.0f, 20.0f, 0.0f}};
constexpr TempleCameraModel::Pose k_DoorEntry {{100.0f, 8.0f, 0.0f}, {120.0f, 10.0f, 0.0f}};
/// The door to the room of scrolls isn't one the camera goes through
constexpr uint32_t k_ScrollWall = 6;

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

std::optional<TempleCameraModel::RoomHit> TempleCameraModel::RayCastRoom(glm::vec3 origin, glm::vec3 direction)
{
	// The floor, when the ray points at it enough
	std::optional<float> floorDistance;
	if (std::abs(direction.y) > 0.01f && -origin.y / direction.y > 0.0f)
	{
		floorDistance = -origin.y / direction.y;
	}

	// The walls, from outside them or within
	const float a = direction.x * direction.x + direction.z * direction.z;
	const float b = 2.0f * (origin.x * direction.x + origin.z * direction.z);
	const float c = origin.x * origin.x + origin.z * origin.z - k_RoomRadius * k_RoomRadius;
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
	    .height = point.y,
	    .floor = onFloor,
	    .point = point,
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

TempleCameraModel::TempleCameraModel(Paths paths, Room room)
    : _paths(std::move(paths))
{
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
	if (room == GetRoom())
	{
		return;
	}
	StartIntro(room, true);
	// Temple::GoToRoom skips the room's path, taking the camera straight to where the player has it, and settles it there
	_nextState = InMainRoom() ? State::Orbit : State::FreeLook;
	ChangeState();
	const Input still {
	    .button = 0, .mouse = _lastMouse, .mouseScreen = glm::vec2(0.0f), .hit = std::nullopt, .door = std::nullopt};
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
	const float seconds = std::chrono::duration_cast<std::chrono::duration<float>>(dt).count();
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
	if (Locator::windowing::has_value())
	{
		const auto size = glm::vec2(Locator::windowing::value().GetSize());
		input.mouseScreen = input.mouse / (size * 0.5f) - 1.0f;

		// LH3DTech casts from the camera through the mouse on its near plane
		glm::vec3 rayOrigin;
		glm::vec3 rayDirection;
		camera.DeprojectScreenToWorld(input.mouse / size, rayOrigin, rayDirection);
		const auto forward = camera.GetForward();
		if (const float along = glm::dot(rayDirection, forward); along > 0.0f)
		{
			input.hit = RayCastRoom(camera.GetOrigin(), rayDirection * (k_NearClip / along));
		}
	}
	if (input.hit.has_value())
	{
		input.door = DoorAt(*input.hit);
		_cursorHit = input.hit;
	}

	Step(seconds, input);
	FollowTemple();

	const auto origin = _origin.GetValue();
	const auto focus = _focus.GetValue();
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
	}

	switch (_state)
	{
	case State::Intro:
		UpdateIntro(dt, input);
		break;
	case State::Orbit:
		UpdateOrbit(dt, input);
		break;
	case State::FreeLook:
		UpdateFreeLook(dt);
		break;
	case State::ThroughDoor:
		UpdateThroughDoor(dt, input);
		break;
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
	const auto control = InMainRoom() ? State::Orbit : State::FreeLook;
	_introTime += dt;
	_blendTime += dt;
	const auto& path = _paths.at(static_cast<size_t>(GetRoom()));
	if (!path)
	{
		_nextState = control;
		return;
	}

	const auto key = path->SampleAt(std::chrono::milliseconds(static_cast<int64_t>(_introTime * 1000.0f)));
	_target = {key.position, key.focus};
	const float duration = static_cast<float>(path->GetDuration().count()) * 0.001f;
	// The player takes over half a second after the path ends, or with a click
	if (_introTime > duration + 0.5f || (input.button != 0 && !_wasPressed))
	{
		_nextState = control;
	}

	// The main room's path eases into where the player takes over over its last second
	if (const float intoOrbit = _introTime - (duration - 1.0f); intoOrbit > 0.0f && InMainRoom())
	{
		_target = Mix(_target, OrbitPose(0.0f, k_StartLean), std::min(intoOrbit, 1.0f));
	}
	// Coming through a door, the camera eases off the way it was going over the first two seconds
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
	// TODO(raffclar): Pressing on the pool looks at the island's map in it, and double clicking there leaves the temple
	//                 for that place (WorldRoomCamera's state 4)

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

void TempleCameraModel::UpdateFreeLook(float dt)
{
	// InnerCamera's state 2: the arrow keys turn the camera, slowing as they're let go
	const auto& actions = Locator::gameActionSystem::value();
	if (actions.Get(BindableActionMap::MOVE_LEFT))
	{
		_turnSpeed.x -= dt * 0.1f;
	}
	if (actions.Get(BindableActionMap::MOVE_RIGHT))
	{
		_turnSpeed.x += dt * 0.1f;
	}
	if (actions.Get(BindableActionMap::MOVE_FORWARDS))
	{
		_turnSpeed.y += dt * 0.1f;
	}
	if (actions.Get(BindableActionMap::MOVE_BACKWARDS))
	{
		_turnSpeed.y -= dt * 0.1f;
	}
	_turnSpeed *= std::exp(-2.0f * dt);
	// TODO(raffclar): InnerCamera also moves the camera along its view with two keys not yet known

	// The view turns about the camera's own axes, by the speeds each frame
	const auto origin = _origin.GetValue();
	const auto forward = glm::normalize(_focus.GetValue() - origin);
	const auto right = glm::normalize(glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), forward));
	const auto up = glm::cross(forward, right);
	const float yaw = _turnSpeed.x;
	const float pitch = _turnSpeed.y;
	const auto turned =
	    std::sin(yaw) * right - std::cos(yaw) * std::sin(pitch) * up + std::cos(yaw) * std::cos(pitch) * forward;
	_target = {origin, origin + turned * 10.0f};
	_originTime = 0.0f;
	_focusTime = 0.0f;
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
	// Halfway along, the camera is into the room
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
	case State::FreeLook:
		_turnSpeed = glm::vec2(0.0f);
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

void TempleCameraModel::FollowTemple()
{
	// Temple::Update: draws the room the camera is heading into, and takes the player into it
	auto& temple = Locator::temple::value();
	if (_state != State::ThroughDoor)
	{
		temple.SetTransitionRoom(std::nullopt);
		return;
	}
	switch (_doorStage)
	{
	case DoorStage::Approaching:
		temple.SetTransitionRoom(std::nullopt);
		break;
	case DoorStage::Entering:
		temple.SetTransitionRoom(InMainRoom() ? _doorRoom : std::optional<Room>(Room::Main));
		break;
	case DoorStage::Through:
	case DoorStage::Skipped:
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
