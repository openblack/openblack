/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "UniformTable.h"

#include <algorithm>

namespace openblack::graphics
{

namespace
{
[[nodiscard]] entt::id_type Hash(std::string_view name)
{
	return entt::hashed_string::value(name.data(), name.size());
}
} // namespace

void UniformTable::Add(std::string_view name, UniformHandle handle)
{
	const auto id = Hash(name);
	const auto at = std::ranges::lower_bound(_entries, id, {}, &Entry::id);
	// Among the names that share the hash, the new one goes at their end unless it is already there
	auto end = at;
	for (; end != _entries.end() && end->id == id; ++end)
	{
		if (end->name == name)
		{
			return;
		}
	}
	_entries.insert(end, Entry {.id = id, .name = std::string(name), .handle = handle});
}

std::optional<UniformHandle> UniformTable::Find(std::string_view name) const
{
	const auto id = Hash(name);
	// A plain binary search over the hashes: this runs for every uniform of every draw, debug builds included
	const auto* entries = _entries.data();
	size_t first = 0;
	size_t count = _entries.size();
	while (count > 0)
	{
		const size_t half = count / 2;
		if (entries[first + half].id < id)
		{
			first += half + 1;
			count -= half + 1;
		}
		else
		{
			count = half;
		}
	}
	for (; first < _entries.size() && entries[first].id == id; ++first)
	{
		if (entries[first].name == name)
		{
			return entries[first].handle;
		}
	}
	return std::nullopt;
}

} // namespace openblack::graphics
