/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "AbodeArchetype.h"

#include <glm/gtx/euler_angles.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TownSystemInterface.h"
#include "Game.h"
#include "Locator.h"
#include "Resources/MeshId.h"
#include "Utils.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

entt::entity AbodeArchetype::Create(uint32_t townId, const glm::vec3& position, AbodeInfo type, float yAngleRadians,
                                    float scale, uint32_t foodAmount, uint32_t woodAmount)
{
	auto* game = Game::Instance();
	auto& registry = game->GetEntityRegistry();

	// If there is no town, assign to closest
	if (registry.Context().towns.find(townId) == registry.Context().towns.end())
	{
		SPDLOG_LOGGER_WARN(spdlog::get("scripting"), "Function {} has invalid Town ({}).", __func__, townId);
		const auto town = Locator::townSystem::ref().FindClosestTown(position);
		if (town != entt::null)
		{
			townId = registry.Get<Town>(town).id;
		}
		else
		{
			SPDLOG_LOGGER_ERROR(spdlog::get("scripting"), "Function {} has could not find closest town.", __func__, townId);
			return entt::null;
		}
	}

	const auto entity = registry.Create();

	const auto& info = game->GetInfoConstants().abode.at(static_cast<size_t>(type));

	const auto& transform =
	    registry.Assign<Transform>(entity, position, glm::mat3(glm::eulerAngleY(-yAngleRadians)), glm::vec3(scale));
	registry.Assign<Abode>(entity, info.abodeNumber, townId, foodAmount, woodAmount);
	auto resourceId = resources::MeshIdToResourceId(info.meshId);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(0));

	// Create Fixed component with a 2d bounding circle
	const auto [point, radius] = GetFixedObstacleBoundingCircle(info.meshId, transform);
	registry.Assign<Fixed>(entity, point, radius);

	return entity;
}
