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

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec2.hpp>

/// The palette's lists of things to place, and how they are named and laid out, as plain data
namespace openblack::editor
{

/// The palette's tabs and the kinds of thing each places
enum class PlaceKind : uint8_t
{
	Creature,
	Villager,
	Building,
	Tree,
	Feature,
	MobileObject,
	MobileStatic,
	/// A miracle dispenser, its type the miracle it gives
	Dispenser,
	/// A one-shot miracle's bubble floating over the land, its type the miracle it gives
	MiracleBubble,
};

/// Something picked in the palette to place: its kind and its type within the kind
struct PlaceItem
{
	PlaceKind kind {PlaceKind::Tree};
	int32_t type {0};

	friend bool operator==(const PlaceItem&, const PlaceItem&) = default;
};

/// A game table's name made readable: underscores become spaces and each word starts with a capital, the rest lower
/// case, as "OAK_TREE_A" becomes "Oak Tree A"
[[nodiscard]] std::string TitleCase(std::string_view text);
/// How many characters of whole words, each ended by an underscore, every name starts with, such as "FEATURE_" for a
/// list of features, to be left out of each name; none for fewer than two names
[[nodiscard]] size_t CommonWordPrefix(std::span<const std::string_view> names);
/// Every name made readable, with the words they all start with left out
[[nodiscard]] std::vector<std::string> ReadableNames(std::span<const std::string_view> names);

/// Points for a number of things laid out in rows on the land, as near square as can be, spaced apart and centred on
/// the origin, row by row from the north west
[[nodiscard]] std::vector<glm::vec2> GridLayout(size_t count, float spacing);

/// How far one notch of the wheel or one tap of the rotate keys turns the thing being placed, in degrees
constexpr float k_PlaceTurnDegrees = 15.0f;

} // namespace openblack::editor
