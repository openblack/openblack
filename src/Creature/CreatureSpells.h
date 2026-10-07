/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <span>
#include <vector>

#include <entt/entity/entity.hpp>

#include "Enums.h"

// The miracles cast on a creature: freeze, small and big ("miracle grow"), weak and strong, fat and thin, invisible,
// nice and nasty, itchy, and the never finished hungry, frightened, tired, ill and thirsty.
//
// Each comes over the creature in three parts: it eases in over a start time, holds for its time, then eases out again
// over a finish time, the creature put back as it was. Casting one that is already on the creature adds its time to
// it; casting another of the same kind (big while small, nice while nasty) cuts the first short and waits for it to
// finish before it starts. Pure functions of a creature's spells, so they are tested on their own.

namespace openblack::creature_spells
{

/// In the order of the game's tables
enum class Spell : uint8_t
{
	Freeze,
	Small,
	Big,
	Weak,
	Strong,
	Fat,
	Thin,
	Invisible,
	Nice,
	Nasty,
	Hungry,
	Frightened,
	Tired,
	Ill,
	Thirsty,
	Itchy,

	_Count
};
inline constexpr size_t k_SpellCount = static_cast<size_t>(Spell::_Count);

/// The spell a magic type casts on a creature, none for any other magic type
[[nodiscard]] std::optional<Spell> SpellOf(MagicType type);
[[nodiscard]] std::optional<Spell> SpellOf(CreatureReceiveSpellType type);

/// Spells of one kind can't be on a creature together
enum class Kind : uint8_t
{
	Size,
	Strength,
	/// Nice, nasty and itchy: what the creature wants
	Mood,
	/// Freeze and invisible: how the creature is seen
	Look,
	/// Fat, thin and the unfinished spells
	Other,
};
[[nodiscard]] Kind KindOf(Spell spell);

/// What a spell does to the creature: wants the desire above all others, and pulls a body value towards a target
struct Effect
{
	/// The desire it makes fully dominant, none for none
	std::optional<uint8_t> desire;
	/// The game's sound action of the creature taking it, 0 for none
	int32_t soundAction {0};
};
[[nodiscard]] Effect EffectOf(Spell spell);

/// How far through a spell's three parts a creature is
enum class Phase : uint8_t
{
	Off,
	/// Cast, starting once its delay has passed
	Waiting,
	Starting,
	Holding,
	Finishing,
};

/// The times of a spell's parts, in seconds, from its tables
struct Timing
{
	float startSeconds {1.0f};
	float finishSeconds {1.0f};
};

/// One spell on a creature
struct Slot
{
	Phase phase {Phase::Off};
	/// Turns left of this part
	int32_t turnsLeft {0};
	/// Turns it holds for, once started
	int32_t holdTurns {0};
	/// The running miracle behind it, closed when the spell ends
	entt::entity miracle {entt::null};
	/// The creature's value before the spell began, put back as it ends
	float before {0.0f};
};

/// A spell cast while another of its kind was on the creature, waiting for that one to finish
struct Waiting
{
	Spell spell;
	int32_t holdTurns;
	entt::entity miracle;
};

/// A spell cast on a creature starts this long after it is cast
inline constexpr float k_StartDelaySeconds = 2.0f;
/// No spell may be cast on a creature with more than this many waiting
inline constexpr size_t k_MostWaiting = 5;

/// Every spell on a creature
struct Spells
{
	std::array<Slot, k_SpellCount> slots {};
	std::vector<Waiting> waiting;
	/// Turns between a spell being cast and its start
	int32_t startDelayTurns {0};
	/// Whether a spell puts the creature back as it was when it ends; a script may turn this off, and then a spell that
	/// has held its time simply stops, leaving the creature as it made it
	bool reversion {true};

	[[nodiscard]] Slot& operator[](Spell spell) { return slots.at(static_cast<size_t>(spell)); }
	[[nodiscard]] const Slot& operator[](Spell spell) const { return slots.at(static_cast<size_t>(spell)); }
	[[nodiscard]] bool IsActive(Spell spell) const { return (*this)[spell].phase != Phase::Off; }
	[[nodiscard]] bool IsKindActive(Kind kind) const;
	/// Too many spells wait for another to be cast on it
	[[nodiscard]] bool QueueFull() const { return waiting.size() > k_MostWaiting; }
};

/// Seconds as whole turns
[[nodiscard]] int32_t TurnsOf(float seconds, float turnsPerSecond);
/// Every spell on a creature is brought to its end: none holds on once started, and one holding now finishes at its next
/// turn. Whether none is on it any more.
bool TryFinishAll(Spells& spells);

/// What became of a spell cast on a creature
enum class Received : uint8_t
{
	/// It starts next turn
	Started,
	/// It was on the creature already: its time is added to it
	Extended,
	/// Another of its kind was on the creature, and finishes first
	Queued,
};
struct ReceiveResult
{
	Received received;
	/// A miracle the new one takes the place of, to close down
	entt::entity replaced {entt::null};
};
/// A spell whose miracle has gone holds no longer: it eases out next turn if it was holding
void FinishEarly(Slot& slot);

/// A spell is cast on the creature for some turns, behind a miracle; it starts after the spells' delay
ReceiveResult Receive(Spells& spells, Spell spell, int32_t holdTurns, entt::entity miracle);

/// What a turn of the spells has the creature go through, in order
enum class Event : uint8_t
{
	/// The spell begins: its value is kept to put back, its desire wanted
	Start,
	/// It eases in or out: the creature's value is pulled the ratio of the way to the target
	Ease,
	/// It holds: what it does every turn it is on (itchy keeps the creature off the leash)
	Hold,
	/// It begins to wear off
	BeginFinish,
	/// It has worn off: the creature is put back as it was
	Finish,
};
struct TurnEvent
{
	Spell spell;
	Event event;
	/// How far the spell is over the creature, 0 to 1
	float ratio;
};
struct TurnResult
{
	std::vector<TurnEvent> events;
	/// The miracles of the spells that ended, to close down
	std::vector<entt::entity> ended;
};
/// One turn of a creature's spells
[[nodiscard]] TurnResult Step(Spells& spells, std::span<const Timing, k_SpellCount> timings, float turnsPerSecond);

/// How far a spell is over the creature: rising from 0 to 1 as it starts, 1 while it holds, falling back to 0 as it
/// finishes
[[nodiscard]] float Ratio(const Slot& slot, const Timing& timing, float turnsPerSecond);

// The spells' targets

/// Big makes a creature this much bigger, no bigger than its largest; small this much smaller, no smaller than its
/// smallest
inline constexpr float k_BigFactor = 1.8f;
inline constexpr float k_SmallFactor = 0.5555556f;
inline constexpr float k_WeakStrength = 0.1f;
inline constexpr float k_StrongStrength = 1.0f;
inline constexpr float k_FatFatness = 0.95f;
inline constexpr float k_ThinFatness = 0.1f;
/// Invisible fizzes the creature out to this much
inline constexpr float k_InvisibleFizz = 0.75f;
inline constexpr float k_NiceAlignment = 1.0f;
inline constexpr float k_NastyAlignment = -1.0f;
/// As a mood spell wears off its desire drops to the least dominant, below the others by this factor
inline constexpr float k_LeastDominantFactor = 1.3f;

/// A frozen creature's look: its colour multiplied by white thawed, by this dark icy blue fully frozen, with an icy
/// sheen added over it by how frozen it is
inline constexpr uint32_t k_FrozenColour = 0x354F8Du;
/// The colour, 0xRRGGBB, the land's light on a creature is multiplied by at a freeze from 0 to 1
[[nodiscard]] uint32_t FrozenTint(float freeze);

/// The size a size spell takes a creature to from its size before. Out of a fight small takes it down to its smallest
/// and big up to its largest, neither ever the other way; in a fight they take its size now down to five ninths or up to
/// 1.8 times, within the same limits.
[[nodiscard]] float SizeTarget(Spell spell, float before, float smallest, float largest,
                               std::optional<float> sizeInFight = std::nullopt);
/// A creature's smallest and largest sizes unless a script changes them
inline constexpr float k_SmallestSize = 0.2f;
inline constexpr float k_LargestSize = 2.4f;
/// The value a spell pulls the creature towards, for the spells that pull one: size (from SizeTarget), strength,
/// fatness, fizz, alignment, or the freeze
[[nodiscard]] std::optional<float> Target(Spell spell);
/// A value the ratio of the way from before to the target
[[nodiscard]] float Ease(float before, float target, float ratio);

/// What a creature's size, strength and alignment are saved as
struct SavedBody
{
	float size {1.0f};
	float strength {0.5f};
	float alignment {0.0f};
};
/// The values a creature is saved with while spells are on it: those it had before the spells, so that it is saved as
/// itself. Size: before big, else small, while a size spell is on; strength: before strong, else weak; alignment:
/// before nice, else nasty. Anything else, and a kind on with neither of its two spells, as it is now.
[[nodiscard]] SavedBody ValuesToSave(const Spells& spells, const SavedBody& now);

} // namespace openblack::creature_spells
