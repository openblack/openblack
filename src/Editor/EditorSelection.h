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

#include <functional>
#include <optional>

#include <entt/entity/entity.hpp>

namespace openblack::editor
{

/// The one thing picked in the editor, which every panel shows and acts on. Each pick is counted, so a panel can tell a
/// new pick from the one it last saw, even of the same thing again.
class EditorSelection
{
public:
	void Select(entt::entity entity)
	{
		if (entity == entt::null)
		{
			Clear();
			return;
		}
		_entity = entity;
		++_picks;
	}
	void Clear()
	{
		if (_entity.has_value())
		{
			_entity.reset();
			++_picks;
		}
	}
	[[nodiscard]] std::optional<entt::entity> Get() const { return _entity; }
	[[nodiscard]] bool IsSelected(entt::entity entity) const { return _entity == entity; }
	[[nodiscard]] bool Empty() const { return !_entity.has_value(); }
	/// How many times the selection has changed
	[[nodiscard]] uint32_t GetPicks() const { return _picks; }
	/// Lets the selection go once what it picked is gone, by the registry's say
	void Validate(const std::function<bool(entt::entity)>& exists)
	{
		if (_entity.has_value() && !exists(*_entity))
		{
			Clear();
		}
	}

private:
	std::optional<entt::entity> _entity;
	uint32_t _picks {0};
};

} // namespace openblack::editor
