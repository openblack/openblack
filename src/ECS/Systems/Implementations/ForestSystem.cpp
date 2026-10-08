/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ForestSystem.h"

#include <cmath>

#include <algorithm>
#include <numbers>
#include <optional>
#include <unordered_map>
#include <vector>

#include <LNDFile.h>
#include <glm/geometric.hpp>
#include <glm/gtx/vec_swizzle.hpp>
#include <spdlog/spdlog.h>

#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "Common/GUtilsAngle.h"
#include "Common/GUtilsDistance.h"
#include "Common/GameRandom.h"
#include "Creature/CreatureAudio.h"
#include "ECS/Archetypes/TreeArchetype.h"
#include "ECS/Archetypes/Utils.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/AnimatedStatic.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fixed.h"
#include "ECS/Components/Flowers.h"
#include "ECS/Components/Forest.h"
#include "ECS/Components/ForestMember.h"
#include "ECS/Components/LandForest.h"
#include "ECS/Components/MagicForest.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Spell.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/StoragePit.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/TempleExterior.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/LandForests.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/ScenicForest.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/ResourceStoreSystemInterface.h"
#include "ECS/Systems/SnowSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/ForestRules.h"
#include "Magic/MagicTables.h"
#include "ObjectMeasures.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace forest = openblack::magic::forest;

namespace
{
/// A town's scenic forest reaches this much further than its forests
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// Positions are kept in 1/6553.6 of a metre, as the map keeps them: the spiral's points are cut down to that
constexpr float k_MapUnitsPerMetre = 6553.6f;
/// A tree is added beside another no sooner than this many turns after the last; trying so many turns round it, a
/// sixteenth of a half turn apart, and so many distances at each, from 5 m
constexpr uint32_t k_TreeAddingGapTurns = 40;
constexpr float k_TreeAddingTurns = 32.0f;
constexpr float k_TreeAddingTries = 5.0f;
constexpr float k_TreeAddingStepAngle = std::numbers::pi_v<float> / 16.0f;
constexpr uint32_t k_TreeAddingNearest = 5;
constexpr uint32_t k_TreeAddingDistances = 5;
/// It grows to a full size of this much, up to this much more, from this small
constexpr float k_AddedTreeFullSize = 0.8f;
constexpr float k_AddedTreeFullSizeMore = 0.4f;
constexpr float k_AddedTreeStartSize = 0.1f;
/// The circle round its spot that must be clear of what is fixed there
constexpr float k_AddedTreeClearance = 0.5f;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

/// The cell of the land a point is in, none off the map
std::optional<glm::u16vec2> CellOf(glm::vec3 point)
{
	const auto x = static_cast<int32_t>(std::floor(point.x / LandIslandInterface::k_CellSize));
	const auto z = static_cast<int32_t>(std::floor(point.z / LandIslandInterface::k_CellSize));
	if (x < 0 || z < 0 || x >= LandIslandInterface::k_MapCellsPerSide || z >= LandIslandInterface::k_MapCellsPerSide)
	{
		return std::nullopt;
	}
	return glm::u16vec2(static_cast<uint16_t>(x), static_cast<uint16_t>(z));
}

/// Land a tree may stand on: on the map and not under water
bool IsLand(glm::vec3 point)
{
	if (!Locator::terrainSystem::has_value())
	{
		return false;
	}
	const auto cell = CellOf(point);
	if (!cell.has_value())
	{
		return false;
	}
	const auto* land = Locator::terrainSystem::value().FindCell(*cell);
	return land != nullptr && land->properties.hasWater == 0;
}

/// Something fixed to the land over every cell it covers, as the game marks them: the buildings, fields, features and
/// flowers, the statics that can be picked up such as rocks, the big forests and the citadel. Trees never are.
bool IsMultiCellFixed(const ecs::Registry& registry, entt::entity entity)
{
	return registry.AnyOf<Abode, AnimatedStatic, BigForest, Feature, Field, Flowers, MobileStatic, StoragePit, SpellDispenser,
	                      Temple, TempleExterior>(entity);
}

/// Where a tree may be planted: on land, in a cell no building or other large fixed thing stands in. Trees, even new
/// ones, never stop another.
bool ValidPlaceForTree(glm::vec3 point)
{
	if (!IsLand(point) || !Locator::entitiesMap::has_value())
	{
		return false;
	}
	const auto& registry = EntityRegistry();
	const auto& fixed = Locator::entitiesMap::value().GetFixedInGridCell(point);
	return std::ranges::none_of(fixed, [&registry](entt::entity entity) { return IsMultiCellFixed(registry, entity); });
}

/// Where a tree may be added beside another: anywhere but on land where a small circle round the spot touches
/// something fixed in the spot's cell. In the water, and off the map, is no hindrance.
bool FreeForAddedTree(glm::vec3 point)
{
	if (!Locator::terrainSystem::has_value() || !Locator::entitiesMap::has_value())
	{
		return true;
	}
	const auto cell = CellOf(point);
	if (!cell.has_value())
	{
		return true;
	}
	const auto* land = Locator::terrainSystem::value().FindCell(*cell);
	if (land == nullptr || land->properties.hasWater != 0)
	{
		return true;
	}
	// Each fixed object's obstacle circle stands for its outline here
	const auto& registry = EntityRegistry();
	const auto& fixed = Locator::entitiesMap::value().GetFixedInGridCell(point);
	return std::ranges::none_of(fixed, [&registry, point](entt::entity entity) {
		const auto* obstacle = registry.TryGet<const Fixed>(entity);
		return obstacle != nullptr &&
		       glm::distance(glm::xz(point), obstacle->boundingCenter) < obstacle->boundingRadius + k_AddedTreeClearance;
	});
}

/// The ground's material at a point, as the trees see it: deep snow is snow whatever lies under it
uint16_t MaterialAt(glm::vec3 point)
{
	std::optional<uint16_t> type;
	const auto cell = CellOf(point);
	if (cell.has_value() && Locator::terrainSystem::has_value())
	{
		const auto& land = Locator::terrainSystem::value();
		if (const auto* found = land.FindCell(*cell); found != nullptr)
		{
			if (found->properties.hasWater != 0)
			{
				return static_cast<uint16_t>(TerrainMaterialType::ShallowWater);
			}
			const auto& countries = land.GetCountries();
			const auto types = land.GetMaterialTypes();
			if (found->properties.country < countries.size())
			{
				const auto index = countries[found->properties.country].materials.at(found->altitude).indices[1];
				if (index < types.size())
				{
					type = types[index];
				}
			}
		}
	}
	const float snow = Locator::snowSystem::has_value() ? Locator::snowSystem::value().GetDepth({point.x, point.z}) : 0.0f;
	return creature_audio::TerrainMaterial(type, snow);
}

const GTreeInfo& TreeInfoOf(TreeInfo type)
{
	return Locator::infoConstants::value().tree.at(static_cast<size_t>(type));
}

/// A tree's size changed: it is drawn so, and walked round at its new size
void Resize(entt::entity entity, float scale)
{
	auto& registry = EntityRegistry();
	auto& transform = registry.Get<Transform>(entity);
	transform.scale = glm::vec3(scale);
	if (auto* fixed = registry.TryGet<Fixed>(entity))
	{
		const auto [centre, radius] = ecs::archetypes::GetFixedObstacleBoundingCircle(
		    TreeInfoOf(registry.Get<const Tree>(entity).type).normal, transform);
		fixed->boundingCenter = centre;
		fixed->boundingRadius = radius;
	}
}

void RemoveTree(entt::entity tree)
{
	if (Locator::fireSystem::has_value())
	{
		Locator::fireSystem::value().Forget(tree);
	}
	if (Locator::reactionSystem::has_value())
	{
		Locator::reactionSystem::value().RemoveFrom(tree);
	}
	EntityRegistry().Destroy(tree);
}
} // namespace

uint32_t ForestSystem::Plant(entt::entity spell, Spell& miracle, float tribalPower)
{
	if (!Locator::infoConstants::has_value() || HasTrees(spell))
	{
		return 0;
	}
	const auto& info = Locator::infoConstants::value();
	const auto* trees = magic::GetMagicInfoAs<GMagicForestInfo>(info, miracle.magicType);
	if (trees == nullptr)
	{
		return 0;
	}
	auto& registry = EntityRegistry();
	auto& random = Locator::gameRandom::value();
	// The event that plants it has just been paid for, so it has strength
	const auto count = forest::TreesWeCanAfford(miracle.maxObjectsToCreate, trees->finalNoTrees, 1.0f);
	const auto centre = miracle.castPosition;
	const auto& kinds = info.terrainMaterial.at(std::min<size_t>(MaterialAt(centre), info.terrainMaterial.size() - 1));
	const auto& effect = magic::GetMagicEffectInfo(info, miracle.magicType);

	entt::entity forestEntity = entt::null;
	uint32_t forestId = 0;
	std::vector<entt::entity> planted;
	for (uint32_t i = 0; i < count; ++i)
	{
		const auto offset = forest::SpiralOffset(i, count);
		glm::vec3 point(std::trunc((centre.x + offset.x) * k_MapUnitsPerMetre) / k_MapUnitsPerMetre, 0.0f,
		                std::trunc((centre.z + offset.y) * k_MapUnitsPerMetre) / k_MapUnitsPerMetre);
		// A slot with no room is passed over, never tried again
		if (!ValidPlaceForTree(point) || planted.size() >= count)
		{
			continue;
		}
		const auto type = forest::SpeciesFor(kinds.magicTreeTypes, random.GameRand(4));
		const float yaw = random.GameFloatRand(k_TwoPi);
		point.y = Locator::terrainSystem::value().GetHeightAt({point.x, point.z});
		const float target = forest::TargetScale(glm::distance(glm::xz(point), glm::xz(centre)));
		// The miracle's trees make a forest of their own, with its own number, made with its first tree
		if (forestEntity == entt::null)
		{
			forestId = _nextMiracleForestId++;
		}
		const auto tree = ecs::archetypes::TreeArchetype::Create(forestId, point, type, true, yaw, target, 0.0f);
		if (forestEntity == entt::null)
		{
			// The forest stands where its first tree does
			forestEntity = registry.Create();
			registry.Assign<Transform>(forestEntity, point, glm::mat3(1.0f), glm::vec3(1.0f));
			registry.Assign<MagicForest>(
			    forestEntity, MagicForest {.spell = spell, .player = miracle.caster.player, .centre = centre, .trees = {}});
		}
		registry.Assign<MagicTree>(tree,
		                           MagicTree {.forest = forestEntity,
		                                      .woodMultiplier = forest::WoodMultiplier(trees->woodValueMultiplier, tribalPower),
		                                      .impressiveValue = effect.impressiveValue});
		// The caster's people come to look at it
		if (Locator::reactionSystem::has_value())
		{
			Locator::reactionSystem::value().Create({.initiator = tree,
			                                         .type = Reaction::ReactToMagicTree,
			                                         .player = miracle.caster.player,
			                                         .position = point,
			                                         .impressiveValue = effect.impressiveValue,
			                                         .power = 1.0f,
			                                         .magicType = MagicType::None,
			                                         .casterCreature = entt::null});
		}
		planted.push_back(tree);
	}
	if (forestEntity != entt::null)
	{
		registry.Get<MagicForest>(forestEntity).trees = planted;
	}
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Magic: a forest planted {} of {} trees on material {}", planted.size(), count,
	                    MaterialAt(centre));
	miracle.forestPlanted = true;
	miracle.objectCount = static_cast<uint32_t>(planted.size());
	registry.SetLayoutDirty();
	return miracle.objectCount;
}

bool ForestSystem::CanGrowAt(glm::vec3 point) const
{
	if (!Locator::entitiesMap::has_value())
	{
		return false;
	}
	// No building's fire would be inside the forest: no abode in the cell (fields aren't abodes here) whose fire, at the
	// abode's place and as wide as its model on the ground, takes in the point, measured across the land in whole map
	// units
	const auto& registry = EntityRegistry();
	const auto& fixed = Locator::entitiesMap::value().GetFixedInGridCell(point);
	const auto at = map_coords::FromMetres(glm::xz(point));
	const bool clear = std::ranges::none_of(fixed, [&registry, at](entt::entity entity) {
		const auto* transform = registry.TryGet<const Transform>(entity);
		if (!registry.AnyOf<Abode>(entity) || registry.AnyOf<Field>(entity) || transform == nullptr)
		{
			return false;
		}
		const auto fireCentre = map_coords::FromMetres(glm::xz(transform->position));
		return gutils::GetDistanceInMetres(at, fireCentre) < object_measures::TwoDRadius(registry, entity);
	});
	return clear && ValidPlaceForTree(point);
}

bool ForestSystem::HasTrees(entt::entity spell) const
{
	bool found = false;
	EntityRegistry().Each<const MagicForest>(
	    [&found, spell](const MagicForest& forest) { found = found || (forest.spell == spell && !forest.trees.empty()); });
	return found;
}

std::optional<entt::entity> ForestSystem::AddTreeNear(entt::entity tree)
{
	auto& registry = EntityRegistry();
	const auto* member = registry.Valid(tree) ? registry.TryGet<const ForestMember>(tree) : nullptr;
	const auto* kind = registry.Valid(tree) ? registry.TryGet<const Tree>(tree) : nullptr;
	const auto* transform = registry.Valid(tree) ? registry.TryGet<const Transform>(tree) : nullptr;
	if (member == nullptr || kind == nullptr || transform == nullptr || !Locator::gameRandom::has_value() ||
	    !Locator::time::has_value() || !Locator::terrainSystem::has_value())
	{
		return std::nullopt;
	}
	const uint32_t turn = Locator::time::value().GetTurn();
	if (!(turn - _lastTreeAddedTurn > k_TreeAddingGapTurns))
	{
		return std::nullopt;
	}
	auto& random = Locator::gameRandom::value();
	const auto& land = Locator::terrainSystem::value();
	// From the tree's own map position, keeping its height above the land
	auto base = map_coords::FromMetres({transform->position.x, transform->position.z});
	base.altitude = transform->position.y - land.GetHeightAt({transform->position.x, transform->position.z});
	float angle = random.GameFloatRand(k_TwoPi);
	// Round the tree at a step of a sixteenth of a half turn, each time at a whole number of metres from 5 to 9 and four
	// more, two apart and wrapping round 10: 5, 7, 9, 1 and 3, say
	for (float outer = 0.0f; outer < k_TreeAddingTurns; outer += 1.0f)
	{
		auto distance = random.GameRand(k_TreeAddingDistances) + k_TreeAddingNearest;
		for (float inner = 0.0f; inner < k_TreeAddingTries; inner += 1.0f)
		{
			const auto spot = base + gutils::GetPosFromAngle(angle, static_cast<float>(distance));
			const glm::vec2 xz = map_coords::ToMetres(spot);
			const glm::vec3 point(xz.x, land.GetHeightAt(xz) + spot.altitude, xz.y);
			if (FreeForAddedTree(point))
			{
				_lastTreeAddedTurn = turn;
				// The way it faces is drawn before its full size
				const float yaw = random.GameFloatRand(k_TwoPi);
				const float fullSize = k_AddedTreeFullSize + random.GameFloatRand(k_AddedTreeFullSizeMore);
				const auto added = ecs::archetypes::TreeArchetype::Create(member->forest, point, kind->type, false, yaw,
				                                                          fullSize, k_AddedTreeStartSize);
				// Beside a forest miracle's tree it is the miracle's forest's, and goes with it
				if (const auto* magic = registry.TryGet<const MagicTree>(tree);
				    magic != nullptr && registry.Valid(magic->forest))
				{
					if (auto* forest = registry.TryGet<MagicForest>(magic->forest))
					{
						forest->trees.push_back(added);
					}
				}
				registry.SetLayoutDirty();
				SPDLOG_LOGGER_DEBUG(spdlog::get("game"),
				                    "Forest: a young tree #{} planted {} m from #{} at ({:.1f}, {:.1f}) on turn {}",
				                    entt::to_integral(added), distance, entt::to_integral(tree), point.x, point.z, turn);
				return added;
			}
			distance = (distance + 2) % 10;
		}
		angle += k_TreeAddingStepAngle;
	}
	return std::nullopt;
}

void ForestSystem::ProcessTurn()
{
	if (!Locator::infoConstants::has_value() || !Locator::entitiesRegistry::has_value())
	{
		return;
	}
	ProcessForests();
}

void ForestSystem::ProcessForests()
{
	auto& registry = EntityRegistry();
	const auto& info = Locator::infoConstants::value();
	std::vector<entt::entity> gone;
	bool resized = false;
	registry.Each<MagicForest>([&](entt::entity entity, MagicForest& forest) {
		auto* spell =
		    forest.spell != entt::null && registry.Valid(forest.spell) ? registry.TryGet<Spell>(forest.spell) : nullptr;
		const auto* trees = spell != nullptr ? magic::GetMagicInfoAs<GMagicForestInfo>(info, spell->magicType) : nullptr;
		const auto& rules = trees != nullptr ? *trees : info.magicForest.at(0);
		// Once its miracle has gone or closed down it has no strength, and may keep none of its trees
		const float strength = spell != nullptr && !spell->closedDown ? spell->processInfo.power : 0.0f;
		const auto afford =
		    forest::TreesWeCanAfford(spell != nullptr ? spell->maxObjectsToCreate : -1, rules.finalNoTrees, strength);
		std::erase_if(forest.trees, [&registry](entt::entity tree) { return !registry.Valid(tree); });
		if (afford < forest.trees.size())
		{
			// Every tree withers, and goes once it has withered away
			std::erase_if(forest.trees, [&](entt::entity tree) {
				const auto after = forest::Wither(registry.Get<const Transform>(tree).scale.y, rules.decaySpeed);
				if (!after.has_value())
				{
					RemoveTree(tree);
					return true;
				}
				Resize(tree, *after);
				return false;
			});
		}
		else
		{
			for (const auto tree : forest.trees)
			{
				const float scale = registry.Get<const Transform>(tree).scale.y;
				Resize(tree, forest::Grow(scale, rules.growSpeed, registry.Get<const Tree>(tree).maxSize));
			}
		}
		resized = resized || !forest.trees.empty();
		if (spell != nullptr)
		{
			spell->objectCount = static_cast<uint32_t>(forest.trees.size());
		}
		if (forest.trees.empty())
		{
			gone.push_back(entity);
		}
	});
	for (const auto entity : gone)
	{
		registry.Destroy(entity);
	}
	if (resized || !gone.empty())
	{
		registry.SetDirty();
	}
}

uint32_t ForestSystem::MakeLandForest(std::optional<uint32_t> id, glm::vec3 position, entt::entity bigForest, bool scenic)
{
	// A number given is kept and moves the count past it; none takes the next
	uint32_t number = _nextLandForestId;
	if (id.has_value() && *id != 0)
	{
		number = *id;
		_nextLandForestId = std::max(_nextLandForestId, number + 1);
	}
	else
	{
		++_nextLandForestId;
	}
	auto& registry = EntityRegistry();
	const auto forest = registry.Create();
	registry.Assign<Transform>(forest, position, glm::mat3(1.0f), glm::vec3(1.0f));
	registry.Assign<LandForest>(
	    forest, LandForest {.id = number, .bigForest = bigForest, .scenic = scenic, .made = _landForestsMade++});
	return number;
}

std::vector<entt::entity> ForestSystem::LandForests() const
{
	std::vector<std::pair<uint32_t, entt::entity>> made;
	EntityRegistry().Each<const LandForest>(
	    [&made](entt::entity forest, const LandForest& record) { made.emplace_back(record.made, forest); });
	std::ranges::sort(made, std::greater {});
	std::vector<entt::entity> forests;
	forests.reserve(made.size());
	std::ranges::transform(made, std::back_inserter(forests), [](const auto& pair) { return pair.second; });
	return forests;
}

std::optional<entt::entity> ForestSystem::LandForestOf(uint32_t id) const
{
	const auto& registry = EntityRegistry();
	for (const auto forest : LandForests())
	{
		if (registry.Get<const LandForest>(forest).id == id)
		{
			return forest;
		}
	}
	return std::nullopt;
}

std::vector<entt::entity> ForestSystem::TreesOf(entt::entity forest, bool growing) const
{
	const auto& registry = EntityRegistry();
	const auto* record = registry.Valid(forest) ? registry.TryGet<const LandForest>(forest) : nullptr;
	if (record == nullptr)
	{
		return {};
	}
	const auto centre = map_coords::FromMetres(glm::xz(registry.Get<const Transform>(forest).position));
	std::vector<std::pair<float, entt::entity>> trees;
	registry.Each<const Tree, const ForestMember, const Transform>(
	    [&](entt::entity tree, const Tree& kind, const ForestMember& member, const Transform& at) {
		    // A tree still short of its size grows on; the others are grown
		    if (member.forest == record->id && (at.scale.x < kind.maxSize) == growing)
		    {
			    trees.emplace_back(gutils::GetDistanceInMetres(map_coords::FromMetres(glm::xz(at.position)), centre), tree);
		    }
	    });
	std::ranges::stable_sort(trees, {}, &std::pair<float, entt::entity>::first);
	std::vector<entt::entity> sorted;
	std::ranges::transform(trees, std::back_inserter(sorted), [](const auto& pair) { return pair.second; });
	return sorted;
}

float ForestSystem::WoodOf(entt::entity forest) const
{
	const auto& registry = EntityRegistry();
	const auto* record = registry.Valid(forest) ? registry.TryGet<const LandForest>(forest) : nullptr;
	if (record == nullptr || !Locator::resourceStoreSystem::has_value())
	{
		return 0.0f;
	}
	const auto& stores = Locator::resourceStoreSystem::value();
	float wood = 0.0f;
	if (record->bigForest != entt::null && registry.Valid(record->bigForest))
	{
		wood += static_cast<float>(stores.ResourceOf(record->bigForest).amount);
	}
	for (const bool growing : {false, true})
	{
		for (const auto tree : TreesOf(forest, growing))
		{
			wood += static_cast<float>(stores.ResourceOf(tree).amount);
		}
	}
	return wood;
}

glm::vec3 ForestSystem::NearestPointOf(entt::entity forest, glm::vec3 to) const
{
	const auto& registry = EntityRegistry();
	const auto& record = registry.Get<const LandForest>(forest);
	if (record.bigForest != entt::null && registry.Valid(record.bigForest))
	{
		// On the big forest's edge, towards the point
		const auto& at = registry.Get<const Transform>(record.bigForest).position;
		const glm::vec2 towards = glm::xz(to) - glm::xz(at);
		const float length = glm::length(towards);
		const auto edge =
		    length > 0.0f ? towards / length * ecs::world_objects::SizeOf(record.bigForest).radius : glm::vec2(0.0f);
		return {at.x + edge.x, at.y, at.z + edge.y};
	}
	return registry.Get<const Transform>(forest).position;
}

void ForestSystem::AssignForestsToTowns()
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();
	const float reach = Locator::infoConstants::value().town.maxDistanceForTownForest;
	const auto forests = LandForests();
	registry.Each<const Town>([&](entt::entity townEntity, const Town& town) {
		// Measured from the town's storage pit. (The game makes a town without one a temporary pile for wood beside its
		// meeting place and measures from that; openblack's towns have no meeting place yet, so such a town lists none.)
		std::optional<glm::vec3> from;
		registry.Each<const StoragePit, const Abode, const Transform>(
		    [&](entt::entity, const StoragePit&, const Abode& abode, const Transform& at) {
			    if (!from.has_value() && abode.townId == town.id)
			    {
				    from = at.position;
			    }
		    });
		auto& listed = registry.AssignOrReplace<TownForests>(townEntity);
		if (!from.has_value())
		{
			return;
		}
		for (const auto forest : forests)
		{
			const float distance = glm::length(glm::xz(NearestPointOf(forest, *from)) - glm::xz(*from));
			if (ecs::land_forests::NearTown(distance, reach, WoodOf(forest)))
			{
				listed.forests.push_back(forest);
			}
		}
	});
}

std::optional<glm::vec3> ForestSystem::ForestLair(AnimalInfo kind, glm::vec3 from) const
{
	const auto& registry = EntityRegistry();
	const auto own = map_coords::FromMetres(glm::xz(from));
	const auto distanceTo = [&own](glm::vec3 at) { return gutils::GetDistance(own, map_coords::FromMetres(glm::xz(at))); };
	if (kind == AnimalInfo::Wolf)
	{
		// A wolf's flock goes to the nearest big forest first
		std::vector<entt::entity> bigForests;
		std::vector<int32_t> distances;
		registry.Each<const BigForest, const Transform>([&](entt::entity big, const BigForest&, const Transform& at) {
			bigForests.push_back(big);
			distances.push_back(distanceTo(at.position));
		});
		if (const auto nearest = ecs::land_forests::Nearest(distances))
		{
			return registry.Get<const Transform>(bigForests[*nearest]).position;
		}
	}
	// Then the forest the flock's rule chooses, at its grown tree nearest its place
	const auto forests = LandForests();
	std::vector<float> scores;
	std::ranges::transform(forests, std::back_inserter(scores), [&](entt::entity forest) {
		return ecs::land_forests::LairScore(distanceTo(registry.Get<const Transform>(forest).position));
	});
	if (const auto chosen = ecs::land_forests::LairForest(scores))
	{
		const auto grown = TreesOf(forests[*chosen], false);
		if (!grown.empty())
		{
			return registry.Get<const Transform>(grown.front()).position;
		}
	}
	if (kind == AnimalInfo::Wolf)
	{
		// Else the nearest tree of all
		std::vector<entt::entity> trees;
		std::vector<int32_t> distances;
		registry.Each<const Tree, const Transform>([&](entt::entity tree, const Tree&, const Transform& at) {
			trees.push_back(tree);
			distances.push_back(distanceTo(at.position));
		});
		if (const auto nearest = ecs::land_forests::Nearest(distances))
		{
			return registry.Get<const Transform>(trees[*nearest]).position;
		}
	}
	// Else where the animal is
	return from;
}

void ForestSystem::MakeScenicForests()
{
	if (!Locator::infoConstants::has_value() || !Locator::entitiesMap::has_value())
	{
		return;
	}
	auto& registry = EntityRegistry();
	const auto& map = Locator::entitiesMap::value();
	const float reach = Locator::infoConstants::value().town.maxDistanceForTownForest + ecs::scenic_forest::k_Beyond;
	// Where each scenic forest was made, by its number
	std::unordered_map<uint32_t, glm::vec2> centres;
	registry.Each<const Town>([&centres](entt::entity, const Town& town) {
		if (town.scenicForest.has_value())
		{
			centres[*town.scenicForest] = town.scenicForestCentre;
		}
	});
	registry.Each<Town>([&](entt::entity, Town& town) {
		// The town's centre is the middle of the ground its buildings and fields cover
		ecs::scenic_forest::TownArea area;
		registry.Each<const Abode, const Transform>([&](entt::entity thing, const Abode& abode, const Transform& at) {
			if (abode.townId == town.id)
			{
				area.Add({at.position.x, at.position.z}, ecs::world_objects::SizeOf(thing).radius);
			}
		});
		registry.Each<const Field, const Transform>([&](entt::entity thing, const Field& field, const Transform& at) {
			if (field.town == static_cast<int>(town.id))
			{
				area.Add({at.position.x, at.position.z}, ecs::world_objects::SizeOf(thing).radius);
			}
		});
		const auto centre = area.Centre();
		std::optional<uint32_t> forest = town.scenicForest;
		for (const auto& coords : ecs::scenic_forest::Walk(map_coords::FromMetres(centre), reach))
		{
			const glm::ivec2 cell = map_coords::Cell(coords);
			if (!map_coords::InBounds(cell))
			{
				continue;
			}
			for (const auto tree : map.GetAllInCell(cell))
			{
				if (!registry.AllOf<Tree, Transform>(tree))
				{
					continue;
				}
				const auto& position = registry.Get<const Transform>(tree).position;
				ecs::scenic_forest::Tree seen {.at = {position.x, position.z}};
				if (const auto* member = registry.TryGet<const ForestMember>(tree))
				{
					seen.inForest = true;
					if (const auto other = centres.find(member->forest); other != centres.end())
					{
						seen.scenicCentre = other->second;
					}
				}
				if (!ecs::scenic_forest::Takes(seen, centre))
				{
					continue;
				}
				// The forest is made, about the town's centre, at the first tree it takes
				if (!forest.has_value())
				{
					const float height =
					    Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(centre) : 0.0f;
					forest = MakeLandForest(std::nullopt, {centre.x, height, centre.y}, entt::null, true);
					centres[*forest] = centre;
					town.scenicForestCentre = centre;
				}
				registry.AssignOrReplace<ForestMember>(tree, ForestMember {.forest = *forest});
			}
		}
		town.scenicForest = forest;
	});
}

void ForestSystem::Reset()
{
	_lastTreeAddedTurn = 0;
	_nextMiracleForestId = k_FirstMiracleForestId;
	_nextLandForestId = 1;
	_landForestsMade = 0;
}
