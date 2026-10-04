/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <array>
#include <optional>

#include <entt/resource/resource.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/CameraPath.h"
#include "3D/TempleInteriorInterface.h"
#include "CameraModel.h"
#include "Common/Zoomer.h"

namespace openblack
{

/// The camera inside the temple (InnerCamera, and WorldRoomCamera in the main room). Each room's camera comes in along
/// the room's path. In the main room the player then turns the room around them and leans in and out, by dragging its
/// walls, with the arrow keys and at the screen's top and bottom edges, and walks through its doors by clicking them. In
/// the other rooms the arrow keys turn the camera.
class TempleCameraModel final: public CameraModel
{
public:
	using Room = TempleRoom;
	/// Each room but TempleRoom::Unknown
	static constexpr size_t k_RoomCount = 7;
	using Paths = std::array<entt::resource<CameraPath>, k_RoomCount>;

	struct Pose
	{
		glm::vec3 origin;
		glm::vec3 focus;
	};

	/// Where a ray from the camera meets a room: the cylinder of its walls or its floor (InnerCamera::RayCast)
	struct RoomHit
	{
		/// From the room's axis
		float distance;
		/// Around the room's axis, as atan2 of z and x
		float angle;
		float height;
		bool floor;
		glm::vec3 point;
		/// What the hand turns to: up from the floor, and back along the ray from the walls
		glm::vec3 normal;
	};

	/// The radius of every room's walls, from InnerRoom
	static constexpr float k_RoomRadius = 45.0f;

	/// A ray from origin along direction, which is scaled as LH3DTech's is to the near plane, against a room at the
	/// origin
	[[nodiscard]] static std::optional<RoomHit> RayCastRoom(glm::vec3 origin, glm::vec3 direction);
	/// The main room's door a hit on its walls is in (InnerCamera::CalcDoorHit): the eighth of the room it is in, when
	/// it is within that eighth's door
	[[nodiscard]] static std::optional<uint32_t> DoorAt(const RoomHit& hit);
	/// The room behind a door of the main room: none behind the door out and the wall of the scrolls
	[[nodiscard]] static std::optional<Room> RoomBehindDoor(uint32_t door);
	/// The main room's view at a turn about the room and a lean between looking down at the pool, at 0, and up at the
	/// room, at 1 (WorldRoomCamera)
	[[nodiscard]] static Pose OrbitPose(float yaw, float lean);
	/// A pose about the main room's centre turned to face one of its doors
	[[nodiscard]] static Pose TurnToDoor(const Pose& pose, uint32_t door);

	TempleCameraModel(Paths paths, Room room);
	~TempleCameraModel() final;

	std::optional<CameraInterpolationUpdateInfo> Update(std::chrono::microseconds dt, const Camera& camera) final;
	void HandleActions(std::chrono::microseconds dt) final;
	void SetFlight(glm::vec3 origin, glm::vec3 focus) final;
	[[nodiscard]] glm::vec3 GetTargetOrigin() const final;
	[[nodiscard]] glm::vec3 GetTargetFocus() const final;
	[[nodiscard]] std::chrono::seconds GetIdleTime() const final;
	[[nodiscard]] std::optional<Lens> GetLens() const final;

	/// Comes into a room along its path. When blending, the camera eases from where it is onto the path.
	void StartIntro(Room room, bool blendFromCurrent);
	/// Temple::GoToRoom: cuts to a room, past its path
	void GoToRoom(Room room);
	/// Where the cursor last met the room, which TempleRoom::Draw keeps for the hand
	[[nodiscard]] const std::optional<RoomHit>& GetCursorHit() const { return _cursorHit; }
	/// Walks through the main room's door to a room, as clicking it does, while the player has the main room's camera
	/// @return Whether the camera is on its way
	bool GoThroughDoorTo(Room room);

private:
	enum class State : uint8_t
	{
		Intro,
		Orbit,
		FreeLook,
		ThroughDoor,
	};
	/// How far through a door the camera is (InnerCamera's state 3)
	enum class DoorStage : uint8_t
	{
		Approaching,
		Entering,
		Through,
		Skipped,
	};
	struct Input
	{
		/// 0, held, or double clicked
		uint8_t button;
		glm::vec2 mouse;
		/// The mouse from -1 to 1 across the screen, down it
		glm::vec2 mouseScreen;
		std::optional<RoomHit> hit;
		std::optional<uint32_t> door;
	};

	void Step(float dt, const Input& input);
	void UpdateIntro(float dt, const Input& input);
	void UpdateOrbit(float dt, const Input& input);
	void UpdateFreeLook(float dt);
	void UpdateThroughDoor(float dt, const Input& input);
	void ChangeState();
	void FollowTemple();
	[[nodiscard]] Room GetRoom() const;
	[[nodiscard]] bool InMainRoom() const { return GetRoom() == Room::Main; }

	/// Each room's path in, data/citadel/engine/<room>.cam
	Paths _paths;
	State _state {State::Intro};
	State _nextState {State::Intro};

	// Where the camera is and looks, easing to the targets over the times
	Zoomer3 _origin;
	Zoomer3 _focus;
	Pose _target {};
	float _originTime {0.5f};
	float _focusTime {0.5f};
	bool _easeToTarget {true};

	// The path in
	float _introTime {0.0f};
	float _blendTime {0.0f};
	bool _blendFromPrevious {false};
	Zoomer3 _previousOrigin;
	Zoomer3 _previousFocus;

	// The main room's turn and lean, and where they're heading
	Zoomer _yaw {0.0f};
	Zoomer _lean {0.5f};
	float _yawTarget {0.0f};
	float _leanTarget {0.5f};
	/// How long the mouse has been at the edge of the screen, building up the lean's speed
	float _edgeRamp {0.0f};

	// Turning in the other rooms
	glm::vec2 _turnSpeed {0.0f};

	std::optional<RoomHit> _cursorHit;

	// The press the mouse is held from
	bool _wasPressed {false};
	std::optional<RoomHit> _pressHit;
	float _lastHitAngle {0.0f};
	glm::vec2 _lastMouse {0.0f};

	// Through a door
	float _doorTime {0.0f};
	DoorStage _doorStage {DoorStage::Approaching};
	Pose _doorStart {};
	Pose _doorEnd {};
	std::optional<Room> _doorRoom;
};

} // namespace openblack
