/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ObjectMeasures.h"

#include <algorithm>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "Creature/CreatureCastMoves.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
namespace cast_moves = openblack::creature_cast_moves;

/// How far a species' bones reach from its middle, in the first frame of its first animation on its base model
float BoneReachOf(CreatureType species)
{
	if (!Locator::resources::has_value())
	{
		return 0.0f;
	}
	auto& resources = Locator::resources::value();
	const auto& meshes = resources.GetMeshes();
	const auto& rigs = resources.GetCreatureRigs();
	const auto meshId = creature::GetIdFromType(species, creature::CreatureBody::Appearance::Base);
	const auto rigId = creature::GetRigId(species);
	if (!meshes.Contains(meshId) || !rigs.Contains(rigId))
	{
		return 0.0f;
	}
	const auto mesh = meshes.Handle(meshId);
	const auto* animation = rigs.Handle(rigId)->GetAnimation(creature::CreatureRig::Mesh::Base, 0);
	if (animation == nullptr)
	{
		return 0.0f;
	}
	const auto skeleton = skeletal_animation::Skeleton::FromRestMatrices(mesh->GetBoneParents(), mesh->GetBoneMatrices());
	return cast_moves::BoneReach(*animation, skeleton);
}

/// A model's half width, height and length, times the thing's scale
std::optional<glm::vec3> HalfExtentsOf(const ecs::Registry& registry, entt::entity entity)
{
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	const auto* transform = registry.TryGet<const Transform>(entity);
	if (mesh == nullptr || transform == nullptr || !Locator::resources::has_value())
	{
		return std::nullopt;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh->id))
	{
		return std::nullopt;
	}
	const auto& box = meshes.Handle(mesh->id)->GetBoundingBox();
	return (box.maxima - box.minima) * 0.5f * transform->scale.x;
}
} // namespace

float object_measures::TwoDRadius(const Registry& registry, entt::entity entity)
{
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		const auto* transform = registry.TryGet<const Transform>(entity);
		return cast_moves::CreatureRadius(transform != nullptr ? transform->scale.x : 0.0f, BoneReachOf(creature->species));
	}
	if (registry.AllOf<Field>(entity))
	{
		return cast_moves::k_FieldRadius;
	}
	const auto half = HalfExtentsOf(registry, entity);
	return half.has_value() ? std::max(half->x, half->z) : 0.0f;
}

float object_measures::Height(const Registry& registry, entt::entity entity)
{
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		return cast_moves::k_HeightOfSizeOne * creature->size;
	}
	const auto half = HalfExtentsOf(registry, entity);
	return half.has_value() ? 2.0f * half->y : 0.0f;
}

float object_measures::RoutePlanRadius(const Registry& registry, entt::entity entity, entt::entity creature)
{
	return cast_moves::RoutePlanRadius(TwoDRadius(registry, entity), Height(registry, entity), registry.AllOf<Tree>(entity),
	                                   Height(registry, creature), TwoDRadius(registry, creature));
}

bool object_measures::WalkedUpToItself(const Registry& registry, entt::entity entity)
{
	return registry.AnyOf<Abode, Field, Feature, MobileStatic, Tree>(entity);
}

std::optional<glm::vec3> object_measures::PositionOf(const Registry& registry, entt::entity entity)
{
	if (!registry.Valid(entity))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(entity);
	return transform != nullptr ? std::optional(transform->position) : std::nullopt;
}
