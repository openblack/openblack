/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// What the particle effects of the heal, the lightning and the creature spells ask of the living world: lighting the
// healed, the things a bolt may strike, the arcs a strike leaves and the bolts from hands that may clash

#define LOCATOR_IMPLEMENTATIONS

#include <algorithm>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/common.hpp>
#include <glm/geometric.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/L3DSubMesh.h"
#include "3D/MapCoords.h"
#include "Creature/CreatureRig.h"
#include "Creature/CreatureSpells.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Animal.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureSpells.h"
#include "ECS/Components/Feature.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/ObjectGlow.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Map.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MapSpiral.h"
#include "ParticleSystem.h"
#include "Particles/LightningMaths.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// A glow nothing has set for this many turns goes, as when its effect went without letting go of it
constexpr uint8_t k_GlowTurnsUnset = 2;
/// While this many objects have arcs crawling over them, no more are queued
constexpr long k_MostLiveArcs = 100;
} // namespace

void GameParticleWorld::SetTargetGlow(entt::entity target, glm::u8vec3 rgb)
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(target))
	{
		return;
	}
	if (rgb == glm::u8vec3(0))
	{
		registry.Remove<ObjectGlow>(target);
		return;
	}
	registry.AssignOrReplace<ObjectGlow>(target, rgb, static_cast<uint8_t>(0));
}

void GameParticleWorld::ProcessGlows()
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> faded;
	registry.Each<ObjectGlow>([&faded](entt::entity entity, ObjectGlow& glow) {
		if (++glow.turnsUnset > k_GlowTurnsUnset)
		{
			faded.push_back(entity);
		}
	});
	for (const auto entity : faded)
	{
		registry.Remove<ObjectGlow>(entity);
	}
}

std::vector<particles::StrikeCandidate> GameParticleWorld::StrikeCandidates(glm::vec3 centre, size_t cells) const
{
	std::vector<particles::StrikeCandidate> candidates;
	if (!Locator::entitiesRegistry::has_value() || !Locator::entitiesMap::has_value())
	{
		return candidates;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	const auto& map = Locator::entitiesMap::value();
	for (const auto& cell : magic::SpiralCells(map_coords::CellOf(glm::vec2(centre.x, centre.z)), cells))
	{
		// Everything in the cell as the map keeps it, what stays put first; a building covering several cells only in
		// the cell it stands in
		for (const auto entity : map.GetAllInCell(cell))
		{
			if (!registry.Valid(entity) || registry.AnyOf<OneOffSpellSeed>(entity))
			{
				continue;
			}
			const auto* transform = registry.TryGet<const Transform>(entity);
			if (transform == nullptr || map_coords::CellOf(transform->position) != cell)
			{
				continue;
			}
			// The dead are no longer there to strike
			if (const auto* villager = registry.TryGet<const Villager>(entity); villager != nullptr && villager->health <= 0)
			{
				continue;
			}
			if (const auto* animal = registry.TryGet<const Animal>(entity); animal != nullptr && animal->Dead())
			{
				continue;
			}
			// A creature not faded away draws the bolt; arcs crawl over what isn't alive
			bool drawsBolt = false;
			if (registry.AnyOf<Creature>(entity))
			{
				const auto* spells = registry.TryGet<const CreatureSpells>(entity);
				drawsBolt = spells == nullptr || spells->fizz < particles::maths::k_DrawsBoltBelowFizz;
			}
			const auto info = Target(entity, false);
			// Where it stands as a map position holds it
			candidates.push_back({.object = entity,
			                      .position = {map_coords::Quantise(transform->position.x), transform->position.y,
			                                   map_coords::Quantise(transform->position.z)},
			                      .height = info ? info->height : 0.0f,
			                      .drawsBolt = drawsBolt,
			                      .arcs = !registry.AnyOf<Creature, Villager, Animal>(entity)});
		}
	}
	return candidates;
}

void GameParticleWorld::QueueArcs(entt::entity object)
{
	SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Particles: arcs crawl over struck object {}", static_cast<uint32_t>(object));
	if (_liveArcs.use_count() - 1 >= k_MostLiveArcs)
	{
		return;
	}
	_arcsWaiting.push_back(object);
	_arcsWanted = true;
}

std::shared_ptr<const void> GameParticleWorld::HoldArc() const
{
	return _liveArcs;
}

std::optional<entt::entity> GameParticleWorld::TakeArcs()
{
	if (_arcsWaiting.empty())
	{
		return std::nullopt;
	}
	const auto object = _arcsWaiting.back();
	_arcsWaiting.pop_back();
	return object;
}

bool GameParticleWorld::TakeArcsWanted()
{
	const bool wanted = _arcsWanted;
	_arcsWanted = false;
	return wanted;
}

std::vector<particles::SurfacePoint> GameParticleWorld::SurfacePoints(entt::entity object, size_t count,
                                                                      const std::function<int32_t(int32_t)>& random) const
{
	std::vector<particles::SurfacePoint> points;
	if (!Locator::entitiesRegistry::has_value() || !Locator::resources::has_value() || !random)
	{
		return points;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(object))
	{
		return points;
	}
	const auto* mesh = registry.TryGet<const Mesh>(object);
	const auto* transform = registry.TryGet<const Transform>(object);
	auto& meshes = Locator::resources::value().GetMeshes();
	if (mesh == nullptr || transform == nullptr || !meshes.Contains(mesh->id))
	{
		return points;
	}
	const auto model = meshes.Handle(mesh->id);
	// The game picks a part of the model at random, then one of its primitives, then each point among that primitive's
	// vertices
	std::vector<const graphics::L3DSubMesh*> parts;
	for (const auto& part : model->GetSubMeshes())
	{
		if (!part->IsPhysics() && !part->GetSurfacePrimitives().empty())
		{
			parts.push_back(part.get());
		}
	}
	if (parts.empty())
	{
		return points;
	}
	const auto* part = parts.at(static_cast<size_t>(random(static_cast<int32_t>(parts.size()))));
	const auto& primitives = part->GetSurfacePrimitives();
	const auto& primitive = primitives.at(static_cast<size_t>(random(static_cast<int32_t>(primitives.size()))));
	if (primitive.count == 0)
	{
		return points;
	}
	for (size_t i = 0; i < count; ++i)
	{
		const auto& vertex =
		    part->GetSurfacePoints().at(primitive.first + static_cast<uint32_t>(random(static_cast<int32_t>(primitive.count))));
		const auto position = transform->rotation * (vertex.position * transform->scale) + transform->position;
		// The game turns the normal by the whole of the object's frame, its place included, so the arcs leave leaning
		// away from the map's corner
		auto normal = transform->rotation * (vertex.normal * transform->scale) + transform->position;
		const float length = glm::length(normal);
		if (length > 0.0f)
		{
			normal /= length;
		}
		points.push_back({.position = position, .normal = normal});
	}
	return points;
}

void GameParticleWorld::AddBolt(const std::shared_ptr<particles::BoltShare>& bolt)
{
	std::erase_if(_bolts, [](const auto& weak) { return weak.expired(); });
	_bolts.push_back(bolt);
}

std::vector<std::shared_ptr<particles::BoltShare>> GameParticleWorld::Bolts() const
{
	std::vector<std::shared_ptr<particles::BoltShare>> live;
	for (const auto& weak : _bolts)
	{
		if (auto bolt = weak.lock())
		{
			live.push_back(std::move(bolt));
		}
	}
	return live;
}

void GameParticleWorld::StrikeWithoutMiracle(glm::vec3 point)
{
	if (!Locator::magicSystem::has_value() || !Locator::infoConstants::has_value())
	{
		return;
	}
	const auto& effects = Locator::infoConstants::value().effect;
	const auto row = static_cast<size_t>(EffectInfo::WeatherLightning);
	if (row < effects.size())
	{
		Locator::magicSystem::value().ApplyEffectAt(point, magic::EffectValues::From(effects.at(row)), PlayerNames::NEUTRAL);
	}
}

bool GameParticleWorld::QueueBeliefSprite(const particles::BeliefSprite& sprite)
{
	constexpr size_t k_MostBeliefSprites = 400;
	if (_beliefSprites.size() >= k_MostBeliefSprites)
	{
		return false;
	}
	_beliefSprites.push_back(sprite);
	return true;
}

std::optional<particles::BeliefSprite> GameParticleWorld::TakeBeliefSprite()
{
	if (_beliefSprites.empty())
	{
		return std::nullopt;
	}
	const auto sprite = _beliefSprites.back();
	_beliefSprites.pop_back();
	return sprite;
}

void ParticleSystem::AddBeliefSprite(const particles::BeliefSprite& sprite)
{
	_beliefWanted |= _world.QueueBeliefSprite(sprite);
}

std::optional<particles::CreatureSpellBody> GameParticleWorld::CreatureBody(entt::entity creature) const
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return std::nullopt;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(creature))
	{
		return std::nullopt;
	}
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (body == nullptr || transform == nullptr)
	{
		return std::nullopt;
	}
	particles::CreatureSpellBody result {.origin = transform->position, .size = body->size};
	// Each bone where it is drawn this frame, and its mirror as the species' file has it
	if (const auto* animation = registry.TryGet<const CreatureAnimation>(creature))
	{
		const auto placement = creature::PlacementMatrix(transform->position, transform->rotation, transform->scale);
		result.bones.reserve(animation->boneMatrices.size());
		for (uint32_t bone = 0; bone < animation->boneMatrices.size(); ++bone)
		{
			result.bones.emplace_back(creature::PosedBone(bone, animation->boneMatrices, placement)[3]);
		}
		result.mirror = animation->mirror;
	}
	if (Locator::resources::has_value())
	{
		const auto& rigs = Locator::resources::value().GetCreatureRigs();
		const auto rigId = creature::GetRigId(body->species);
		if (rigs.Contains(rigId))
		{
			const auto& rig = *rigs.Handle(rigId);
			if (rig.actionPoints.has_value())
			{
				result.rightFoot = rig.actionPoints->rightFoot;
			}
			result.rightEye = rig.rightEye;
		}
	}
	if (const auto* spells = registry.TryGet<const CreatureSpells>(creature))
	{
		result.invisible = spells->invisible ? std::clamp(spells->fizz / creature_spells::k_InvisibleFizz, 0.0f, 1.0f) : 0.0f;
	}
	return result;
}
