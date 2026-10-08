/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "VegetationSystem.h"

#include <cmath>

#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/L3DMesh.h"
#include "3D/TreeGrowth.h"
#include "Audio/AudioManagerInterface.h"
#include "Camera/Camera.h"
#include "Common/RandomNumberManager.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Swayable.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Registry.h"
#include "ECS/Systems/AlignmentSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/InfluenceSystemInterface.h"
#include "ECS/Systems/TreeRustle.h"
#include "ECS/Systems/WeatherSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// The land's alignment at a place: each player's influence there times their alignment
float LandAlignmentAt(const glm::vec3& position)
{
	if (!openblack::Locator::influenceSystem::has_value() || !openblack::Locator::alignmentSystem::has_value())
	{
		return 0.0f;
	}
	const auto& influence = openblack::Locator::influenceSystem::value();
	const auto& alignment = openblack::Locator::alignmentSystem::value();
	float sum = 0.0f;
	for (size_t i = 0; i < static_cast<size_t>(openblack::PlayerNames::_COUNT); ++i)
	{
		const auto player = static_cast<openblack::PlayerNames>(i);
		sum += alignment.GetPlayerAlignment(player) * influence.PlayerInfluence(player, position);
	}
	return std::clamp(sum, -1.0f, 1.0f);
}
} // namespace

void VegetationSystem::ProcessTurn()
{
	if (!Locator::infoConstants::has_value())
	{
		return;
	}
	const auto& infos = Locator::infoConstants::value().tree;
	auto* weather = Locator::weatherSystem::has_value() ? &Locator::weatherSystem::value() : nullptr;
	Locator::entitiesRegistry::value().Each<Tree, Transform>([&](Tree& tree, Transform& transform) {
		if (!(transform.scale.x < tree.maxSize) || --tree.turnsToGrowth != 0)
		{
			return;
		}
		const auto& info = infos.at(static_cast<size_t>(tree.type));
		tree.turnsToGrowth = std::max<uint32_t>(info.growsAfterNumGameTurns, 1);
		int wet = 0;
		if (weather != nullptr)
		{
			const auto here = weather->GetWeather(transform.position);
			wet = std::max<int>(here.rain, here.snow);
		}
		const openblack::tree_growth::Type type {.turnsBetween = info.growsAfterNumGameTurns,
		                                         .amount = info.growthAmount,
		                                         .rainAccelerator = info.rainingAcceleratorMultiplier};
		const float grown = openblack::tree_growth::Grown(
		    transform.scale.x, openblack::tree_growth::Growth(type, wet, LandAlignmentAt(transform.position)), tree.maxSize);
		transform.scale = glm::vec3(grown);
	});
}

namespace
{
// The sways speed up and slow down at random every two seconds
constexpr auto k_SpeedChangeTime = std::chrono::duration<float, std::milli>(2000.0f);
constexpr float k_MinSpeed = 1.0f;
constexpr float k_MaxSpeed = 2.0f;
// Radians a sway moves on per millisecond of game time at a speed of 1
constexpr float k_PhasePerMillisecond = 0.0010606061f;
// How far a sway leans a tree, per unit of its height
constexpr float k_Lean = 0.03f;
// How much further a field's crop leans than a tree
constexpr float k_FieldLean = 1.75f;

// A tree within a bend point's radius leans away from it by up to this many radians
constexpr float k_MaxBend = 0.47123894f;
// The bend falls from (1 - k_BendCore / radius) at the trunk to nothing at the radius
constexpr float k_BendCore = 1.5f;
/// A creature bends the trees within this many times its radius of its feet
constexpr float k_CreatureBendReach = 1.5f;
} // namespace

void VegetationSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	_speedTime += gameTime;
	const bool changeSpeeds = _speedTime > k_SpeedChangeTime;
	auto& rng = Locator::rng::value();
	for (size_t i = 0; i < k_SwayCount; ++i)
	{
		if (changeSpeeds)
		{
			_speeds.at(i) = rng.NextValue(k_MinSpeed, k_MaxSpeed);
		}
		_phases.at(i) += gameTime.count() * _speeds.at(i) * k_PhasePerMillisecond;
		// The sways lean along the z axis only: the direction they lean in is fixed at 0
		_leans.at(i) = -std::cos(_phases.at(i)) * k_Lean;
	}
	if (changeSpeeds)
	{
		_speedTime = decltype(_speedTime)::zero();
	}
}

// The game marks the trees around the hand to bend away from it, within its bounding sphere
void VegetationSystem::UpdateBendPoints()
{
	_bendPoints.clear();
	const auto& registry = Locator::entitiesRegistry::value();
	// Creatures walk through the trees too small for them to walk round, and bend them as they go
	registry.Each<const CreatureLocomotion, const Transform>(
	    [this](const CreatureLocomotion& creature, const Transform& transform) {
		    _bendPoints.push_back({.position = transform.position, .radius = creature.radius * k_CreatureBendReach});
	    });

	auto& handSystem = Locator::handSystem::value();
	const auto handEntity = handSystem.GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	const auto position = handSystem.GetPlayerHandPositions()[static_cast<size_t>(HandSystemInterface::Side::Left)];
	const auto* transform = registry.TryGet<const Transform>(handEntity);
	const auto* mesh = registry.TryGet<const Mesh>(handEntity);
	if (!position || transform == nullptr || mesh == nullptr)
	{
		return;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh->id))
	{
		return;
	}
	// The mesh's diagonal length: half the diagonal of the mesh's bounding box
	const auto halfDiagonal = glm::length(meshes.Handle(mesh->id)->GetBoundingBox().Size()) * 0.5f;
	_bendPoints.push_back({
	    .position = *position,
	    .radius = std::abs(transform->scale.y) * halfDiagonal,
	});
}

std::optional<VegetationSystem::Bend> VegetationSystem::GetBend(const glm::vec3& position, float height) const
{
	std::optional<Bend> most;
	for (const auto& point : _bendPoints)
	{
		// The hand bends the trees it is among, from below their tops
		if (point.position.y > position.y + height || point.radius <= 0.0f)
		{
			continue;
		}
		const auto away = glm::xz(position) - glm::xz(point.position);
		const auto distance = glm::length(away);
		if (distance >= point.radius || distance <= 0.0f)
		{
			continue;
		}
		const auto radius = point.radius;
		const auto amount = 1.0f - (((((radius - k_BendCore) * distance) / radius) + k_BendCore) / radius);
		if (amount > 0.0f && (!most || amount > most->amount))
		{
			most = Bend {.direction = glm::vec3(away.x, 0.0f, away.y) / distance, .amount = amount};
		}
	}
	return most;
}

void VegetationSystem::Rustle(std::chrono::duration<float, std::milli> gameTime)
{
	const auto& registry = Locator::entitiesRegistry::value();
	auto& meshes = Locator::resources::value().GetMeshes();
	auto& audio = Locator::audio::value();
	auto& rng = Locator::rng::value();
	const auto camera = Locator::camera::value().GetOrigin();
	const auto idleChance = TreeRustle::IdleChance(gameTime);
	const auto bank = std::string(TreeRustle::k_Bank);

	// The trees share a few meshes, whose heights are looked up once each
	std::unordered_map<entt::id_type, float> meshHeights;
	const auto meshHeight = [&meshes, &meshHeights](entt::id_type id) {
		const auto [found, inserted] = meshHeights.try_emplace(id, 0.0f);
		if (inserted && meshes.Contains(id))
		{
			found->second = meshes.Handle(id)->GetBoundingBox().Size().y;
		}
		return found->second;
	};

	registry.Each<const Tree, const Transform, const Mesh, const Swayable>([&](entt::entity entity, const Tree& /*unused*/,
	                                                                           const Transform& transform, const Mesh& mesh,
	                                                                           const Swayable& /*unused*/) {
		// Only a tree near the camera or within a bend point's reach can rustle, and only for those does its height
		// matter: the others are passed over before it is looked up
		const bool nearBend = std::ranges::any_of(_bendPoints, [&transform](const BendPoint& point) {
			return point.radius > 0.0f && glm::distance(glm::xz(transform.position), glm::xz(point.position)) < point.radius;
		});
		if (!nearBend && !TreeRustle::IsNearCamera(transform.position, camera))
		{
			return;
		}
		const auto height = transform.scale.y * meshHeight(mesh.id);
		if (const auto bend = GetBend(transform.position, height))
		{
			audio.PlayAnimEffect(bank, TreeRustle::BendKeys(bend->amount).ToArray(), entity, transform.position);
		}
		if (idleChance > 1 && TreeRustle::IsAmongTree(transform.position, height, camera) &&
		    rng.NextValue<uint32_t>(0, idleChance - 1) == 1)
		{
			audio.PlayAnimEffect(bank, TreeRustle::IdleKeys().ToArray(), entity, transform.position);
		}
	});
}

glm::mat4 VegetationSystem::GetTreeMatrix(const glm::mat4& model, const glm::vec3& position, float scale, float height,
                                          uint8_t swaySlot) const
{
	if (const auto bend = GetBend(position, scale * height))
	{
		// Tip the tree's top away from the hand, about its base
		const auto axis = glm::cross(glm::vec3(0.0f, 1.0f, 0.0f), bend->direction);
		const auto tilt = glm::rotate(glm::mat4(1.0f), k_MaxBend * bend->amount, axis);
		return glm::translate(glm::mat4(1.0f), position) * tilt * glm::translate(glm::mat4(1.0f), -position) * model;
	}

	// Lean the tree with its sway: the higher a point of the tree, the further it moves
	auto swaying = model;
	swaying[1].z = scale * _leans.at(swaySlot % k_SwayCount);
	return swaying;
}

// TODO(raffclar): the game only sways a field once its crop is fully grown, there is no growth in openblack yet
glm::mat4 VegetationSystem::GetFieldMatrix(const glm::mat4& model, float scale, uint8_t swaySlot) const
{
	auto swaying = model;
	swaying[1].x = 0.0f;
	swaying[1].z = scale * _leans.at(swaySlot % k_SwayCount) * k_FieldLean;
	return swaying;
}
