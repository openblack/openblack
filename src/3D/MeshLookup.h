/*****************************************************************************
 * Copyright (c) 2018-2020 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include "AllMeshes.h"
#include "Entities/Components/AnimatedStatic.h"
#include "Entities/Components/Villager.h"

#include <spdlog/spdlog.h>

#include <unordered_map>

namespace openblack
{

enum class TreeInfo;

class MeshId
{
public:
	using IdType = int;
	MeshId(MeshPackId id) : _id(static_cast<IdType>(id)) {};
	MeshId(IdType id) : _id(static_cast<IdType>(id)) {};
	operator int() const { return _id; }
	bool operator ==(const int& other) const { return _id == other; }
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

extern MeshLookup<TreeInfo> treeMeshLookup;
extern MeshLookup<MobileStaticInfo> mobileStaticMeshLookup;
extern MeshLookup<MobileObjectInfo> mobileObjectMeshLookup;
extern MeshLookup<AbodeInfo> abodeMeshLookup;
extern MeshLookup<VillagerType, VillagerTypeId> villagerMeshLookup;
extern MeshLookup<FeatureInfo> featureMeshLookup;
} // namespace openblack
