/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureScars.h"

#include <glm/geometric.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "Common/GameRandom.h"
#include "Creature/CreatureMarks.h"
#include "Creature/CreatureRig.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/PosedModel.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureSkinSystemInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;

std::optional<glm::vec3> creature_scars::GroinOf(entt::entity creature)
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::resources::has_value())
	{
		return std::nullopt;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* animation = registry.TryGet<const CreatureAnimation>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	if (body == nullptr || animation == nullptr || transform == nullptr)
	{
		return std::nullopt;
	}
	const auto rigId = creature::GetRigId(body->species);
	if (!rigs.Contains(rigId) || !rigs.Handle(rigId)->actionPoints.has_value())
	{
		return std::nullopt;
	}
	const auto bone = rigs.Handle(rigId)->actionPoints->groin;
	if (bone >= animation->boneMatrices.size())
	{
		return std::nullopt;
	}
	const auto placement = creature::PlacementMatrix(transform->position, transform->rotation, transform->scale);
	return glm::vec3(creature::PosedBone(bone, animation->boneMatrices, placement)[3]);
}

void creature_scars::MarkAlong(entt::entity creature, glm::vec3 from, glm::vec3 groin, uint8_t kind, uint8_t column)
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::resources::has_value() ||
	    !Locator::creatureSkinSystem::has_value())
	{
		return;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* mesh = registry.TryGet<const Mesh>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	auto& meshes = Locator::resources::value().GetMeshes();
	if (mesh == nullptr || transform == nullptr || !meshes.Contains(mesh->id))
	{
		return;
	}
	const auto placement = creature::PlacementMatrix(transform->position, transform->rotation, transform->scale);
	const auto hit = posed_model::NearestSkinHit(registry, creature, *meshes.Handle(mesh->id), placement, from,
	                                             glm::normalize(groin - from));
	if (!hit.has_value() || !hit->skin.has_value())
	{
		return;
	}
	const auto texel = creature_marks::scar::TexelAt(hit->uvs, hit->hit.s, hit->hit.t);
	Locator::creatureSkinSystem::value().AddWound(
	    creature,
	    {.u = texel.x, .v = texel.y, .skin = static_cast<uint8_t>(*hit->skin), .age = 0, .type = kind, .column = column});
}

void creature_scars::BurnOnCatching(entt::entity creature)
{
	if (!Locator::gameRandom::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	const auto* body = Locator::entitiesRegistry::value().TryGet<const Creature>(creature);
	const auto groin = GroinOf(creature);
	if (body == nullptr || !groin.has_value())
	{
		return;
	}
	auto& random = Locator::gameRandom::value();
	const float reach = body->size * creature_marks::scar::k_BurnReachPerSize;
	for (int32_t i = 0; i < creature_marks::scar::k_CatchingBurnTries; ++i)
	{
		// Each try's point is drawn across, then up, then along
		glm::vec3 from = *groin;
		from.x += random.GameFloatRand(reach * 0.25f) - reach * 0.125f;
		from.y += random.GameFloatRand(reach) - reach * 0.5f;
		from.z += random.GameFloatRand(reach * 0.25f) - reach * 0.125f;
		const auto direction = *groin - from;
		if (!(glm::dot(direction, direction) > creature_marks::scar::k_LeastReachSquared))
		{
			continue;
		}
		const auto kind = creature_marks::scar::BurnKind(random.GameRand(3));
		const auto column = creature_marks::scar::CatchingBurnColumn(random.GameRand(8));
		MarkAlong(creature, from, *groin, kind, column);
	}
}
