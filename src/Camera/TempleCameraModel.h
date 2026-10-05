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
#include "3D/CreatureCaveTargets.h"
#include "3D/TempleInteriorInterface.h"
#include "CameraModel.h"
#include "Common/Zoomer.h"

namespace openblack
{

/// The camera inside the temple and its rooms' cameras. Each room's camera comes in along the room's path, then the
/// player takes it over:
/// - in the main room they turn the room around them and lean in and out, by dragging its walls, with
///   the arrow keys and at the screen's top and bottom edges, and walk through its doors by clicking them
/// - in the creature's room they look around from where its path ends, by dragging and with the
///   arrow keys
/// - in the rooms of pictures (the challenge, save game, options, multiplayer and credits rooms, which share one kind
///   of camera) they turn the room around them and raise and lower the camera, by dragging its walls
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

	/// Where a ray from the camera meets a room: the cylinder of its walls or its floor
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

	/// The radius of the main room's walls
	static constexpr float k_RoomRadius = 45.0f;

	/// A room's cylinder of walls about its axis, standing on its floor
	struct Cylinder
	{
		glm::vec3 centre;
		float radius;
		/// Whether rays meet its floor, which the creature's room has none of
		bool floor {true};
	};
	/// The cylinder each room's camera casts the cursor against
	[[nodiscard]] static Cylinder CylinderOf(Room room);

	/// A ray from origin along direction, which is scaled to the near plane as the game's picking rays are, against a
	/// room at the origin
	[[nodiscard]] static std::optional<RoomHit> RayCastRoom(glm::vec3 origin, glm::vec3 direction);
	[[nodiscard]] static std::optional<RoomHit> RayCastRoom(glm::vec3 origin, glm::vec3 direction, const Cylinder& room);
	/// The main room's door a hit on its walls is in: the eighth of the room it is in, when
	/// it is within that eighth's door
	[[nodiscard]] static std::optional<uint32_t> DoorAt(const RoomHit& hit);
	/// The room behind a door of the main room: none behind the door out and the wall of the scrolls
	[[nodiscard]] static std::optional<Room> RoomBehindDoor(uint32_t door);
	/// The main room's view at a turn about the room and a lean between looking down at the pool, at 0, and up at the
	/// room, at 1
	[[nodiscard]] static Pose OrbitPose(float yaw, float lean);
	/// A pose about the main room's centre turned to face one of its doors
	[[nodiscard]] static Pose TurnToDoor(const Pose& pose, uint32_t door);
	/// Whether a room is one of the rooms of pictures, which share one kind of camera
	[[nodiscard]] static bool IsPictureRoom(Room room);
	/// The view of a room of pictures at a turn about its centre, a lean between looking up, at 0, and
	/// down, at 1, and a height. The credits room turns about where its path ends instead of its centre.
	[[nodiscard]] static Pose PictureOrbitPose(Room room, float yaw, float lean, float height, glm::vec3 pathEnd);
	/// The turn a room of pictures' camera starts at, which looks along the end of the room's path
	[[nodiscard]] static float PictureStartYaw(const Pose& pathEnd);
	/// The creature's room camera's look from a point, as a heading about the vertical and a pitch up and down
	[[nodiscard]] static Pose LookPose(glm::vec3 from, float heading, float pitch);
	/// The heading and pitch of a look at a point, as the game's camera works them out
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
	/// Cuts to a room, past its path, as the temple does when it sends the player straight to a room
	void GoToRoom(Room room);
	/// Where the cursor meets the room's cylinder this frame, if it does, which the room puts the hand on when it
	/// is the floor
	[[nodiscard]] const std::optional<RoomHit>& GetCursorHit() const { return _cursorHit; }
	/// The line from the camera through the cursor, which the game picks the room's mesh along
	[[nodiscard]] const std::optional<Pose>& GetCursorRay() const { return _cursorRay; }
	/// Walks through the main room's door to a room, as clicking it does, while the player has the main room's camera
	/// @return Whether the camera is on its way
	bool GoThroughDoorTo(Room room);
	/// Looks at a scroll from close by, from position at lookAt, with a woosh as it starts. A press elsewhere, or the
	/// arrow keys, sends the camera back.
	void LookAtSubMesh(glm::vec3 position, glm::vec3 lookAt);
	/// How close the camera has come to what it looks at, from 0 to 1
	[[nodiscard]] float GetSubMeshZoom() const { return _subMeshZoom; }
	/// Whether the player has the room's camera, past its path in
	[[nodiscard]] bool IsInControl() const { return _state == ControlOf(GetRoom()); }
	/// Whether a dialog is up over the temple, as the game's options are in the Game Options room, which takes the rooms
	/// of pictures' cameras from the player
	void SetDialogOpen(bool open) { _dialogOpen = open; }
	/// The main room's door the cursor is over, while the player has its camera
	[[nodiscard]] std::optional<uint32_t> GetHoveredDoor() const { return _hoveredDoor; }
	/// Whether the cursor is over the main room's pool, and whether a press began on it
	[[nodiscard]] bool IsOverPool() const { return _overPool; }
	[[nodiscard]] bool IsPressingPool() const { return _pressingPool; }
	/// Whether the cursor is over the door a room other than the main room is left by, while the player has its camera
	[[nodiscard]] bool IsOverWayBack() const { return _overWayBack; }
	/// Whether the camera looks at a scroll from close by
	[[nodiscard]] bool IsLookingAtSubMesh() const { return _lookingAtSubMesh; }
	/// The creature's room's target the cursor is over, while the player has its camera
	[[nodiscard]] std::optional<CreatureCaveTargets::Target> GetCaveTarget() const { return _caveTarget; }
	/// Whether the camera is zooming to one of the creature's room's targets, or there
	[[nodiscard]] bool IsZoomingToCaveTarget() const { return _caveZoomTarget == 1.0f; }
	/// The point of the pool the island's map was double clicked at, once, in the camera's space: the player asks to
	/// leave the temple for that place
	[[nodiscard]] std::optional<glm::vec3> TakeMapDoubleClick() { return std::exchange(_mapDoubleClick, std::nullopt); }

private:
	enum class State : uint8_t
	{
		Intro,
		/// The main room's control
		Orbit,
		/// The creature's room's control
		Look,
		/// The rooms of pictures' control
		PictureOrbit,
		ThroughDoor,
	};
	/// How far through a door the camera is
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
		/// The screen's size in pixels
		glm::vec2 screen;
		std::optional<RoomHit> hit;
		std::optional<uint32_t> door;
		/// The door back to the main room the cursor is over, from the other rooms
		std::optional<uint32_t> doorBack;
		/// Where on the screen the creature's room's targets are, in pixels down from the top
		std::array<std::optional<glm::vec2>, CreatureCaveTargets::k_Count> caveTargets;
	};

	void Step(float dt, const Input& input);
	void UpdateIntro(float dt, const Input& input);
	void UpdateOrbit(float dt, const Input& input);
	void UpdateLook(float dt, const Input& input);
	/// The creature's room's targets: a click on one zooms to it, a press anywhere or the arrow keys zoom back,
	/// and the exit, zoomed to, leaves the temple
	void UpdateCaveTargets(float dt, const Input& input);
	/// The place of a point of the creature's room's mesh
	[[nodiscard]] static std::optional<glm::vec3> CaveMeshPoint(uint32_t index);
	/// Zooms the camera to a target of the creature's room
	void ZoomToCaveTarget(CreatureCaveTargets::Target target);
	/// Zoomed back to no target at once
	void ResetCaveTargets();
	/// A room of pictures' camera's work in every state: the turn, lean and height ease after their targets
	void UpdatePictureCamera(float dt);
	void UpdatePictureOrbit(float dt, const Input& input);
	/// As when the temple cuts straight to a room: back to looking along the end of the room's path
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
	/// The arrow keys' turn, lean and rise of a room of pictures' camera
	static void UpdatePictureKeys(float dt, PictureCamera& picture);

	std::optional<RoomHit> _cursorHit;
	/// From the camera, towards the point the cursor is over
	std::optional<Pose> _cursorRay;

	// The press the mouse is held from
	bool _wasPressed {false};
	bool _dialogOpen {false};
	bool _overPool {false};
	bool _pressingPool {false};
	bool _overWayBack {false};
	/// How long a room of pictures' camera has had the player, which its turn under a dialog speeds up over
	float _pictureOrbitTime {0.0f};
	std::optional<RoomHit> _pressHit;
	float _lastHitAngle {0.0f};
	glm::vec2 _lastMouse {0.0f};
	glm::vec2 _pressMouse {0.0f};

	// Looking at a scroll from close by, and how close, which eases in at 0.75 a second and out at 0.7
	std::optional<Pose> _subMeshLook;
	bool _lookingAtSubMesh {false};
	float _subMeshZoom {0.0f};
	/// Holding the press on the pool's map: the point pressed, which the camera turns about, and the turn and tilt it
	/// had from it as the press began
	glm::vec3 _mapFocus {0.0f};
	float _mapYaw {0.0f};
	float _mapPitch {0.0f};
	std::optional<glm::vec3> _mapDoubleClick;
	bool _leavingByMap {false};
	std::optional<uint32_t> _hoveredDoor;

	// The creature's room's targets: the one the cursor is over, the one zoomed to and where the camera looks at it from,
	// and how far it has zoomed, from 0 to 1, after the zoom's target
	std::optional<CreatureCaveTargets::Target> _caveTarget;
	std::optional<CreatureCaveTargets::Target> _caveZoomedTo;
	Pose _caveLook {};
	float _caveZoom {0.0f};
	float _caveZoomTarget {0.0f};
	/// Whether the mouse has moved since it was pressed, which makes the press a drag rather than a click
	bool _pressDragged {false};

	// Through a door
	float _doorTime {0.0f};
	DoorStage _doorStage {DoorStage::Approaching};
	Pose _doorStart {};
	Pose _doorEnd {};
	std::optional<Room> _doorRoom;
};

} // namespace openblack
