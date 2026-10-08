/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "PosedModel.h"

#include <algorithm>
#include <array>

#include <glm/geometric.hpp>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "Creature/CreatureMorph.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/VillagerPose.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;

namespace
{
float HeightAt(glm::vec2 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
}
} // namespace

std::span<const glm::mat4> posed_model::BonesOf(const ecs::Registry& registry, entt::entity entity,
                                                const graphics::L3DMesh& mesh)
{
	const auto& rest = mesh.GetBoneMatrices();
	if (const auto* pose = registry.TryGet<const AnimalPose>(entity); pose != nullptr && pose->bones.size() == rest.size())
	{
		return pose->bones;
	}
	if (const auto* pose = registry.TryGet<const VillagerPose>(entity); pose != nullptr && pose->bones.size() == rest.size())
	{
		return pose->bones;
	}
	if (const auto* animation = registry.TryGet<const CreatureAnimation>(entity);
	    animation != nullptr && animation->boneMatrices.size() == rest.size())
	{
		return animation->boneMatrices;
	}
	return rest;
}

void posed_model::Place(const ecs::Registry& registry, entt::entity entity, const graphics::L3DMesh& mesh, size_t index,
                        const glm::mat4& model, std::vector<glm::vec3>& corners)
{
	const auto& subMesh = *mesh.GetSubMeshes()[index];
	const auto& geometry = subMesh.GetBodyGeometry();
	corners.clear();
	corners.reserve(geometry.positions.size());

	// A creature's body is its base mesh moved towards the meshes of how evil or good, thin or fat and weak or strong
	// it is drawn
	std::array<const graphics::L3DSubMesh*, 3> morphs {};
	const creature_morph::Morph* morph = nullptr;
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		if (const auto* shape = registry.TryGet<const CreatureMorph>(entity))
		{
			auto& meshes = Locator::resources::value().GetMeshes();
			const auto ids = creature_morph::MeshesOf(creature->species, shape->drawn,
			                                          [&meshes](entt::id_type id) { return meshes.Contains(id); });
			const auto subMeshOf = [&meshes, index](entt::id_type id) -> const graphics::L3DSubMesh* {
				const auto& subs = meshes.Handle(id)->GetSubMeshes();
				return index < subs.size() ? subs[index].get() : nullptr;
			};
			morphs = {subMeshOf(ids.evilGood), subMeshOf(ids.thinFat), subMeshOf(ids.weakStrong)};
			morph = &shape->drawn;
		}
	}
	const auto bones = posed_model::BonesOf(registry, entity, mesh);
	const bool followsLand = registry.AllOf<MorphWithTerrain>(entity);
	const glm::vec3 origin(model[3]);
	const glm::vec3 up(model[1]);
	const float scale = glm::length(glm::vec3(model[0]));
	for (size_t i = 0; i < geometry.positions.size(); ++i)
	{
		auto position = geometry.positions[i];
		if (morph != nullptr)
		{
			const auto partner = [&](const graphics::L3DSubMesh* other) {
				const auto& positions = other != nullptr ? other->GetBodyGeometry().positions : geometry.positions;
				return i < positions.size() ? positions[i] : geometry.positions[i];
			};
			position = creature_morph::Blend(position, partner(morphs[0]), partner(morphs[1]), partner(morphs[2]), *morph);
		}
		glm::vec4 local(position, 1.0f);
		if (i < geometry.bones.size() && geometry.bones[i] != graphics::L3DSubMesh::k_NoBone &&
		    geometry.bones[i] < bones.size())
		{
			local = bones[geometry.bones[i]] * local;
		}
		auto world = glm::vec3(model * local);
		if (followsLand)
		{
			// Each corner moves along the model's up by how much higher the land is under it than under its origin
			const float rise = HeightAt({world.x, world.z}) - HeightAt({origin.x, origin.z});
			if (up.x == 0.0f && up.z == 0.0f)
			{
				world.y += rise;
			}
			else
			{
				world += up * (rise / scale);
			}
		}
		corners.push_back(world);
	}
}

bool posed_model::IsDrawn(const graphics::L3DSubMesh& subMesh)
{
	const auto flags = subMesh.GetFlags();
	return (flags.lodMask & 1) != 0 && flags.status == 0;
}

std::optional<posed_model::SkinHit> posed_model::NearestSkinHit(const ecs::Registry& registry, entt::entity entity,
                                                                const graphics::L3DMesh& mesh, const glm::mat4& model,
                                                                glm::vec3 origin, glm::vec3 direction)
{
	std::optional<SkinHit> nearest;
	std::vector<glm::vec3> placed;
	const auto& skinOrder = mesh.GetSkinOrder();
	for (size_t s = 0; s < mesh.GetSubMeshes().size(); ++s)
	{
		const auto& subMesh = *mesh.GetSubMeshes()[s];
		// The submeshes of the nearest level of detail
		if ((subMesh.GetFlags().lodMask & 1) == 0)
		{
			continue;
		}
		Place(registry, entity, mesh, s, model, placed);
		const auto& geometry = subMesh.GetBodyGeometry();
		const auto hit = screen_pick::NearestIntersection(placed, geometry.indices, origin, direction, false);
		if (!hit.has_value() || (nearest.has_value() && !(hit->distance < nearest->hit.distance)))
		{
			continue;
		}
		const auto uvAt = [&geometry, &hit](uint32_t corner) {
			const auto index = geometry.indices[hit->firstIndex + corner];
			return index < geometry.uvs.size() ? geometry.uvs[index] : glm::vec2(0.0f);
		};
		// The skin of the primitive the triangle belongs to, counted among the model's skins
		std::optional<uint32_t> skin;
		for (const auto& primitive : subMesh.GetPrimitives())
		{
			if (hit->firstIndex >= primitive.indicesOffset &&
			    hit->firstIndex < primitive.indicesOffset + primitive.indicesCount)
			{
				const auto found = std::ranges::find(skinOrder, primitive.skinID);
				if (found != skinOrder.end())
				{
					skin = static_cast<uint32_t>(std::distance(skinOrder.begin(), found));
				}
				break;
			}
		}
		nearest = SkinHit {.hit = *hit, .uvs = {uvAt(0), uvAt(1), uvAt(2)}, .skin = skin};
	}
	return nearest;
}
