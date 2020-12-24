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
#include "Entities/Components/Abode.h"
#include "Entities/Components/AnimatedStatic.h"
#include "Entities/Components/Feature.h"
#include "Entities/Components/Mobile.h"
#include "Entities/Components/Tree.h"
#include "Entities/Components/Villager.h"

namespace openblack
{

class MeshId
{
public:
	using IdType = int;
	MeshId(MeshPackId id)
	    : _id(static_cast<IdType>(id)) {};
	MeshId(IdType id)
	    : _id(static_cast<IdType>(id)) {};
	operator int() const { return _id; }
	bool operator==(const int& other) const { return _id == other; }

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
			spdlog::error("Unknown item. Using Dummy mesh.");
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

extern MeshLookup<Tree::Info> treeMeshLookup;
extern MeshLookup<MobileStatic::Info> mobileStaticMeshLookup;
extern MeshLookup<MobileObject::Info> mobileObjectMeshLookup;
extern MeshLookup<Abode::Info> abodeMeshLookup;
extern MeshLookup<Villager::Type, Villager::TypeId> villagerMeshLookup;
extern MeshLookup<Feature::Info> featureMeshLookup;
} // namespace openblack
