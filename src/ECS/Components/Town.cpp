/******************************************************************************
 * Copyright (c) 2018-2023 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Town.h"

#include <array>
#include <functional>
#include <random>

#include <glm/common.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <glm/vec3.hpp>

#include "3D/L3DMesh.h"
#include "Abode.h"
#include "Common/Spiral.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "Field.h"
#include "Fixed.h"
#include "Game.h"
#include "Locator.h"
#include "Mesh.h"
#include "Mobile.h"
#include "Resources/ResourcesInterface.h"
#include "Transform.h"

using namespace openblack;
using namespace openblack::ecs::components;

float GetDistanceSquaredInMetres(const glm::vec3& p1, const glm::vec3& p2)
{
	auto d = (p2 - p1);
	return dot(d, d);
}

bool CheckForClearArea(glm::vec3 point, float radius, std::function<bool(entt::entity)> checkCallback)
{
	// struct MapCoords local_c;
	// __ct__9MapCoordsFP9MapCoords(&local_c, edx, param_1);

	int index = 1;
	int subIndex = 1;

	const auto& map = Locator::entitiesMap::value();
	const auto& registry = Locator::entitiesRegistry::value();
	const auto& meshes = Locator::resources::value().GetMeshes();

	const int cellSize = utils::spiral::GetMapCellSpiralSizeFromRadius(radius);
	for (int i = 0; i < cellSize; ++i)
	{
		// bool isFixed = true;
		std::array<std::reference_wrapper<const std::unordered_set<entt::entity>>, 2> entities = {
		    map.GetFixedInGridCell(point),
		    map.GetMobileInGridCell(point),
		};
		for (size_t j = 0; j < entities.size(); ++j)
		{
			if (j > 0)
			{
				// isFixed = false;
			}
			for (const auto& entity : entities.at(j).get())
			{
				const auto& transform = registry.Get<Transform>(entity);
				auto diff = glm::xz(point - transform.position);
				const float distance2 = glm::dot(diff, diff);
				float objectRadius;
				if (registry.AllOf<Field>(entity) /*|| registry.Has<FishFarm>(entity)*/)
				{
					objectRadius = Field::radius2D;
				}
				// TODO(bwrsandman): Fish farm has same fixed radius as Field (5)
				// else if (registry.Has<FishFarm>(entity))
				// {
				// 	objectRadius = FishFarm::radius2D;
				// }
				// TODO(bwrsandman): Magic teleport has a radius of 6
				// else if (registry.Has<MagicTeleport>(entity))
				// {
				// 	objectRadius = MagicTeleport::radius2D;
				// }
				// else if (registry.Has<Creature>(entity))
				// {
				// 	this->physical->creature_3d->field_0x5228;
				// 	objectRadius= 0.0f;
				// }

				// Object.Get2DRadius
				else
				{
					[[maybe_unused]] const bool any = registry.AnyOf<Fixed, Mobile>(entity);
					assert(any);
					assert(false); // TODO
					const auto& [meshTransform, mesh] = registry.Get<Transform, Mesh>(entity);
					const auto& l3dMesh = meshes.Handle(mesh.id);
					auto submeshId = (l3dMesh->GetNumSubMeshes() + mesh.bbSubmeshId) % l3dMesh->GetNumSubMeshes();
					auto bb = l3dMesh->GetSubMeshes()[submeshId]->GetBoundingBox();
					objectRadius = glm::compMax(meshTransform.scale * bb.Size());
				}
				// TODO(bwrsandman): A food pile is scaled by amount
				// if (registry.Has<PileFood>(entity))
				// {
				// 	objectRadius *= registry.Get<PileFood>(entity).GetProportionRaised();
				// }
				if (distance2 - objectRadius * objectRadius < radius * radius && checkCallback(entity))
				{
					return false;
				}
			}
		}
		point = ecs::MapInterface::PointGridCellAdd(point, utils::spiral::Spiral(index, subIndex));
	}

	return true;
}

bool FindClearArea(const glm::vec3& point, float radius1, float incrementSize, float radius2,
                   std::function<bool(entt::entity)> checkCallback)
{
	glm::vec3 workingPoint = point;
	int index = 1;
	int subIndex = 1;

	const int increments = utils::spiral::GetIncrementSpiralSizeFromRadius(radius1, incrementSize);
	for (int i = 0; i < increments; ++i)
	{
		if (CheckForClearArea(workingPoint, radius2, checkCallback))
		{
			return true;
		}
		utils::spiral::SpiralIncrement(workingPoint, index, subIndex, incrementSize);
	}
	return false;
}

const glm::vec3& Town::GetCongregationPos()
{
	if (cachedCongregationPosition != glm::zero<glm::vec3>())
	{
		return cachedCongregationPosition;
	}

	const auto& registry = Locator::entitiesRegistry::value();

	bool checkPlanned = false; // TODO: rename to searchMore

	// If there are not abodes, then we can skip checking fields and planned
	registry.Each<const Abode>([this, &checkPlanned](auto abode) {
		if (abode.townId == id)
		{
			checkPlanned = false;
		}
	});

	std::array<entt::entity, 100> fieldsAndPlanned;
	uint32_t fieldsAndPlannedCount = 0; // TODO: rename to something about circular buffer
	uint32_t abodeCount = 0;            // TODO: Rename to fieldsAndPlannedTotal
	// First try to find at least three fields to get a centre point
	if (!checkPlanned)
	{
		registry.Each<const Abode, const Field>([this, &fieldsAndPlanned, &fieldsAndPlannedCount,
		                                         &abodeCount](entt::entity entity, auto abode, [[maybe_unused]] auto field) {
			if (abode.townId != id)
			{
				return;
			}
			fieldsAndPlanned.at(fieldsAndPlannedCount) = entity;
			++fieldsAndPlannedCount;
			if (fieldsAndPlannedCount == fieldsAndPlanned.size())
			{
				fieldsAndPlannedCount = 0;
			}
			++abodeCount;
		});
		if (abodeCount < 3)
		{
			checkPlanned = true;
		}
	}

	// TODO: If there aren't enough, throw planned structures into the mix
	// if (!checkPlanned)
	// {
	// 	registry.Each<const PlannedMultiMapFixed>(
	// 	    [this, &fieldsAndPlanned, &fieldsAndPlannedCount, &abodeCount](entt::entity entity, auto planned) {
	// 		    if (planned.townId != id)
	// 		    {
	// 			    return;
	// 		    }
	// 		    fieldsAndPlanned[fieldsAndPlannedCount] = entity;
	// 		    ++fieldsAndPlannedCount;
	// 		    if (fieldsAndPlannedCount == fieldsAndPlanned.size())
	// 		    {
	// 			    fieldsAndPlannedCount = 0;
	// 		    }
	// 		    ++abodeCount;
	// 	    });
	// }

	bool useAveragePosition = abodeCount > 1;

	uint32_t thingIndex = 0;
	if (useAveragePosition)
	{
		// Find centre of these entities
		glm::vec3 averagePosition;
		for (uint32_t i = 0; i < abodeCount; ++i)
		{
			auto thing = fieldsAndPlanned.at(thingIndex);
			++thingIndex;
			if (thingIndex == 100)
			{
				thingIndex = 0;
			}

			averagePosition += registry.Get<Transform>(thing).position;
		}
		averagePosition /= abodeCount;

		// Make sure this area is cleared
		if (FindClearArea(averagePosition, 130.0f, 3.0f, 10.0f, [&registry](entt::entity entity) {
			    // TODO(bwrsandman): this should be
			    // Object && !Tree to be technically
			    // correct
			    return registry.AnyOf<Abode>(entity);
		    }))
		{
			cachedCongregationPosition = averagePosition;
		}
		else
		{
			useAveragePosition = false;
		}
	}

	// Only one object
	if (!useAveragePosition)
	{
		int iVar2 = fieldsAndPlannedCount - thingIndex;
		if (thingIndex > fieldsAndPlannedCount)
		{
			iVar2 += 100;
		}

		auto thing = (iVar2 != 0) ? fieldsAndPlanned.at(thingIndex) : registry.ToEntity(*this);
		cachedCongregationPosition = registry.Get<Transform>(thing).position;

		// Random generators for a point on the region of an Annulus (ring)
		thread_local std::default_random_engine generator;
		thread_local std::uniform_real_distribution headingDistribution(2.0f * glm::pi<float>());
		thread_local std::uniform_real_distribution radiusDistribution(10.0f, 20.0f);

		// From heading angle and radius, get point as an random offset around the thing
		auto offset = glm::zero<glm::vec3>();
		const float heading = headingDistribution(generator);
		offset.x = glm::cos(heading);
		offset.z = glm::sin(heading);
		offset *= radiusDistribution(generator);

		cachedCongregationPosition += offset;
	}

	return cachedCongregationPosition;
}
