/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "GameFireflyWorld.h"

#include <cmath>

#include <entt/core/hashed_string.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "3D/DayNightClock.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "3D/SkyInterface.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/Field.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/SpellDispenser.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Map.h"
#include "ECS/ObjectPhysics.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MagicSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Graphics/Texture2D.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/MagicTables.h"
#include "Nature/Fireflies.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The sprite sheet the fireflies' light is on, and its alpha
constexpr entt::hashed_string k_Sheet = entt::hashed_string("raw/S_SpriteSheet3");
constexpr entt::hashed_string k_SheetAlpha = entt::hashed_string("raw/S_SpriteSheet3a");
/// The game's turn counts in milliseconds; a flight in seconds
constexpr float k_SecondsPerMillisecond = 0.001f;
} // namespace

ecs::Registry& GameFireflyWorld::Entities()
{
	return Locator::entitiesRegistry::value();
}

uint32_t GameFireflyWorld::GameRand(uint32_t n)
{
	return Locator::gameRandom::value().GameRand(n);
}

float GameFireflyWorld::GameFloatRand(float x)
{
	return Locator::gameRandom::value().GameFloatRand(x);
}

float GameFireflyWorld::VisualHour() const
{
	return Locator::skySystem::value().GetClock().GetVisualTime();
}

std::array<float, 4> GameFireflyWorld::SkyHours() const
{
	return Locator::skySystem::value().GetClock().GetVisualTimes();
}

float GameFireflyWorld::TurnSeconds() const
{
	return static_cast<float>(TimeSystemInterface::k_TurnDuration.count()) * k_SecondsPerMillisecond;
}

glm::vec3 GameFireflyWorld::ToWorld(const map_coords::MapCoords& coords) const
{
	return map_coords::ToWorld(Locator::terrainSystem::value(), coords);
}

map_coords::MapCoords GameFireflyWorld::FromWorld(glm::vec3 point) const
{
	return map_coords::FromWorld(Locator::terrainSystem::value(), point);
}

std::vector<entt::entity> GameFireflyWorld::ThingsInCell(glm::ivec2 cell) const
{
	if (!Locator::entitiesMap::has_value())
	{
		return {};
	}
	return Locator::entitiesMap::value().GetAllInCell(cell);
}

std::optional<map_coords::MapCoords> GameFireflyWorld::SpotOf(entt::entity thing) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	if (!registry.Valid(thing))
	{
		return std::nullopt;
	}
	const auto* transform = registry.TryGet<const Transform>(thing);
	if (transform == nullptr || !Locator::terrainSystem::has_value())
	{
		return std::nullopt;
	}
	return FromWorld(transform->position);
}

bool GameFireflyWorld::IsHidingPlace(entt::entity thing) const
{
	return Locator::entitiesRegistry::value().AnyOf<Tree, DeadTree>(thing) || IsRock(thing);
}

bool GameFireflyWorld::IsRock(entt::entity thing) const
{
	return object_physics::IsRock(thing);
}

bool GameFireflyWorld::IsBuilding(entt::entity thing) const
{
	// Every building of a town counts, its fields too; street lanterns never do
	return Locator::entitiesRegistry::value().AnyOf<Abode, Field, SpellDispenser>(thing);
}

float GameFireflyWorld::HeightOf(entt::entity thing) const
{
	// As tall as its model's box at its size; nothing for a thing without a model
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* transform = registry.TryGet<const Transform>(thing);
	const auto* mesh = registry.TryGet<const Mesh>(thing);
	if (transform == nullptr || mesh == nullptr || !Locator::resources::has_value())
	{
		return 0.0f;
	}
	auto& meshes = Locator::resources::value().GetMeshes();
	if (!meshes.Contains(mesh->id))
	{
		return 0.0f;
	}
	return meshes.Handle(mesh->id)->GetBoundingBox().Size().y * transform->scale.y;
}

std::optional<size_t> GameFireflyWorld::MagicKindNamed(std::string_view name) const
{
	if (!Locator::infoConstants::has_value())
	{
		return std::nullopt;
	}
	const auto type = magic::FindMagicTypeByName(Locator::infoConstants::value(), name);
	return type.has_value() ? std::optional(static_cast<size_t>(*type)) : std::nullopt;
}

void GameFireflyWorld::MakeReward(size_t kind, const map_coords::MapCoords& spot)
{
	if (!Locator::infoConstants::has_value() || !Locator::magicSystem::has_value() || kind >= magic::k_MagicTypeCount)
	{
		return;
	}
	const auto& info = Locator::infoConstants::value();
	const auto type = static_cast<MagicType>(kind);
	// The miracle's seed is the first seed that casts it, as the game links them once its tables are read; at the
	// power-up that casts it, and only if the seed is a real one
	const auto seed = magic::FindFirstSpellSeedForMagicType(info, type);
	if (!seed.has_value())
	{
		return;
	}
	const auto& seedInfo = magic::GetSpellSeedInfo(info, *seed);
	if (seedInfo.exists == 0)
	{
		return;
	}
	Locator::magicSystem::value().CreateOneOffSeed(ToWorld(spot), *seed, magic::GetPowerUpFromMagicType(seedInfo, type), 1.0f);
}

glm::vec3 GameFireflyWorld::CameraPosition() const
{
	return Locator::camera::value().GetOrigin();
}

bool GameFireflyWorld::InView(glm::vec3 point) const
{
	const glm::vec4 clip = Locator::camera::value().GetViewProjectionMatrix() * glm::vec4(point, 1.0f);
	return clip.w > 0.0f && std::abs(clip.x) <= clip.w && std::abs(clip.y) <= clip.w;
}

std::optional<Sprite> GameFireflyWorld::Look() const
{
	if (!Locator::resources::has_value())
	{
		return std::nullopt;
	}
	auto& textures = Locator::resources::value().GetTextures();
	if (!textures.Contains(k_Sheet.value()) || !textures.Contains(k_SheetAlpha.value()))
	{
		return std::nullopt;
	}
	// One picture of the sheet's 8 by 8, white, added to what is behind it by its alpha
	constexpr float k_Cell = 1.0f / static_cast<float>(fireflies::k_SheetPictures);
	const auto column = static_cast<float>(fireflies::k_SpritePicture % fireflies::k_SheetPictures);
	const auto row = static_cast<float>(fireflies::k_SpritePicture / fireflies::k_SheetPictures);
	return Sprite {
	    .texture = textures.Handle(k_Sheet.value())->GetNativeHandle(),
	    .uvMin = glm::vec2(column, row) * k_Cell,
	    .uvExtent = glm::vec2(k_Cell),
	    .tint = glm::vec4(1.0f),
	    .additive = true,
	    .facesCamera = true,
	    .alpha = textures.Handle(k_SheetAlpha.value())->GetNativeHandle(),
	};
}
