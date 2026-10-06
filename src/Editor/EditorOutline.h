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
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include <entt/entity/entity.hpp>

/// The outliner's lists: every thing on the land sorted into kinds, narrowed by a search, kept as plain data so the
/// sorting and searching can be tested without a registry.
namespace openblack::editor
{

enum class EntityKind : uint8_t
{
	Creature,
	Villager,
	Animal,
	Town,
	Building,
	Field,
	Tree,
	Feature,
	MobileObject,
	MobileStatic,
	Store,
	/// Miracle dispensers and the bubbles they float
	Miracle,
	Other,

	_Count
};
constexpr size_t k_EntityKindCount = static_cast<size_t>(EntityKind::_Count);
[[nodiscard]] std::string_view Name(EntityKind kind);

struct OutlineEntry
{
	entt::entity entity {entt::null};
	EntityKind kind {EntityKind::Other};
	std::string label;
};

/// One kind's entries that pass the search, in the order given
struct OutlineGroup
{
	EntityKind kind {EntityKind::Other};
	std::vector<const OutlineEntry*> entries;
	/// How many of the kind there are before the search
	size_t total {0};
};

/// Whether a label holds every word of the search, ignoring case; an empty search matches everything
[[nodiscard]] bool MatchesSearch(std::string_view label, std::string_view search);
/// The entries sorted into a group per kind, in the kinds' order, the empty kinds left out
[[nodiscard]] std::vector<OutlineGroup> Group(std::span<const OutlineEntry> entries, std::string_view search);
/// The entry's label as the outliner shows it: its own label and its entity number, so two alike can be told apart
[[nodiscard]] std::string RowLabel(const OutlineEntry& entry);

} // namespace openblack::editor
