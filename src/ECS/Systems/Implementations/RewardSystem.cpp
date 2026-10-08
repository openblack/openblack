/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "RewardSystem.h"

#include <vector>

#include <entt/core/hashed_string.hpp>
#include <glm/gtx/euler_angles.hpp>

#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Reward.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "ECS/RewardRules.h"
#include "ECS/Systems/ExplosionSystemInterface.h"
#include "Locator.h"
#include "Particles/ParticleBlast.h"
#include "Particles/ParticleDrawFrame.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The closed chest's model, and the thump of one landing, heard alike wherever the camera is
constexpr entt::hashed_string k_ChestMesh = entt::hashed_string("misc/chest0");
constexpr entt::hashed_string k_LandingSound = entt::hashed_string("InGame.sad/174");
/// The smoke sheet its dust is drawn from
constexpr entt::hashed_string k_SmokeSheet = entt::hashed_string("raw/smoke");
constexpr entt::hashed_string k_SmokeSheetAlpha = entt::hashed_string("raw/smokea");
/// The landing shakes a camera only near the middle of the world's first corner: the game shakes about the world's
/// origin rather than the chest
constexpr glm::vec3 k_ShakeCentre {0.0f};

float LocalRandom(float a, float b)
{
	return Locator::gameRandom::has_value() ? a + Locator::gameRandom::value().LocalFloatRand(b - a) : (a + b) * 0.5f;
}

float LandHeight(glm::vec3 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt({point.x, point.z}) : 0.0f;
}

particles::Creator MakeDustCreator()
{
	particles::Creator dust;
	dust.kind = particles::Creator::Kind::Sprite;
	dust.className = "RewardDust";
	dust.texture = "smoke";
	dust.spritesPerRow = 8;
	dust.numFrames = 64;
	dust.additive = false;
	return dust;
}
} // namespace

entt::entity RewardSystem::Create(glm::vec3 position, RewardObjectInfo type, std::optional<PlayerNames> player,
                                  entt::entity town, bool fromSky)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	// Given on the land it stands at the height asked for; from the sky it falls to the land itself
	const glm::vec3 ground {position.x, LandHeight(position), position.z};
	registry.Assign<Transform>(entity, fromSky ? ground + glm::vec3(0.0f, reward::k_FallFrom, 0.0f) : position, glm::mat3(1.0f),
	                           glm::vec3(1.0f));
	registry.Assign<Mesh>(entity, k_ChestMesh.value(), static_cast<int8_t>(0), static_cast<int8_t>(1));
	auto& chest = registry.Assign<Reward>(entity);
	chest.type = type;
	chest.player = player;
	chest.town = town;
	chest.landingPoint = ground;
	chest.madeAt = position;
	// Its dust is made with it, and only shown once it has fallen
	chest.dust = reward::MakeDust(LocalRandom);
	if (fromSky)
	{
		chest.state = Reward::State::Falling;
		chest.dustMilliseconds = reward::k_DustMilliseconds;
	}
	else
	{
		// Put on the land, it stands in the map's cells at once
		chest.state = Reward::State::Landed;
		registry.Assign<RewardOnLand>(entity);
	}
	return entity;
}

void RewardSystem::Update(float milliseconds)
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	registry.Each<Reward, Transform>([milliseconds](Reward& chest, Transform& transform) {
		if (chest.state == Reward::State::Landed)
		{
			// Its dust fades once it has fallen
			chest.dustMilliseconds = std::max(chest.dustMilliseconds - milliseconds, 0.0f);
			return;
		}
		// It is placed by its clock as it stood before this frame, which then runs on
		const float height = reward::FallHeight(chest.seconds);
		transform.rotation = glm::mat3(glm::eulerAngleY(-reward::FallYaw(chest.seconds)));
		chest.seconds += milliseconds * 0.001f;
		if (height > 0.0f)
		{
			transform.position = chest.landingPoint + glm::vec3(0.0f, height, 0.0f);
			return;
		}
		// It thumps down on the land, shaking a camera near the world's origin
		transform.position = chest.landingPoint;
		chest.state = Reward::State::Landed;
		chest.goesIntoMap = true;
		if (Locator::audio::has_value())
		{
			Locator::audio::value().PlaySoundEffect(k_LandingSound.value(), std::nullopt);
		}
		if (Locator::explosionSystem::has_value())
		{
			Locator::explosionSystem::value().AddShake(k_ShakeCentre, reward::k_ShakeRadius, reward::k_ShakeStrength,
			                                           reward::k_ShakeSeconds, false);
		}
	});
}

void RewardSystem::ProcessTurn()
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	auto& registry = Locator::entitiesRegistry::value();
	std::vector<entt::entity> landed;
	registry.Each<Reward>([&landed](entt::entity entity, Reward& chest) {
		if (chest.goesIntoMap)
		{
			chest.goesIntoMap = false;
			landed.push_back(entity);
		}
	});
	// A chest that landed goes into the map's cells the turn after
	for (const auto entity : landed)
	{
		registry.AssignOrReplace<RewardOnLand>(entity);
	}
}

void RewardSystem::CollectDrawFrame(particles::draw::Frame& frame) const
{
	if (!Locator::entitiesRegistry::has_value())
	{
		return;
	}
	const auto creator = MakeDustCreator();
	const particles::draw::Sources sources {
	    .textures = [](std::string_view /*texture*/) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    return std::pair {k_SmokeSheet.value(), k_SmokeSheetAlpha.value()};
	    },
	    .playerColour = {},
	    .random = {},
	};
	particles::Effect::DrawWalk walk;
	Locator::entitiesRegistry::value().Each<const Reward>([&](const Reward& chest) {
		// Only a chest that fell shows its dust, while its time lasts
		if (chest.state != Reward::State::Landed || chest.dustMilliseconds <= 0.0f)
		{
			return;
		}
		const auto look = reward::LookOfDust(chest.dustMilliseconds);
		walk.Clear();
		for (const auto& sprite : chest.dust)
		{
			walk.steps.push_back({.chain = false, .index = static_cast<uint32_t>(walk.atoms.size())});
			walk.atoms.push_back({
			    .creator = &creator,
			    .position = chest.madeAt + sprite.offset,
			    .rotation = glm::mat3(1.0f),
			    .scale = look.size * 0.5f,
			    .stretch = 1.0f,
			    .alpha = static_cast<float>(look.alpha),
			    .frame = static_cast<float>(sprite.frame),
			    .rgb = sprite.rgb,
			});
		}
		particles::draw::AddEffect(frame, walk, particles::draw::DrawPath::Sorted, chest.madeAt,
		                           particles::draw::k_NeutralPlayer, sources);
	});
}
