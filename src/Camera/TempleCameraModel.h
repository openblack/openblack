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

/// The camera inside the temple (InnerCamera and the rooms' cameras). Each room's camera comes in along the room's path,
/// then the player takes it over:
/// - in the main room (WorldRoomCamera) they turn the room around them and lean in and out, by dragging its walls, with
///   the arrow keys and at the screen's top and bottom edges, and walk through its doors by clicking them
/// - in the creature's room (CreatureRoomCamera) they look around from where its path ends, by dragging and with the
///   arrow keys
/// - in the rooms of pictures (ChallengeRoomCamera, which PictureRoomBase gives the challenge, save game, options,
///   multiplayer and credits rooms) they turn the room around them and raise and lower the camera, by dragging its walls
///   and with the arrow keys, and walk back to the main room by clicking its door
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

	/// The radius of the main room's walls, InnerRoom's
	static constexpr float k_RoomRadius = 45.0f;

	/// A room's cylinder of walls about its axis, standing on its floor (InnerRoom)
	struct Cylinder
	{
		glm::vec3 centre;
		float radius;
		/// Whether rays meet its floor, which the creature's room has none of
		bool floor {true};
	};
	/// The cylinder each room's camera casts the cursor against
	[[nodiscard]] static Cylinder CylinderOf(Room room);

	/// A ray from origin along direction, which is scaled as LH3DTech's is to the near plane, against a room at the
	/// origin
	[[nodiscard]] static std::optional<RoomHit> RayCastRoom(glm::vec3 origin, glm::vec3 direction);
	[[nodiscard]] static std::optional<RoomHit> RayCastRoom(glm::vec3 origin, glm::vec3 direction, const Cylinder& room);
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
	/// Whether a room is one of PictureRoomBase's, whose camera is ChallengeRoomCamera
	[[nodiscard]] static bool IsPictureRoom(Room room);
	/// ChallengeRoomCamera's view of a room of pictures at a turn about its centre, a lean between looking up, at 0, and
	/// down, at 1, and a height. The credits room turns about where its path ends instead of its centre.
	[[nodiscard]] static Pose PictureOrbitPose(Room room, float yaw, float lean, float height, glm::vec3 pathEnd);
	/// The turn ChallengeRoomCamera starts at, which looks along the end of the room's path
	[[nodiscard]] static float PictureStartYaw(const Pose& pathEnd);
	/// CreatureRoomCamera's look from a point, about the vertical and up and down (GCamera's heading and pitch)
	[[nodiscard]] static Pose LookPose(glm::vec3 from, float heading, float pitch);
	/// The heading and pitch of a look at a point (GCamera::GetHeadingAndPitchFromPoints)
	[[nodiscard]] static glm::vec2 HeadingAndPitch(const Pose& look);

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
	/// Where the cursor last met the room's cylinder, which TempleRoom::Draw keeps for the hand
	[[nodiscard]] const std::optional<RoomHit>& GetCursorHit() const { return _cursorHit; }
	/// The line from the camera through the cursor, which LH3D picks the room's mesh along
	[[nodiscard]] const std::optional<Pose>& GetCursorRay() const { return _cursorRay; }
	/// Walks through the main room's door to a room, as clicking it does, while the player has the main room's camera
	/// @return Whether the camera is on its way
	bool GoThroughDoorTo(Room room);
	/// InnerCamera::FocusOnSubMesh: looks at a scroll from close by, from position at lookAt (InnerCamera's state 4),
	/// with a woosh as it starts. A press elsewhere, or the arrow keys, sends the camera back.
	void LookAtSubMesh(glm::vec3 position, glm::vec3 lookAt);
	/// How close the camera has come to what it looks at, from 0 to 1 (InnerCamera +0x450)
	[[nodiscard]] float GetSubMeshZoom() const { return _subMeshZoom; }
	/// Whether the player has the room's camera, past its path in (InnerCamera's state 1)
	[[nodiscard]] bool IsInControl() const { return _state == ControlOf(GetRoom()); }
	/// The main room's door the cursor is over, while the player has its camera (WorldRoomCamera +0x12C)
	[[nodiscard]] std::optional<uint32_t> GetHoveredDoor() const { return _hoveredDoor; }

private:
	enum class State : uint8_t
	{
		Intro,
		/// WorldRoomCamera's control
		Orbit,
		/// CreatureRoomCamera's control
		Look,
		/// ChallengeRoomCamera's control
		PictureOrbit,
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
		/// The door back to the main room the cursor is over, from the other rooms
		std::optional<uint32_t> doorBack;
	};

	void Step(float dt, const Input& input);
	void UpdateIntro(float dt, const Input& input);
	void UpdateOrbit(float dt, const Input& input);
	void UpdateLook(float dt, const Input& input);
	/// ChallengeRoomCamera::UpdateMain's work in every state: the turn, lean and height ease after their targets
	void UpdatePictureCamera(float dt);
	void UpdatePictureOrbit(float dt, const Input& input);
	/// ChallengeRoomCamera::Reinit, as Temple::GoToRoom has it: back to looking along the end of the room's path
	void ResetPictureCamera(Room room);
	/// Walks back to the main room through the door of it a room is behind
	void WalkBackThrough(uint32_t door);
	/// The control state of a room's camera
	[[nodiscard]] State ControlOf(Room room) const;
	/// Where a room's camera has the player as it takes over, which its path eases into over its last second
	[[nodiscard]] Pose ControlPose(Room room) const;
	/// The pose at the end of a room's path
	[[nodiscard]] Pose PathEnd(Room room) const;
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

	// The creature's room: the look from the end of its path, where it was when the mouse was pressed, and how fast the
	// arrow keys turn it
	glm::vec3 _lookFrom {0.0f};
	glm::vec2 _look {0.0f};
	glm::vec2 _lookAtPress {0.0f};
	glm::vec2 _lookSpeed {0.0f};

	/// A room of pictures' turn, lean and height, kept for each room while the player is in the temple
	struct PictureCamera
	{
		Zoomer yaw {0.0f};
		Zoomer lean {0.0f};
		Zoomer height {0.0f};
		float yawTarget {0.0f};
		float leanTarget {0.0f};
		float heightTarget {0.0f};
	};
	std::array<PictureCamera, k_RoomCount> _pictures {};

	std::optional<RoomHit> _cursorHit;
	/// From the camera, towards the point the cursor is over
	std::optional<Pose> _cursorRay;

	// The press the mouse is held from
	bool _wasPressed {false};
	std::optional<RoomHit> _pressHit;
	float _lastHitAngle {0.0f};
	glm::vec2 _lastMouse {0.0f};
	glm::vec2 _pressMouse {0.0f};

	// Looking at a scroll from close by, and how close, which eases in at 0.75 a second and out at 0.7
	std::optional<Pose> _subMeshLook;
	bool _lookingAtSubMesh {false};
	float _subMeshZoom {0.0f};
	std::optional<uint32_t> _hoveredDoor;

	// Through a door
	float _doorTime {0.0f};
	DoorStage _doorStage {DoorStage::Approaching};
	Pose _doorStart {};
	Pose _doorEnd {};
	std::optional<Room> _doorRoom;
};

} // namespace openblack
