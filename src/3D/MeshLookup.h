/* openblack - A reimplementation of Lionhead's Black & White.
 *
 * openblack is the legal property of its developers, whose names
 * can be found in the AUTHORS.md file distributed with this source
 * distribution.
 *
 * openblack is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * openblack is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with openblack. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <AllMeshes.h>
#include <Entities/Components/AnimatedStatic.h>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace openblack
{
enum class TreeInfo;

template <typename T>
class MeshLookup
{
public:
	MeshLookup(std::initializer_list<typename std::unordered_map<T, MeshId>::value_type> init): lookup(init) {}
	MeshId operator[](T key)
	{
		auto item = lookup.find(key);
		auto meshId = MeshId::Dummy;

		if (item == lookup.end())
		{
			spdlog::error("Missing mesh entry for type, \"{}\" with key \"{}\".", typeid(key).name(), static_cast<int>(key));
			return meshId;
		}

		return item->second;
	}

private:
	std::unordered_map<T, MeshId> lookup;
};

extern MeshLookup<TreeInfo> treeMeshLookup;
extern MeshLookup<MobileStaticInfo> mobileStaticMeshLookup;
extern MeshLookup<MobileObjectInfo> mobileObjectMeshLookup;
extern MeshLookup<AbodeInfo> abodeMeshLookup;
extern MeshLookup<FeatureInfo> featureMeshLookup;
} // namespace openblack
