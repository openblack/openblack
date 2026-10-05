/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "StreetLanternArchetype.h"

#include "3D/AllMeshes.h"
#include "3D/VillageLights.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/VillageLight.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs::archetypes;
using namespace openblack::ecs::components;

entt::entity StreetLanternArchetype::Create(const glm::vec3& position, MobileStaticInfo info)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	// It stands as it was made, unturned
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(1.0f));
	const bool country = info != MobileStaticInfo::StreetLantern;
	const auto resourceId = resources::HashIdentifier(country ? MeshId::BuildingCampfire : MeshId::ObjectTownLight);
	registry.Assign<Mesh>(entity, resourceId, static_cast<int8_t>(0), static_cast<int8_t>(1));
	// Its light starts flickering at random, from the C runtime's numbers
	const auto flickerTimer = Locator::gameRandom::value().CrtRandom(0.0f, village_lights::k_FlickerMilliseconds);
	registry.Assign<VillageLight>(entity, VillageLight {
	                                          .kind = country ? VillageLight::Kind::Country : VillageLight::Kind::Town,
	                                          .flickerTimer = flickerTimer,
	                                          .flicker = glm::vec2(0.0f),
	                                          .glowSize = village_lights::k_GlowSize,
	                                      });
	return entity;
}
