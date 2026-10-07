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

#include <vector>

#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// A gesture the player drew with the hand that the miracles act on
struct GestureEvent
{
	enum class Kind : uint8_t
	{
		/// A circle drawn on the land: the storm, the tornado and both shields are cast at its middle at its size. It is
		/// remembered for a few seconds, and a cast with no circle remembered fails.
		Circle,
		/// A power-up gesture of the held seed was drawn: the seed moves up to that level (0 for the first power-up,
		/// 1 for the second)
		PowerUp,
		/// A scribble: a power-up being asked for is called off first, otherwise the held seed is dropped
		Scribble,
		/// The hand shaken: the held seed is dropped
		Shake,
	};
	Kind kind {Kind::Circle};
	/// The gesture recognised, for the debug window: GestureType::Circle, a power-up's gesture, GestureType::Scribble,
	/// or None for a shake
	GestureType gesture {GestureType::None};
	/// Circle: the point on the land at its middle. Other kinds: where the hand was.
	glm::vec3 centre {0.0f};
	/// Circle: its radius across the land, in world units. 0 for the other kinds.
	float radius {0.0f};
	/// PowerUp: the level asked for, 0 or 1 (a seed's power-up gestures are indexed so). -1 for the other kinds.
	int powerUpLevel {-1};
};

/// Where the miracles hear of the gestures the hand draws. The gesture recogniser implements it and puts in what it
/// recognises; the miracle system takes the events once a frame, oldest first, and decides what each means for the seed
/// in the hand. Debug tools and the testbed put events in by hand, as if they had been drawn, to try the miracles
/// without drawing.
class GestureEventsInterface
{
public:
	virtual ~GestureEventsInterface() = default;

	/// The gestures recognised since the last call, oldest first. Each event is handed out once.
	[[nodiscard]] virtual std::vector<GestureEvent> TakeEvents() = 0;
	/// An event as if it had been recognised, for debug tools and the testbed
	virtual void Inject(const GestureEvent& event) = 0;
	/// A new land: nothing waiting
	virtual void Reset() = 0;
};

} // namespace openblack::ecs::systems
