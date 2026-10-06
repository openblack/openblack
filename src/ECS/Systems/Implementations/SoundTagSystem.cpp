/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "SoundTagSystem.h"

#include <vector>

#include <glm/geometric.hpp>

#include "Audio/AudioManagerInterface.h"
#include "ECS/Components/SoundTag.h"
#include "ECS/Components/Transform.h"
#include "ECS/Registry.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// Units a second sound travels at
constexpr float k_SpeedOfSound = 347.0f;
/// Seconds of a game turn
constexpr float k_TurnSeconds = 0.1f;

void StopSound(SoundTag& tag)
{
	if (tag.emitter == entt::null)
	{
		return;
	}
	if (Locator::audio::has_value())
	{
		auto& audio = Locator::audio::value();
		if (audio.EmitterExists(tag.emitter))
		{
			audio.StopEmitter(tag.emitter);
			audio.DestroyEmitter(tag.emitter);
		}
	}
	tag.emitter = entt::null;
}

bool IsPlaying(const SoundTag& tag)
{
	auto& audio = Locator::audio::value();
	return tag.emitter != entt::null && audio.EmitterExists(tag.emitter) &&
	       audio.GetStatus(tag.emitter) != audio::AudioStatus::Stopped;
}

/// Whether a point is within a sound's reach of the camera, a reach of none meaning anywhere
bool WithinReach(float reach, const glm::vec3& position, const glm::vec3& camera)
{
	const auto offset = position - camera;
	return reach <= 0.0f || glm::dot(offset, offset) <= reach * reach;
}
} // namespace

void SoundTagSystem::ProcessTurn(const glm::vec3& camera)
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	auto& audio = Locator::audio::value();
	const auto& sounds = Locator::resources::value().GetSounds();
	std::vector<entt::entity> finished;
	Locator::entitiesRegistry::value().Each<SoundTag, const Transform>(
	    [&](entt::entity entity, SoundTag& tag, const Transform& transform) {
		    ++tag.turns;
		    if (!sounds.Contains(tag.sound))
		    {
			    if (tag.point)
			    {
				    finished.push_back(entity);
			    }
			    return;
		    }
		    const auto position = transform.position + tag.offset;
		    const auto reach = sounds.Handle(tag.sound)->maxDistance;
		    if (tag.point)
		    {
			    if (tag.delayed)
			    {
				    // Not heard until the sound has travelled as far as the camera, and then only within its reach
				    const auto travelled = k_SpeedOfSound * static_cast<float>(tag.turns) * k_TurnSeconds;
				    const auto offset = position - camera;
				    if (travelled * travelled < glm::dot(offset, offset))
				    {
					    return;
				    }
				    if (tag.active && WithinReach(reach, position, camera))
				    {
					    tag.emitter = audio.CreateEmitter(tag.sound, position, audio::PlayType::Once);
					    if (tag.emitter != entt::null)
					    {
						    audio.PlayEmitter(tag.emitter);
					    }
				    }
				    tag.delayed = false;
				    return;
			    }
			    if (!IsPlaying(tag))
			    {
				    finished.push_back(entity);
			    }
			    return;
		    }
		    if (!tag.active || IsPlaying(tag) || !WithinReach(reach, position, camera))
		    {
			    return;
		    }
		    StopSound(tag);
		    tag.emitter = audio.CreateEmitter(tag.sound, position, audio::PlayType::Repeat);
		    if (tag.emitter != entt::null)
		    {
			    audio.PlayEmitter(tag.emitter);
		    }
	    });
	auto& registry = Locator::entitiesRegistry::value();
	for (const auto entity : finished)
	{
		StopSound(registry.Get<SoundTag>(entity));
		registry.Destroy(entity);
	}
}

void SoundTagSystem::SetActive(entt::entity entity, bool active)
{
	auto* tag = Locator::entitiesRegistry::value().TryGet<SoundTag>(entity);
	if (tag == nullptr)
	{
		return;
	}
	tag->active = active;
	if (!active)
	{
		StopSound(*tag);
	}
}

entt::entity SoundTagSystem::CreatePointSound(entt::id_type sound, const glm::vec3& position, bool delayed)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto entity = registry.Create();
	registry.Assign<Transform>(entity, position, glm::mat3(1.0f), glm::vec3(1.0f));
	registry.Assign<SoundTag>(entity, SoundTag {
	                                      .sound = sound,
	                                      .offset = glm::vec3(0.0f),
	                                      .active = true,
	                                      .emitter = entt::null,
	                                      .point = true,
	                                      .delayed = delayed,
	                                      .turns = 0,
	                                  });
	return entity;
}
