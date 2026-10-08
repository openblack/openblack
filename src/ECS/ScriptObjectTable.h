/*******************************************************************************
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

namespace openblack::ecs::script_objects
{

/// The scripts know the objects they hold by a place in one table of this many places; the first stands for nothing
inline constexpr uint16_t k_Places = 512;
/// A place whose count is this is kept for ever: no reference changes it
inline constexpr uint8_t k_Pinned = 0xFF;

/// Whether the native a script is calling takes control of the objects it is given: only the natives that move,
/// attach, detach or set the script state of an object do
[[nodiscard]] bool TakesControl(uint32_t native);

/// One place of the table: the object, whether a script made it, and how many script references hold it
struct Place
{
	uint32_t object {0};
	bool createdByScript {false};
	uint8_t count {0};
	/// Ever filled since the table was last cleared: a place is only free when it has never been used
	bool used {false};
};

/// What a reference did to an object
enum class Referenced : uint8_t
{
	/// The place is pinned, or there is no such place
	Nothing,
	/// The object's first reference: it is now in a script, and controlled by it if it already was or a script made it
	First,
	/// A later reference: it stays in a script
	Again,
};

/// The table of the objects the scripts hold, as the game keeps it: places are handed out round the table from where
/// the last search stopped, an object already in a script keeps its place, and places are only freed when the land's
/// scripts are cleared
class Table
{
public:
	/// The place of an object, if it has one
	[[nodiscard]] std::optional<uint16_t> Find(uint32_t object) const;
	/// An object a native made or found for a script: its place if it is in a script already and has one, otherwise a
	/// free place; none when every place is taken
	std::optional<uint16_t> Register(uint32_t object, bool createdByScript, bool inScript);
	/// A script reference to the object in a place
	Referenced AddReference(uint16_t place);
	/// A script reference let go; a count never goes below nothing
	void RemoveReference(uint16_t place);
	[[nodiscard]] const Place& At(uint16_t place) const { return _places.at(place); }
	/// The place of one object now holds another, as a tree a script held becomes a dead tree
	void Replace(uint32_t from, uint32_t to);
	/// Every place is free again
	void Clear();

private:
	std::array<Place, k_Places> _places {};
	uint16_t _next {1};
};

} // namespace openblack::ecs::script_objects
