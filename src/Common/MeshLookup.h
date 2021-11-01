/*****************************************************************************
 * Copyright (c) 2018-2021 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <unordered_map>

#include <spdlog/spdlog.h>

#include "AllMeshes.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "Enums.h"

namespace openblack
{

class MeshId
{
public:
	using IdType = int16_t;
	constexpr MeshId(MeshPackId id)
	    : _id(static_cast<IdType>(id)) {};
	constexpr MeshId(IdType id)
	    : _id(static_cast<IdType>(id)) {};
	constexpr operator IdType() const { return _id; }
	constexpr bool operator==(const IdType& other) const { return _id == other; }

private:
	IdType _id;
};

template <typename T, typename K = std::hash<T>>
class MeshLookup
{
public:
	MeshLookup(std::initializer_list<typename std::unordered_map<T, MeshPackId, K>::value_type> init)
	    : lookup(init)
	{
	}

	MeshId operator[](T key)
	{
		auto item = lookup.find(key);
		auto meshId = MeshPackId::Dummy;

		if (item == lookup.end())
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("game"), "Unknown item. Using Dummy mesh.");
			return static_cast<MeshId>(meshId);
		}

		return static_cast<MeshId>(item->second);
	}

private:
	std::unordered_map<T, MeshPackId, K> lookup;
};

// TODO(bwrsandman): would be nice to have this as a constexpr but that is not available for strings and maps
template <class C, size_t size>
std::unordered_map<std::string_view, C> makeLookup(std::array<std::string_view, size> strings)
{
	std::unordered_map<std::string_view, C> table;
	for (size_t i = 0; i < size; ++i)
	{
		table.insert(std::make_pair(strings[i], static_cast<C>(i)));
	}
	return table;
}

extern MeshLookup<ecs::components::MobileStatic::Info> mobileStaticMeshLookup;
extern MeshLookup<ecs::components::MobileObject::Info> mobileObjectMeshLookup;
extern MeshLookup<ecs::components::Feature::Info> featureMeshLookup;
} // namespace openblack
