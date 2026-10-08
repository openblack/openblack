/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ParticleObjectEffects.h"

#include <cmath>

#include <algorithm>
#include <numbers>

#include <LNDFile.h>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/MapCoords.h"
#include "3D/WaterRings.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Fire.h"
#include "ECS/Components/MagicFireBall.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/ExplosionSystemInterface.h"
#include "ECS/Systems/FireSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "ECS/Systems/WaterRingSystemInterface.h"
#include "ECS/WorldObjects.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Particles/ParticleBlast.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
namespace world_objects = openblack::ecs::world_objects;

namespace
{
constexpr float k_TwoPi = 2.0f * std::numbers::pi_v<float>;
/// The land's cells are this wide; a blast looks this much further than its range for the objects round it
constexpr float k_CellSize = 10.0f;
constexpr float k_WaveCellMargin = 20.0f;

ecs::Registry& Entities()
{
	return Locator::entitiesRegistry::value();
}
} // namespace

std::optional<entt::entity> GameObjectEffects::AttachFireBall(glm::vec3 position, float strength, float radius,
                                                              bool affectedByRain, std::optional<PlayerNames> player)
{
	if (!Locator::fireSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	auto& registry = Entities();
	const auto ball = registry.Create();
	registry.Assign<Transform>(ball, position, glm::mat3(1.0f), glm::vec3(1.0f));
	registry.Assign<MagicFireBall>(ball, MagicFireBall {
	                                         .strength = strength,
	                                         .radius = radius,
	                                         .affectedByRain = affectedByRain,
	                                         .player = player.value_or(PlayerNames::NEUTRAL),
	                                         .hasPlayer = player.has_value(),
	                                         .lastTurn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0,
	                                     });
	// As hot as its miracle is strong; every fireball takes the table's first row
	const float temperature = strength * Locator::infoConstants::value().magicFireBall.at(0).initialTemperature;
	Locator::fireSystem::value().SetTemperature(ball, temperature, entt::null);
	if (!registry.AllOf<Fire>(ball))
	{
		registry.Destroy(ball);
		return std::nullopt;
	}
	return ball;
}

GameObjectEffects::FireBallState GameObjectEffects::FollowFireBall(entt::entity ball, glm::vec3 position, float radius,
                                                                   float strength, std::optional<glm::vec3> handTarget)
{
	auto& registry = Entities();
	if (!registry.Valid(ball) || !registry.AllOf<Fire, MagicFireBall>(ball))
	{
		if (registry.Valid(ball))
		{
			registry.Destroy(ball);
		}
		return FireBallState::Gone;
	}
	registry.Get<Transform>(ball).position = position;
	auto& magic = registry.Get<MagicFireBall>(ball);
	magic.radius = radius;
	// Its heat capacity follows its miracle's strength as it is now, none once the miracle has closed down
	magic.strength = strength;
	magic.handTarget = handTarget;
	magic.lastTurn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
	const float deletion = Locator::infoConstants::value().magicFireBall.at(0).deletionTemperature;
	return registry.Get<Fire>(ball).state.temperature < deletion ? FireBallState::Cooled : FireBallState::Burning;
}

uint32_t GameObjectEffects::StartSpotVisual(SpotVisualType type, glm::vec3 position, float magnitude, int turns)
{
	if (!Locator::particleSystem::has_value())
	{
		return ParticleSystemInterface::k_NoEffect;
	}
	return Locator::particleSystem::value().StartSpotVisual(type, position, turns, entt::null, magnitude);
}

void GameObjectEffects::CloseSpotVisual(uint32_t id)
{
	if (Locator::particleSystem::has_value())
	{
		Locator::particleSystem::value().CloseDown(id);
	}
}

std::vector<particles::WaveTarget> GameObjectEffects::FixedObjectsNear(glm::vec3 centre, float range) const
{
	std::vector<particles::WaveTarget> targets;
	if (!Locator::entitiesMap::has_value())
	{
		return targets;
	}
	const auto& map = Locator::entitiesMap::value();
	const auto& registry = Entities();
	// A spiral of cells out from the centre's, each object taken from its own cell, once
	const auto side = static_cast<int>(std::ceil((range + k_WaveCellMargin) / k_CellSize));
	const auto start = glm::ivec2(ecs::MapInterface::GetGridCell(centre));
	glm::ivec2 offset {0, 0};
	map_coords::Spiral spiral;
	for (int step = 0; step < side * side; ++step)
	{
		const auto cell = start + offset;
		if (map_coords::InBounds(cell))
		{
			for (const auto object : map.GetFixedInGridCell(ecs::MapInterface::CellId(cell)))
			{
				// Pots and one-off spell seeds are filed among the fixed objects here, but in the game they move about
				// and the wave never looks at them
				const auto* transform = registry.TryGet<const Transform>(object);
				if (transform == nullptr || registry.AnyOf<Pot, OneOffSpellSeed>(object) ||
				    glm::ivec2(ecs::MapInterface::GetGridCell(transform->position)) != cell)
				{
					continue;
				}
				const auto target = Available(object);
				const glm::vec2 across(transform->position.x - centre.x, transform->position.z - centre.z);
				if (target.has_value() && glm::length(across) < target->radius + range)
				{
					targets.push_back(*target);
				}
			}
		}
		const auto& next = spiral.Next();
		offset += glm::ivec2(next.x, next.z);
	}
	return targets;
}

std::optional<particles::WaveTarget> GameObjectEffects::Available(entt::entity object) const
{
	const auto& registry = Entities();
	if (!registry.Valid(object))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(object);
	if (transform == nullptr)
	{
		return std::nullopt;
	}
	return particles::WaveTarget {
	    .object = object, .point = transform->position, .radius = world_objects::SizeOf(object).radius};
}

bool GameObjectEffects::CanBeDestroyedBySpell(entt::entity object) const
{
	return world_objects::CanBeDestroyedBySpell(object);
}

bool GameObjectEffects::IsCreature(entt::entity object) const
{
	return Entities().AllOf<Creature>(object);
}

void GameObjectEffects::ExplodeObject(entt::entity object, glm::vec3 origin, float speed)
{
	const auto& registry = Entities();
	const auto* mesh = registry.TryGet<const Mesh>(object);
	const auto* transform = registry.TryGet<const Transform>(object);
	if (mesh == nullptr || transform == nullptr)
	{
		return;
	}
	auto matrix = glm::translate(glm::mat4(1.0f), transform->position) * glm::mat4(transform->rotation);
	matrix = glm::scale(matrix, transform->scale);
	ExplodeMesh(mesh->id, matrix, origin, speed);
}

void GameObjectEffects::ExplodeMesh(entt::id_type mesh, const glm::mat4& transform, glm::vec3 origin, float speed)
{
	_queues.at(0).push_back({.mesh = mesh, .transform = transform, .origin = origin, .speed = speed});
}

std::vector<particles::ExplodeRecord> GameObjectEffects::TakeExplodeRecords(int queue)
{
	if (queue < 0 || static_cast<size_t>(queue) >= _queues.size())
	{
		return {};
	}
	return std::exchange(_queues.at(static_cast<size_t>(queue)), {});
}

void GameObjectEffects::DestroyByBeam(entt::entity object)
{
	if (Locator::fireSystem::has_value() && !world_objects::IsBuilding(object))
	{
		// What goes takes its fire with it
		Locator::fireSystem::value().PutOut(object);
	}
	world_objects::Destroy(object);
}

void GameObjectEffects::AddRubbleMark(glm::vec3 centre)
{
	if (Locator::explosionSystem::has_value() && Locator::gameRandom::has_value())
	{
		Locator::explosionSystem::value().AddRubble(centre, Locator::gameRandom::value().LocalFloatRand(k_TwoPi),
		                                            particles::blast::k_RubbleScale);
	}
}

void GameObjectEffects::AddWaterRing(glm::vec3 centre, float size)
{
	if (Locator::waterRingSystem::has_value())
	{
		Locator::waterRingSystem::value().Add({.position = centre, .growth = size, .cell = 0x30, .argb = 0xFFFFFFFFu});
	}
}

void GameObjectEffects::ShakeCamera(glm::vec3 position, float radius, float strength, float seconds)
{
	if (Locator::explosionSystem::has_value())
	{
		Locator::explosionSystem::value().AddShake(position, radius, strength, seconds, true);
	}
}

bool GameObjectEffects::IsDryLand(glm::vec3 point) const
{
	if (!Locator::terrainSystem::has_value() || point.x < 0.0f || point.z < 0.0f)
	{
		return false;
	}
	// Dry where its cell stands above the sea's level, whether or not the land's file marks water in it, as a coast's
	// cells may be; off the map is not
	constexpr uint8_t k_HighestWetAltitude = 3;
	const auto* cell =
	    Locator::terrainSystem::value().FindCell(glm::u16vec2(glm::floor(glm::vec2(point.x, point.z) / k_CellSize)));
	const bool dry = cell != nullptr && cell->altitude > k_HighestWetAltitude;
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Blast: land at ({:.1f}, {:.1f}) altitude {} water flag {}: {}", point.x, point.z,
	                    static_cast<int>(cell != nullptr ? cell->altitude : 0),
	                    static_cast<int>(cell != nullptr ? cell->properties.hasWater : 0), dry ? "dry" : "wet");
	return dry;
}

void GameObjectEffects::Reset()
{
	for (auto& queue : _queues)
	{
		queue.clear();
	}
}

std::shared_ptr<const std::vector<particles::blast::FragmentPiece>> GameFragmentSource::Fragments(entt::id_type mesh)
{
	if (const auto found = _pieces.find(mesh); found != _pieces.end())
	{
		return found->second;
	}
	if (!Locator::resources::has_value() || !Locator::resources::value().GetMeshes().Contains(mesh))
	{
		return nullptr;
	}
	const auto model = Locator::resources::value().GetMeshes().Handle(mesh);
	// The first detail's triangles of the submeshes that always show
	std::vector<particles::blast::SourcePrimitive> primitives;
	for (const auto& subMesh : model->GetSubMeshes())
	{
		const auto flags = subMesh->GetFlags();
		const auto& surface = subMesh->GetSurface();
		if ((flags.lodMask & 1u) == 0 || flags.status != 0 || surface.indices.empty())
		{
			continue;
		}
		for (const auto& primitive : subMesh->GetPrimitives())
		{
			if (primitive.indicesOffset + primitive.indicesCount > surface.indices.size())
			{
				continue;
			}
			primitives.push_back({
			    .positions = surface.positions,
			    .uvs = surface.uvs,
			    .normals = surface.normals,
			    .indices = std::span(surface.indices).subspan(primitive.indicesOffset, primitive.indicesCount),
			    .skin = primitive.skinID,
			});
		}
	}
	auto pieces =
	    std::make_shared<const std::vector<particles::blast::FragmentPiece>>(particles::blast::BreakIntoPieces(primitives));
	_pieces.emplace(mesh, pieces);
	return pieces;
}
