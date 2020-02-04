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

template <typename T, typename K = std::hash<T>>
class MeshLookup
{
public:
	MeshLookup(std::initializer_list<typename std::unordered_map<T, MeshId, K>::value_type> init): lookup(init) {}

	MeshId operator[](T key)
	{
		auto item = lookup.find(key);
		auto meshId = MeshId::Dummy;

		if (item == lookup.end())
		{
			spdlog::error("Unknown item. Using Dummy mesh.");
			return meshId;
		}

		return item->second;
	}

private:
	std::unordered_map<T, MeshId, K> lookup;
};

extern MeshLookup<TreeInfo> treeMeshLookup;
extern MeshLookup<MobileStaticInfo> mobileStaticMeshLookup;
extern MeshLookup<MobileObjectInfo> mobileObjectMeshLookup;
extern MeshLookup<AbodeInfo> abodeMeshLookup;
extern MeshLookup<VillagerType, VillagerTypeId> villagerMeshLookup;
extern MeshLookup<FeatureInfo> featureMeshLookup;
} // namespace openblack
