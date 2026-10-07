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

#include <functional>
#include <optional>
#include <span>
#include <vector>

#include <GestureFile.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "ECS/Systems/GestureEventsInterface.h"
#include "Enums.h"
#include "Gestures/GestureMatcher.h"
#include "Gestures/GestureRecorder.h"
#include "Gestures/GestureRequests.h"

namespace openblack::ecs::systems
{

/// The gestures the player draws with the hand. Once a frame the cursor is sampled into the hand's path, the gestures
/// the hand is waiting for are worked out from what it holds and the player's creature's leash (see gesture::Requests),
/// and the path is matched against them with the game's templates. A gesture for a miracle becomes a GestureEvent for
/// the miracles to take; the leash gestures put the leash on, open the leash picker, change the leash and shake it off
/// through the leash system.
class GestureSystemInterface: public GestureEventsInterface
{
public:
	/// How the screen meets the land this frame
	struct View
	{
		/// The window's size in pixels
		glm::vec2 screenSize {0.0f};
		/// The land (or the sea) under a point of the screen, in pixels; none over the sky
		std::function<std::optional<glm::vec3>(glm::vec2 pixel)> landAt;
		/// The point under a point of the screen at the same distance in front of the camera as another point
		std::function<glm::vec3(glm::vec2 pixel, glm::vec3 sameDepthAs)> atDepthOf;
		/// Which way is right on the screen, in the world
		glm::vec3 cameraRight {1.0f, 0.0f, 0.0f};
		/// Where the camera's eye is. The path is forgotten when it has moved since the last frame, unless the camera
		/// is shaking
		glm::vec3 cameraEye {0.0f};
		/// Which way the camera looks
		glm::vec3 cameraForward {0.0f, 0.0f, 1.0f};
		/// Where a recognised gesture's trail is laid under a pixel of the screen (see gesture::TrailView)
		std::function<glm::vec3(glm::ivec2 pixel)> trailPointUnder;
	};

	/// What a frame tells the gestures
	struct Frame
	{
		float seconds {0.0f};
		/// The cursor, in pixels from the top left
		glm::vec2 cursor {0.0f};
		/// Whether the hand is drawing over the world: not over a debug window, in the temple or in a cut scene. The path
		/// is forgotten while it isn't.
		bool overWorld {true};
		/// Whether the Action button is held, which readies a storm or shield to be sized with a circle
		bool actionHeld {false};
		PlayerNames player {PlayerNames::PLAYER_ONE};
		View view;
		/// Whether a camera shake is going on anywhere, near the camera or not
		bool cameraShaking {false};
	};

	/// The last gesture recognised, for the debug window
	struct Recognised
	{
		gesture::Request request;
		gesture::Match match;
		/// Where it was drawn on the screen the path is measured on
		gesture::ScreenBox box;
		/// What the miracles were told of it, if anything
		std::optional<GestureEvent> event;
		/// Seconds since
		float age {0.0f};
		/// Counts the gestures recognised, telling one from the next
		uint32_t number {0};
	};

	/// Once a frame
	virtual void Update(const Frame& frame) = 0;
	/// A path drawn as if by the hand, for the testbed: the points, on the screen the path is measured on (768 pixels
	/// high), are taken one a sample in place of the cursor's, as quickly as a hand would draw them, with the Action
	/// button held throughout or not
	virtual void DrawPath(std::vector<glm::vec2> path, bool holdingAction) = 0;
	/// Whether a path given to DrawPath is still being drawn
	[[nodiscard]] virtual bool IsDrawingPath() const = 0;
	/// The next point of a path given to DrawPath, on the screen the path is measured on, where the hand drawing it is;
	/// none once it is drawn
	[[nodiscard]] virtual std::optional<glm::vec2> GetDrawingPoint() const = 0;
	/// Forgets the hand's path
	virtual void ForgetPath() = 0;

	/// The templates the paths are matched against, empty until the game's file is loaded
	[[nodiscard]] virtual std::span<const gestures::GestureTemplate> GetTemplates() const = 0;
	[[nodiscard]] virtual const gesture::GestureRecorder& GetPath() const = 0;
	/// The screen's width over its height, which the templates' aspect tests are measured with
	[[nodiscard]] virtual float GetScreenAspect() const = 0;
	/// What the hand waits for now, in order
	[[nodiscard]] virtual const std::vector<gesture::Request>& GetRequests() const = 0;
	[[nodiscard]] virtual std::optional<Recognised> GetLastRecognised() const = 0;
	/// Whether the leash picker is open, offering the creature's other leashes as gestures
	[[nodiscard]] virtual bool IsLeashPickerOpen() const = 0;
	/// Whether a circle drawn for the storm or shield in the hand is remembered, and for how much longer
	[[nodiscard]] virtual float GetCircleSecondsLeft() const = 0;
	/// Whether the hand is drawing a gesture that the chain behind it shows: one that powers up the miracle in it, or the
	/// circle that sizes it
	[[nodiscard]] virtual bool IsGesturing() const = 0;
};

} // namespace openblack::ecs::systems
