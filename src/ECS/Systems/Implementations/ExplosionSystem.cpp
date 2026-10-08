/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "ExplosionSystem.h"

#include <glm/gtx/euler_angles.hpp>

#include "3D/MapCoords.h"
#include "Camera/Camera.h"
#include "Common/GameRandom.h"
#include "ECS/Components/DestructionGhost.h"
#include "ECS/Components/GroundMark.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MorphWithTerrain.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Particles/ParticleBlast.h"
#include "Particles/ParticleDrawFrame.h"
#include "Resources/ResourceManager.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The rubble fades out over its last second
constexpr float k_FadeMilliseconds = 1000.0f;
constexpr float k_AlphaPerMillisecond = 0.255f;
/// The dust puff's size
constexpr float k_DustSize = 1.0f;
constexpr entt::hashed_string k_SmokeSheet = entt::hashed_string("raw/smoke");
constexpr entt::hashed_string k_SmokeSheetAlpha = entt::hashed_string("raw/smokea");

float LocalRandom(float a, float b)
{
	return Locator::gameRandom::has_value() ? a + Locator::gameRandom::value().LocalFloatRand(b - a) : (a + b) * 0.5f;
}

/// The dust's sprites: the smoke sheet, eight cells a row, blended over what is behind
const particles::Creator& DustCreator()
{
	static const particles::Creator creator = [] {
		particles::Creator dust;
		dust.kind = particles::Creator::Kind::Sprite;
		dust.className = "Dust";
		dust.texture = "smoke";
		dust.spritesPerRow = 8;
		dust.numFrames = 64;
		dust.additive = false;
		return dust;
	}();
	return creator;
}
} // namespace

void ExplosionSystem::AddRubble(const glm::vec3& centre, float yaw, float scale)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, centre, glm::mat3(glm::eulerAngleY(yaw)), glm::vec3(scale));
	registry.Assign<Mesh>(entity, resources::HashIdentifier(particles::blast::k_RubbleMesh), static_cast<int8_t>(0),
	                      static_cast<int8_t>(0));
	registry.Assign<GroundMark>(entity, GroundMark {.millisecondsLeft = particles::blast::k_RubbleMilliseconds});
	// It lies over the land's shape
	registry.Assign<MorphWithTerrain>(entity);
	_puffs.push_back(dust_puff::Make(centre, k_DustSize, LocalRandom));
}

void ExplosionSystem::AddShake(const glm::vec3& position, float radius, float strength, float seconds)
{
	const float milliseconds = seconds * 1000.0f;
	_shakes.push_back({.position = position,
	                   .radius = radius,
	                   .strength = strength,
	                   .milliseconds = milliseconds,
	                   .millisecondsLeft = milliseconds});
}

void ExplosionSystem::Update(float milliseconds)
{
	if (Locator::entitiesRegistry::has_value())
	{
		auto& registry = Locator::entitiesRegistry::value();
		std::vector<entt::entity> gone;
		bool fading = false;
		registry.Each<GroundMark>([&](entt::entity entity, GroundMark& mark) {
			// In its last second its alpha is its time left as a share of the second, set before the time passes
			if (mark.millisecondsLeft <= k_FadeMilliseconds)
			{
				// Blending it changes how its model is drawn
				if (!mark.alpha.has_value())
				{
					fading = true;
				}
				mark.alpha = static_cast<uint8_t>(map_coords::FtoL(mark.millisecondsLeft * k_AlphaPerMillisecond));
			}
			mark.millisecondsLeft -= milliseconds;
			if (mark.millisecondsLeft < 1.0f)
			{
				gone.push_back(entity);
			}
		});
		for (const auto entity : gone)
		{
			registry.Destroy(entity);
		}
		if (fading)
		{
			registry.SetDirty();
		}
		// A destroyed building's ghost runs down by the frames' time once drawn, and goes as it reaches nothing
		std::vector<entt::entity> ghosts;
		registry.Each<DestructionGhost>([&](entt::entity entity, DestructionGhost& ghost) {
			if (ghost.shown)
			{
				ghost.millisecondsLeft = std::max(ghost.millisecondsLeft - static_cast<int>(milliseconds), 0);
				if (ghost.millisecondsLeft == 0)
				{
					ghosts.push_back(entity);
				}
			}
			ghost.shown = true;
		});
		for (const auto entity : ghosts)
		{
			registry.Destroy(entity);
		}
	}
	const float seconds = milliseconds / 1000.0f;
	std::erase_if(_puffs, [seconds](dust_puff::Puff& puff) { return !dust_puff::Advance(puff, seconds); });
	camera_shake::Advance(_shakes, milliseconds);
	if (Locator::camera::has_value())
	{
		auto& camera = Locator::camera::value();
		const auto offsets = camera_shake::Jitter(camera_shake::Amplitude(_shakes, camera.GetOrigin()), LocalRandom);
		camera.SetShake(offsets.eye, offsets.focus);
	}
}

void ExplosionSystem::CollectDrawFrame(particles::draw::Frame& frame) const
{
	if (_puffs.empty())
	{
		return;
	}
	const particles::draw::Sources sources {
	    .textures = [](std::string_view /*texture*/) -> std::optional<std::pair<entt::id_type, entt::id_type>> {
		    return std::pair {k_SmokeSheet.value(), k_SmokeSheetAlpha.value()};
	    },
	    .playerColour = {},
	    .random = {},
	};
	const auto rgb = std::array<uint8_t, 3> {static_cast<uint8_t>(dust_puff::k_Colour >> 16u),
	                                         static_cast<uint8_t>((dust_puff::k_Colour >> 8u) & 0xFFu),
	                                         static_cast<uint8_t>(dust_puff::k_Colour & 0xFFu)};
	particles::Effect::DrawWalk walk;
	for (const auto& puff : _puffs)
	{
		walk.Clear();
		for (const auto& sprite : dust_puff::Look(puff))
		{
			walk.steps.push_back({.chain = false, .index = static_cast<uint32_t>(walk.atoms.size())});
			walk.atoms.push_back({
			    .creator = &DustCreator(),
			    .position = sprite.position,
			    // The sprite's roll is read from the rotation about the vertical
			    .rotation = glm::mat3(glm::eulerAngleY(-sprite.angle)),
			    .scale = sprite.halfWidth,
			    .stretch = 1.0f,
			    .alpha = sprite.alpha,
			    .frame = static_cast<float>(sprite.cell),
			    .rgb = rgb,
			});
		}
		particles::draw::AddEffect(frame, walk, particles::draw::DrawPath::Sorted, puff.positions.front(),
		                           particles::draw::k_NeutralPlayer, sources);
	}
}

void ExplosionSystem::Reset()
{
	_shakes.clear();
	_puffs.clear();
	if (Locator::camera::has_value())
	{
		Locator::camera::value().SetShake(0.0f, 0.0f);
	}
	if (Locator::entitiesRegistry::has_value())
	{
		auto& registry = Locator::entitiesRegistry::value();
		std::vector<entt::entity> marks;
		registry.Each<GroundMark>([&](entt::entity entity, GroundMark&) { marks.push_back(entity); });
		registry.Each<DestructionGhost>([&](entt::entity entity, DestructionGhost&) { marks.push_back(entity); });
		registry.Destroy(marks.begin(), marks.end());
	}
}
