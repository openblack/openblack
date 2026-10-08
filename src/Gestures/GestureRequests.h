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
#include <string_view>
#include <vector>

#include "Enums.h"

/// Which gestures the hand is waiting for, and what each would do, as the game decides it once a frame. The game never
/// recognises a gesture it isn't waiting for: a circle only sizes a storm or a shield in the hand, a scribble only does
/// something when there is something for it to do. The requests are listed in the game's order; the first one drawn
/// is acted on, the path is forgotten and the hand rests a moment before it records again.
namespace openblack::gesture
{

/// After a gesture is recognised, the hand's path isn't recorded for this long
constexpr float k_RecognisedPauseSeconds = 0.4f;
/// A circle drawn for a storm or a shield is remembered this long
constexpr float k_CircleSeconds = 5.0f;

/// What a recognised gesture does
enum class Purpose : uint8_t
{
	/// A circle sizing the storm or shield held in the hand, drawn while the Action button is held
	SizeCircle,
	/// Powering the held seed up to a level
	PowerUp,
	/// A scribble with a seed in the hand: the power-up asked for is called off, or else the seed is dropped back where
	/// it came from
	DropSeed,
	/// A scribble with the leash held in the empty hand: the leash comes off
	ShakeOffLeash,
	/// A scribble with a thing in the hand, in the player's influence: the hand lets go of it where it is
	ShakeOffHeld,
	/// The leash gesture: the leash goes on, and the leash picker opens when the creature knows more than one
	LeashGesture,
	/// In the leash picker, a leash's gesture: the creature's leash changes to it
	PickLeash,
	/// In the leash picker, a scribble: the picker closes, leaving the leash on
	ClosePicker,
};

[[nodiscard]] std::string_view Name(Purpose purpose);
/// Whether recognising a gesture for a purpose is shown, with its sound and its trail on the land: every gesture but a
/// scribble, which only shakes something off the hand
[[nodiscard]] bool ShowsRecognition(Purpose purpose);
/// A recognised gesture is shown once at least this many of its points lie on the land
inline constexpr size_t k_RecognitionShownPoints = 2;
/// A point of the path lies on the land once any of its coordinates is further than this from 0
inline constexpr float k_OnLandEpsilon = 1e-4f;
/// A gesture's name, as the game's debug messages give it
[[nodiscard]] std::string_view Name(GestureType gesture);

/// A gesture waited for
struct Request
{
	GestureType gesture {GestureType::None};
	Purpose purpose {Purpose::DropSeed};
	/// PowerUp: the level, 0 to 2
	int powerUpLevel {-1};
	/// PickLeash: the leash
	LeashType leash {LeashType::None};

	bool operator==(const Request&) const = default;
};

/// What the requests depend on
struct HandContext
{
	/// The seed held in the hand
	struct Seed
	{
		/// Held long enough to cast, and not yet cast
		bool ready {true};
		bool cast {false};
		/// The gesture that sizes what it casts, a circle for the storm and the shields
		GestureType sizingGesture {GestureType::None};
		/// The gestures that power it up to levels 0, 1 and 2, none where it has no such level
		std::array<GestureType, 3> powerUpGestures {GestureType::None, GestureType::None, GestureType::None};
		/// The level it casts at, -1 for its plain miracle
		int powerUp {-1};
		/// Whether it can be powered up at all. The game only powers up a seed charged at a worship site's icon.
		bool canPowerUp {true};
	};
	std::optional<Seed> seed;
	/// The point under the hand is in the player's influence: a seed that can't be powered up is only dropped by a
	/// scribble there
	bool inInfluence {true};
	/// A circle is remembered for the seed
	bool circleRemembered {false};
	/// The Action button is held: a storm or a shield is readied, and sized while it is held
	bool actionHeld {false};
	/// The hand holds a thing that isn't a miracle's seed
	bool holdingObject {false};

	/// The player's creature
	struct Creature
	{
		bool fighting {false};
		bool leashed {false};
		/// The leash is tied to something rather than held in the hand
		bool tied {false};
		/// The leashes it knows: aggression, learning, compassion
		std::array<bool, 3> knows {false, false, false};
		LeashType worn {LeashType::None};
	};
	std::optional<Creature> creature;
	/// The leash picker is open
	bool pickerOpen {false};

	/// The gesture that puts the leash on, and each leash's gesture in the picker, by leash number (0 is unused)
	GestureType leashGesture {GestureType::SquareSpirial};
	std::array<GestureType, 4> leashGestures {GestureType::None, GestureType::VerticalScribble, GestureType::EShape,
	                                          GestureType::Heart};
};

/// How many leashes a creature knows
[[nodiscard]] int KnownLeashes(const HandContext::Creature& creature);
/// The gestures waited for, in the order they are tried
[[nodiscard]] std::vector<Request> Requests(const HandContext& context);

/// The leash picker: open after the leash gesture while the creature knows more than one leash, offering each other
/// leash it knows as a gesture to draw. It closes when a leash is picked, on a scribble, when the leash comes off, or
/// after a while.
struct LeashPicker
{
	bool open {false};
	float seconds {0.0f};

	void Open();
	void Close();
	/// Time passes: it closes once open longer than the timeout, or once the leash is off
	void Update(float elapsed, float timeoutSeconds, bool leashed);
};

} // namespace openblack::gesture
