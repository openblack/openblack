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

#include <optional>

#include "Enums.h"

// How the action button casts the miracle in the hand. Pressing it over the land or over an object the seed may be cast
// on starts one of three things, by how the seed is cast:
// - a seed cast by a gesture (the fireball, the storms, the shields, the flocks) is armed, with the hand's hum playing,
//   and cast when the button comes up: thrown along the hand's movement, or at the circle drawn;
// - a seed held in the hand (lightning, water, food, wood) is locked on: it is cast at once and applied again every game
//   turn at the hand while the button stays down, and letting go stores what is left back in the seed;
// - a seed placed by the hand (heal, forest, teleport, the beam explosion, the creature spells) is cast at once.
// Pressing with the hand outside the player's influence does nothing at all. Inside it, pressing before the seed is ready,
// or where the seed can't be cast, fails, with the failure's puff and sound. A locked miracle moved over a place it can't
// be cast just isn't applied there, and stays locked until the button comes up. Dropping the seed (a scribble or a
// shake) calls off whatever was started.
// A pure state machine: it is told what the hand is over and says what to do.

namespace openblack::magic
{

/// A circle drawn for a storm or shield is remembered for this many seconds of game time
inline constexpr float k_CircleSeconds = 5.0f;
/// ...which is this many game turns
inline constexpr uint32_t k_CircleTurns = 50;

/// How the hand casts a seed, from its record
struct CastProfile
{
	SpellCastType castType {SpellCastType::SpellCastHandPosition};
	/// Cast on an object rather than at a point: the creature spells
	bool castOnObject {false};
};

/// The press, as the hand is when it comes
struct PressContext
{
	/// The point under the hand is in the player's influence
	bool inInfluence {true};
	/// Held long enough to cast
	bool seedReady {true};
	/// An object under the hand the seed may be cast on
	bool onValidObject {false};
	/// The point under the hand is on the map and the seed may be cast there (its cast rule)
	bool pointValid {false};
	/// The game turn now
	uint32_t turn {0};
};

/// What to cast at
enum class CastTarget : uint8_t
{
	Point,
	Object,
};

/// What the hand does after an input
struct CastActions
{
	/// Cast the seed's miracle, or apply it again when it runs
	std::optional<CastTarget> cast;
	/// The hum of a gesture-cast seed starts, or stops
	bool startHoldLoop {false};
	bool stopHoldLoop {false};
	/// The failure's puff and sound
	bool fail {false};
	/// A locked miracle was let go: what it has left goes back into the seed
	bool unlock {false};
	/// The press came before the seed was ready, and failed
	bool notReady {false};

	bool operator==(const CastActions&) const = default;
};

/// The hand's casting state
struct CastInput
{
	enum class State : uint8_t
	{
		Idle,
		/// A gesture-cast seed waits for the button to come up
		Armed,
		/// A held seed applies itself every turn while the button is down
		Locked,
	};
	State state {State::Idle};
	/// Armed or locked on an object rather than at a point
	bool onObject {false};
	/// The turn it was locked on, and the last turn it was applied
	uint32_t lockTurn {0};
	uint32_t lastApplyTurn {0};
};

/// The action button goes down
[[nodiscard]] CastActions Press(CastInput& input, const CastProfile& profile, const PressContext& context);
/// The action button comes up
[[nodiscard]] CastActions Release(CastInput& input);
/// A game turn passes with the button down: a locked miracle is applied again where the hand is, at most once a turn,
/// if it may be cast there; nothing is shown when it may not
[[nodiscard]] CastActions Tick(CastInput& input, uint32_t turn, bool valid);
/// Whatever was started is called off: the seed was dropped, became ready under a press made too early, or its miracle
/// ended. A locked miracle stores back what it has left.
[[nodiscard]] CastActions Cancel(CastInput& input);
/// How many turns a locked miracle has been held
[[nodiscard]] uint32_t TurnsHeld(const CastInput& input, uint32_t turn);

} // namespace openblack::magic
