/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CloudSystem.h"

#include <vector>

#include "3D/Clouds.h"
#include "Common/GameRandom.h"
#include "ECS/Archetypes/CloudArchetype.h"
#include "ECS/Components/Cloud.h"
#include "ECS/Components/Mist.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

void CloudSystem::Reset()
{
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> old;
	registry.Each<const Cloud>([&old](entt::entity entity, const Cloud& /*unused*/) { old.push_back(entity); });
	for (const auto entity : old)
	{
		registry.Destroy(entity);
	}
	// The game lays them out from the C runtime's numbers
	auto& random = Locator::gameRandom::value();
	for (const auto& layout : clouds::MakeLayout([&random](float low, float high) { return random.CrtRandom(low, high); }))
	{
		archetypes::CloudArchetype::Create(layout);
	}
}

void CloudSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	Locator::entitiesRegistry::value().Each<Cloud, Mist, Transform>([gameTime](Cloud& cloud, Mist& mist, Transform& transform) {
		cloud.track = clouds::Move(cloud.track, cloud.pinned, gameTime.count());
		transform.position = clouds::WorldPosition(cloud.track);
		mist.colour = static_cast<uint32_t>(clouds::EdgeAlpha(cloud.track, cloud.pinned)) << 24u;
	});
}
