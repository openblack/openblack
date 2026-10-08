/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The points of an object's model that glints sparkle on: those of a miracle's seed shown in a globe, every point of
// every part of its model in order, placed as the seed was last drawn

#define LOCATOR_IMPLEMENTATIONS

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Magic/MiracleVisuals.h"
#include "ParticleSystem.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The model of the seed a globe shows, if it is loaded
const graphics::L3DMesh* SeedModelOf(const OneOffSpellSeed& globe)
{
	if (!Locator::infoConstants::has_value() || !Locator::resources::has_value())
	{
		return nullptr;
	}
	const auto& seed = magic::GetSpellSeedInfo(Locator::infoConstants::value(), globe.seedType);
	auto& meshes = Locator::resources::value().GetMeshes();
	const auto id = resources::HashIdentifier(seed.mesh);
	return meshes.Contains(id) ? &*meshes.Handle(id) : nullptr;
}

const OneOffSpellSeed* GlobeOf(entt::entity object)
{
	if (!Locator::entitiesRegistry::has_value() || !Locator::entitiesRegistry::value().Valid(object))
	{
		return nullptr;
	}
	return Locator::entitiesRegistry::value().TryGet<const OneOffSpellSeed>(object);
}
} // namespace

uint32_t GameParticleWorld::TargetPointCount(entt::entity object) const
{
	const auto* globe = GlobeOf(object);
	const auto* model = globe != nullptr ? SeedModelOf(*globe) : nullptr;
	if (model == nullptr)
	{
		return 0;
	}
	size_t count = 0;
	for (const auto& part : model->GetSubMeshes())
	{
		count += part->GetSurfacePoints().size();
	}
	return static_cast<uint32_t>(count);
}

std::optional<glm::vec3> GameParticleWorld::TargetPoint(entt::entity object, uint32_t index) const
{
	const auto* globe = GlobeOf(object);
	const auto* model = globe != nullptr ? SeedModelOf(*globe) : nullptr;
	if (model == nullptr || !globe->seedPlacement.has_value())
	{
		return std::nullopt;
	}
	size_t remaining = index;
	for (const auto& part : model->GetSubMeshes())
	{
		const auto& points = part->GetSurfacePoints();
		if (remaining < points.size())
		{
			return glm::vec3(*globe->seedPlacement * glm::vec4(points[remaining].position, 1.0f));
		}
		remaining -= points.size();
	}
	return std::nullopt;
}

float GameParticleWorld::TargetScale(entt::entity object) const
{
	const auto* globe = GlobeOf(object);
	const auto* transform = globe != nullptr ? Locator::entitiesRegistry::value().TryGet<const Transform>(object) : nullptr;
	// The seed is drawn at six tenths of the globe's scale
	return transform != nullptr ? transform->scale.x * magic::visuals::k_GlobeSeedScale : 1.0f;
}
