/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ScriptObjectTable.h"

#include <algorithm>

using namespace openblack::ecs::script_objects;

namespace
{
/// The natives that take control of what they are given: setting a script state or variable, moving, flock attaching,
/// filling a container, walking a path, swapping a creature, attaching to and detaching from the game, keeping a thing
/// for scripts only, and starting a refereed match
constexpr std::array<uint32_t, 14> k_ControllingNatives = {17, 18, 19, 20, 33, 37, 109, 177, 210, 214, 215, 216, 217, 218};
} // namespace

bool openblack::ecs::script_objects::TakesControl(uint32_t native)
{
	return std::ranges::find(k_ControllingNatives, native) != k_ControllingNatives.end();
}

std::optional<uint16_t> Table::Find(uint32_t object) const
{
	for (uint16_t place = 1; place < k_Places; ++place)
	{
		if (_places.at(place).used && _places.at(place).object == object)
		{
			return place;
		}
	}
	return std::nullopt;
}

std::optional<uint16_t> Table::Register(uint32_t object, bool createdByScript, bool inScript)
{
	if (inScript)
	{
		if (const auto place = Find(object))
		{
			return place;
		}
	}
	// The search goes round from where the last one stopped, never at the first place, and gives up after a lap
	for (uint32_t tries = 0; tries < k_Places - 1; ++tries)
	{
		_next &= k_Places - 1;
		if (_next == 0)
		{
			_next = 1;
		}
		auto& place = _places.at(_next);
		if (!place.used && place.count == 0)
		{
			place = {.object = object, .createdByScript = createdByScript, .count = 0, .used = true};
			return _next;
		}
		++_next;
	}
	return std::nullopt;
}

Referenced Table::AddReference(uint16_t place)
{
	if (place == 0 || place >= k_Places)
	{
		return Referenced::Nothing;
	}
	auto& entry = _places.at(place);
	if (entry.count == k_Pinned)
	{
		return Referenced::Nothing;
	}
	const bool first = entry.count == 0;
	++entry.count;
	return first ? Referenced::First : Referenced::Again;
}

void Table::RemoveReference(uint16_t place)
{
	if (place == 0 || place >= k_Places)
	{
		return;
	}
	auto& entry = _places.at(place);
	if (entry.count != 0)
	{
		--entry.count;
	}
}

void Table::Replace(uint32_t from, uint32_t to)
{
	if (const auto place = Find(from))
	{
		_places.at(*place).object = to;
	}
}

void Table::Clear()
{
	// Where the next search starts is kept, as the game keeps it
	_places = {};
}
