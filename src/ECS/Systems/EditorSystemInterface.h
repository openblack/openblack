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

#include <chrono>

#include <glm/vec2.hpp>

#include "Editor/EditorMath.h"
#include "Editor/EditorSelection.h"

namespace openblack::ecs::systems
{

/// The in-game editor's state, which its panels share: whether it is open, the thing picked, the tool the mouse works
/// in the world, snapping, the camera on the picked thing, how fast the keys move the camera, and stepping the game one turn at
/// a time. While the editor is closed it does nothing and leaves the camera to the player.
class EditorSystemInterface
{
public:
	/// What a left click in the world does
	enum class Tool : uint8_t
	{
		/// Picks the thing under the mouse
		Select,
		/// Drags the picked thing over the land
		Move,
		/// Turns the picked thing about the up axis by dragging across
		Rotate,
		/// Leaves the mouse to the game's hand
		GameHand,
	};
	enum class CameraMode : uint8_t
	{
		/// The player's own camera
		Free,
		/// Circling the picked thing
		Orbit,
		/// Trailing behind the picked thing
		Follow,
	};

	virtual ~EditorSystemInterface() = default;

	/// Once a frame: lets go of a picked thing that is gone, keeps the camera on the picked thing and finishes a step
	virtual void Update(std::chrono::microseconds dt) = 0;

	/// Closing the editor hands the camera back to the player
	virtual void SetOpen(bool open) = 0;
	[[nodiscard]] virtual bool IsOpen() const = 0;

	[[nodiscard]] virtual editor::EditorSelection& GetSelection() = 0;
	[[nodiscard]] virtual const editor::EditorSelection& GetSelection() const = 0;

	virtual void SetTool(Tool tool) = 0;
	[[nodiscard]] virtual Tool GetTool() const = 0;
	[[nodiscard]] virtual editor::Snapping& GetSnapping() = 0;

	/// Orbiting and following need something picked, and stay off inside the temple, whose camera is its own
	virtual void SetCameraMode(CameraMode mode) = 0;
	[[nodiscard]] virtual CameraMode GetCameraMode() const = 0;
	/// Turns the orbiting or following camera round the picked thing, in radians across and up
	virtual void TurnCamera(glm::vec2 radians) = 0;
	/// Draws the orbiting or following camera in by steps of the wheel, out for negative steps
	virtual void ZoomCamera(float steps) = 0;
	/// How much faster than the game's own speed the movement keys move the player's camera over the land while the
	/// editor is open. Closing the editor gives the camera back the game's speed; opening it again brings this back.
	virtual void SetCameraMoveSpeed(float speed) = 0;
	[[nodiscard]] virtual float GetCameraMoveSpeed() const = 0;
	/// Brings the picked thing into view: the player's camera flies to it, an orbit or follow comes in close
	virtual void FrameSelection() = 0;

	/// Plays one game turn from paused, then pauses again
	virtual void StepTurn() = 0;
	[[nodiscard]] virtual bool IsStepping() const = 0;
};

} // namespace openblack::ecs::systems
