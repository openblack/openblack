/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Food and wood the miracles pour. What is poured at a point goes to the stores and piles of it in the map cell under
// the point and then in the eight cells about it, spiralling out, met in each cell in the order the cell keeps them: to
// each that takes it and is near enough, as much as it will take. What is left makes a new pile, unless the point is in the
// water. A pile thuds as it is made or added to; a new pile rises out of the ground, with its people coming to look at it, and
// a power-up's food sparkles over it for good.

#define LOCATOR_IMPLEMENTATIONS

#include <cmath>

#include <chrono>
#include <vector>

#include <LNDFile.h>
#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/norm.hpp>

#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Audio/AudioManagerInterface.h"
#include "ECS/Archetypes/PotArchetype.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/MagicPile.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/ResourcePile.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Transform.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/ResourcePiles.h"
#include "MagicSystem.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace piles = openblack::magic::piles;

namespace
{
/// The game's map is 512 cells each way
constexpr int k_MapCells = 512;

const GPotInfo& PotInfoOf(PotInfo type)
{
	return Locator::infoConstants::value().pot.at(static_cast<size_t>(type));
}

/// The half width and depth of an entity's model, unscaled
glm::vec2 HalfExtentOf(const Mesh& mesh)
{
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh.id))
	{
		return glm::vec2(0.0f);
	}
	const auto size = meshes.Handle(mesh.id)->GetBoundingBox().Size();
	return glm::vec2(size.x, size.z) * 0.5f;
}

/// The thud of a pile, where it is
void Thud(ResourceType type, uint32_t amount, glm::vec3 position)
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	const auto tick = static_cast<uint32_t>(
	    std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
	const auto sample = piles::PileSoundSample(type, amount, tick);
	Locator::audio::value().PlaySoundEffect(entt::hashed_string(fmt::format("InGame.sad/{}", sample).c_str()).value(),
	                                        position);
}

/// A pile takes what it will of what it is given, with its thud
uint32_t AddToPile(entt::entity pile, ResourceType type, uint32_t given)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& pot = registry.Get<Pot>(pile);
	const auto& info = PotInfoOf(pot.type);
	if (piles::Thuds(pot.type))
	{
		Thud(type, given, registry.Get<const Transform>(pile).position);
	}
	const auto taken = piles::AmountTaken(pot.amount, given, pot.maxAmount, piles::IsCapped(info.nextPotForResource));
	pot.amount += taken;
	return taken;
}

/// The pile a storage pit keeps its food or wood in, made empty where the pit's model says if it has none
entt::entity StorePile(entt::entity pit, entt::entity& slot, PotInfo type, size_t place)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (registry.Valid(slot))
	{
		return slot;
	}
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto& mesh = registry.Get<const Mesh>(pit);
	if (!meshes.Contains(mesh.id))
	{
		return entt::null;
	}
	const auto& metrics = meshes.Handle(mesh.id)->GetExtraMetrics();
	if (place >= metrics.size())
	{
		return entt::null;
	}
	// Turned as the pit is, which is turned about the vertical
	const auto& transform = registry.Get<const Transform>(pit);
	const float yaw = std::atan2(transform.rotation[0].z, transform.rotation[0].x);
	const auto offset = glm::vec3(glm::mat4(transform.rotation) * metrics.at(place)[3]);
	slot = ecs::archetypes::PotArchetype::CreateEmpty(transform.position + offset, yaw, type);
	return slot;
}
} // namespace

bool GameMagicWorld::IsWater(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return true;
	}
	const auto cell = piles::CellOf({point.x, point.z});
	if (cell.x >= k_MapCells || cell.y >= k_MapCells)
	{
		return true;
	}
	const auto* landCell = Locator::terrainSystem::value().FindCell(glm::u16vec2(cell));
	return landCell == nullptr || landCell->properties.hasWater != 0;
}

uint32_t GameMagicWorld::AddToStore(entt::entity store, ResourceType type, uint32_t given)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* abode = registry.TryGet<Abode>(store);
	auto* pit = registry.TryGet<StoragePit>(store);
	if (abode == nullptr || pit == nullptr)
	{
		return 0;
	}
	const auto& abodeInfo = Locator::infoConstants::value().abode.at(static_cast<size_t>(abode->type));
	uint32_t left = given;
	if (type == ResourceType::Food)
	{
		// Its one pile of food
		const auto pile = StorePile(store, pit->foodPile, abodeInfo.potForResourceFood, pit->woodPiles.size());
		if (pile != entt::null && left > 0)
		{
			left -= AddToPile(pile, type, left);
		}
	}
	else
	{
		// Its piles of wood, each in turn
		size_t place = 0;
		for (auto potType = abodeInfo.potForResourceWood;
		     potType != PotInfo::_COUNT && place < pit->woodPiles.size() && left > 0;
		     potType = PotInfoOf(potType).nextPotForResource, ++place)
		{
			const auto pile = StorePile(store, pit->woodPiles.at(place), potType, place);
			if (pile != entt::null)
			{
				left -= AddToPile(pile, type, left);
			}
		}
	}
	const auto taken = given - left;
	// The town's count of what the pit holds keeps up with its piles
	(type == ResourceType::Food ? abode->foodAmount : abode->woodAmount) += taken;
	return taken;
}

bool GameMagicWorld::AddResource(ResourceType type, glm::vec3 point, uint32_t amount, bool speedUp, PlayerNames player)
{
	if (amount == 0 || (type != ResourceType::Food && type != ResourceType::Wood) || !InBounds(point))
	{
		return false;
	}
	auto& registry = Locator::entitiesRegistry::value();
	const glm::vec2 at {point.x, point.z};
	const auto near = [&](entt::entity entity, float multiplier) {
		const auto& transform = registry.Get<const Transform>(entity);
		float radius = piles::RadiusOnGround(HalfExtentOf(registry.Get<const Mesh>(entity)), transform.scale.x);
		// A pile of food reaches as far as it is raised
		if (const auto* pot = registry.TryGet<const Pot>(entity); pot != nullptr && type == ResourceType::Food)
		{
			radius *= piles::ProportionRaised(type, pot->amount, PotInfoOf(pot->type).maxAmountInPot);
		}
		return glm::distance(at, glm::vec2(transform.position.x, transform.position.z)) <= radius * multiplier;
	};

	uint32_t left = amount;
	const auto origin = map_coords::CellOf(at);
	for (const auto& step : piles::k_SearchCells)
	{
		if (left == 0)
		{
			break;
		}
		// Everything in the cell, as the cell keeps it: whatever stores this, or is a pile of it, takes what it will
		for (const auto entity : Locator::entitiesMap::value().GetAllInCell(origin + step))
		{
			if (left == 0)
			{
				break;
			}
			if (!registry.Valid(entity))
			{
				continue;
			}
			if (registry.AllOf<StoragePit, Abode>(entity))
			{
				if (near(entity, piles::k_StoreReachMultiplier))
				{
					left -= AddToStore(entity, type, left);
				}
			}
			else if (const auto* pot = registry.TryGet<const Pot>(entity);
			         pot != nullptr && registry.AllOf<ResourcePile>(entity) && PotInfoOf(pot->type).resourceType == type)
			{
				if (near(entity, piles::k_PotReachMultiplier))
				{
					left -= AddToPile(entity, type, left);
				}
			}
		}
	}
	if (left == 0 || IsWater(point))
	{
		return left != amount;
	}

	// What is left makes a new pile, which thuds as it lands
	const auto potType = type == ResourceType::Food ? PotInfo::MagicFood : PotInfo::MagicWood;
	point.y = LandHeight(at);
	const auto created = ecs::archetypes::PotArchetype::Create(point, 0.0f, potType, static_cast<int32_t>(left));
	if (created == entt::null)
	{
		return left != amount;
	}
	registry.Get<Transform>(created).scale =
	    glm::vec3(type == ResourceType::Food ? piles::k_MagicFoodScale : piles::k_MagicWoodScale);
	registry.Assign<MagicPile>(created, type, player);
	Thud(type, left, point);
	// A power-up's food speeds up the people who take from the new pile, and sparkles over it for as long as it lasts
	auto& pile = registry.Get<ResourcePile>(created);
	if (speedUp && type == ResourceType::Food)
	{
		pile.speedUp = true;
		if (Locator::particleSystem::has_value())
		{
			pile.speedUpVisual =
			    Locator::particleSystem::value().StartSpotVisual(SpotVisualType::PilefoodSpeedup, point, -1, created, 1.0f);
		}
	}
	// The people come to look at it
	const auto reaction = PotInfoOf(potType).associatedReaction;
	if (reaction != Reaction::None && Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().Create({.initiator = created,
		                                         .type = reaction,
		                                         .player = player,
		                                         .position = point,
		                                         .impressiveValue = PotInfoOf(potType).impressiveValue,
		                                         .power = 1.0f,
		                                         .magicType = MagicType::None,
		                                         .casterCreature = entt::null});
	}
	return true;
}
