/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <entt/core/hashed_string.hpp>

#include "GraphicsHandle.h"

namespace openblack::graphics
{

/// A shader program's uniforms by name. Uniforms are looked up by name for every draw, so the names are kept by their
/// hashes, sorted, and a lookup is a hash of the name, a binary search over the hashes and one comparison of the name.
class UniformTable
{
public:
	/// Adds a uniform; a name already there keeps its first handle
	void Add(std::string_view name, UniformHandle handle);
	[[nodiscard]] std::optional<UniformHandle> Find(std::string_view name) const;
	[[nodiscard]] bool Contains(std::string_view name) const { return Find(name).has_value(); }
	[[nodiscard]] size_t Size() const { return _entries.size(); }

private:
	struct Entry
	{
		entt::id_type id;
		std::string name;
		UniformHandle handle;
	};
	/// Sorted by their hashes, then by their names where two names share a hash
	std::vector<Entry> _entries;
};

} // namespace openblack::graphics
