/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureArchetype.h"

#include <glm/gtx/euler_angles.hpp>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "Creature/CreatureMorph.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureHair.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/CreatureNeeds.h"
#include "ECS/Components/CreatureSkin.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Enums.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;
using namespace openblack::creature;

namespace
{
// What is known of a species without the game's creature tables
constexpr float k_UnknownStartScale = 0.22f;

const GCreatureInfo* SpeciesInfo(CreatureType species)
{
	if (!Locator::infoConstants::has_value())
	{
		return nullptr;
	}
	const auto& creatures = Locator::infoConstants::value().creature;
	const auto row = creature::InfoRow(species);
	return row < creatures.size() ? &creatures.at(row) : nullptr;
}

} // namespace

float CreatureArchetype::DrawnScale(CreatureType species, float size)
{
	const auto meshId = creature::GetIdFromType(species, CreatureBody::Appearance::Base);
	const auto& meshes = Locator::resources::value().GetMeshes();
	const auto restHeight =
	    meshes.Contains(meshId) ? creature_morph::RestHeight(meshes.Handle(meshId)->GetBoneMatrices()) : 0.0f;
	return creature_morph::DrawnScale(size, restHeight);
}

float CreatureArchetype::SpeciesStrength(CreatureType species)
{
	const auto* info = SpeciesInfo(species);
	return info != nullptr ? info->strength : creature_morph::k_UnknownSpeciesStrength;
}

float CreatureArchetype::StartScale(CreatureType species)
{
	const auto* info = SpeciesInfo(species);
	return creature_morph::ClampScale(info != nullptr ? info->startScale : k_UnknownStartScale);
}

CreatureArchetype::Body CreatureArchetype::StartBody(CreatureType species)
{
	const auto* info = SpeciesInfo(species);
	if (info == nullptr)
	{
		return {};
	}
	return {.alignment = 0.0f, .fatness = info->startFatness, .strength = info->strength};
}

entt::entity CreatureArchetype::Create(const glm::vec3& position, PlayerNames playerName, CreatureType creatureType,
                                       entt::id_type creatureMindId, float yAngleRadians, float scale, const Body& body)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	const auto morph =
	    creature_morph::FromAttributes(body.alignment, body.fatness, body.strength, SpeciesStrength(creatureType));
	const auto size = creature_morph::ClampScale(scale);
	registry.Assign<Creature>(entity, playerName, creatureType, creatureMindId, body.alignment, body.fatness, body.strength,
	                          size);
	// The body is drawn with the base mesh's skins, its shape blended towards the other meshes
	registry.Assign<Mesh>(entity, creature::GetIdFromType(creatureType, CreatureBody::Appearance::Base));
	registry.Assign<CreatureMorph>(entity, CreatureMorph {.shownFatness = body.fatness, .drawn = morph, .revision = 0});
	registry.Assign<CreatureAnimation>(entity);
	registry.Assign<CreatureEyes>(entity);
	registry.Assign<CreatureMindState>(entity);
	registry.Assign<CreatureNeeds>(entity);
	registry.Assign<CreatureHair>(entity);
	registry.Assign<CreatureTattoos>(entity);
	registry.Assign<CreatureMarks>(entity);
	registry.Assign<CreatureSkin>(entity);
	registry.Assign<CreatureLocomotion>(entity);
	registry.Assign<Transform>(entity, position, glm::eulerAngleY(yAngleRadians), glm::vec3(DrawnScale(creatureType, size)));
	return entity;
}
