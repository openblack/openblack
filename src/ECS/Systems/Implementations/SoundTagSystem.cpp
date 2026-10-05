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
} // namespace

void SoundTagSystem::ProcessTurn(const glm::vec3& camera)
{
	if (!Locator::audio::has_value())
	{
		return;
	}
	auto& audio = Locator::audio::value();
	const auto& sounds = Locator::resources::value().GetSounds();
	Locator::entitiesRegistry::value().Each<SoundTag, const Transform>([&](SoundTag& tag, const Transform& transform) {
		if (!tag.active || !sounds.Contains(tag.sound))
		{
			return;
		}
		if (tag.emitter != entt::null && audio.EmitterExists(tag.emitter) &&
		    audio.GetStatus(tag.emitter) != audio::AudioStatus::Stopped)
		{
			return;
		}
		// It only starts within the sound's reach of the camera, a reach of none meaning anywhere
		const auto position = transform.position + tag.offset;
		const auto offset = position - camera;
		const auto reach = sounds.Handle(tag.sound)->maxDistance;
		if (reach > 0.0f && glm::dot(offset, offset) > reach * reach)
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
