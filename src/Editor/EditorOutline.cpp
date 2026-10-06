/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "EditorOutline.h"

#include <cctype>

#include <algorithm>

#include <fmt/format.h>

namespace openblack::editor
{

std::string_view Name(EntityKind kind)
{
	constexpr std::array<std::string_view, k_EntityKindCount> k_Names {
	    "Creatures", "Villagers",      "Animals",        "Towns",  "Buildings", "Fields", "Trees",
	    "Features",  "Mobile objects", "Mobile statics", "Stores", "Miracles",  "Others",
	};
	return k_Names.at(static_cast<size_t>(kind));
}

namespace
{
bool ContainsIgnoringCase(std::string_view text, std::string_view word)
{
	const auto lower = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
	const auto found = std::ranges::search(text, word, {}, lower, lower);
	return !found.empty() || word.empty();
}
} // namespace

bool MatchesSearch(std::string_view label, std::string_view search)
{
	size_t start = 0;
	while (start < search.size())
	{
		const auto end = std::min(search.find(' ', start), search.size());
		if (end > start && !ContainsIgnoringCase(label, search.substr(start, end - start)))
		{
			return false;
		}
		start = end + 1;
	}
	return true;
}

std::vector<OutlineGroup> Group(std::span<const OutlineEntry> entries, std::string_view search)
{
	std::array<OutlineGroup, k_EntityKindCount> groups {};
	for (size_t i = 0; i < groups.size(); ++i)
	{
		groups.at(i).kind = static_cast<EntityKind>(i);
	}
	for (const auto& entry : entries)
	{
		auto& group = groups.at(static_cast<size_t>(entry.kind));
		++group.total;
		if (MatchesSearch(entry.label, search))
		{
			group.entries.push_back(&entry);
		}
	}
	std::vector<OutlineGroup> result;
	for (auto& group : groups)
	{
		if (group.total > 0)
		{
			result.push_back(std::move(group));
		}
	}
	return result;
}

std::string RowLabel(const OutlineEntry& entry)
{
	return fmt::format("{}  #{}", entry.label, entt::to_entity(entry.entity));
}

} // namespace openblack::editor
