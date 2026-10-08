/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ClipSoundPlayer.h"

#include <string>
#include <string_view>

#include "3D/AllMeshes.h"
#include "3D/L3DAnim.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/ClipSounds.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/SoundGround.h"
#include "ECS/WorldObjects.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
namespace clip_sounds = openblack::audio::clip_sounds;

namespace
{
constexpr std::string_view k_EditorBank = "editor.sad";
constexpr std::string_view k_BanterBank = "VillagersBanter.sad";
/// The alignment key every clip's sound is played with
constexpr int32_t k_ClipSoundAlignment = 2;

/// A clip's name as the animation pack spells it, up to its first null
std::string_view NameOf(const L3DAnim& clip)
{
	const auto& name = clip.GetName();
	return std::string_view(name.c_str());
}
} // namespace

void ecs::clip_sound_player::Play(entt::entity entity, AnimId clipId, const L3DAnim& clip, uint32_t place, uint32_t played,
                                  const glm::vec3& position)
{
	if (played == 0 || !Locator::resources::has_value() || !Locator::audio::has_value())
	{
		return;
	}
	auto& tables = Locator::resources::value().GetClipSounds();
	if (!tables.Contains(clip_sounds::k_TableId.value()))
	{
		return;
	}
	const auto* sounds = tables.Handle(clip_sounds::k_TableId.value())->Find(NameOf(clip));
	if (sounds == nullptr || sounds->sounds.empty())
	{
		return;
	}
	const auto duration = clip.GetPlayTime();
	if (!clip.IsLooping() && place >= duration)
	{
		return;
	}
	const auto passed = clip_sounds::Passed(sounds->sounds, place, played, duration, clip.IsLooping());
	if (passed.empty())
	{
		return;
	}

	auto& registry = Locator::entitiesRegistry::value();
	const auto* villager = registry.TryGet<const Villager>(entity);
	const auto* action = registry.TryGet<const LivingAction>(entity);
	const auto size = clip_sounds::SizeOf(sounds->soundType, villager != nullptr,
	                                      villager != nullptr && villager->lifeStage == Villager::LifeStage::Child,
	                                      villager != nullptr && villager->sex == Villager::Sex::FEMALE);
	const auto surface = creature_audio::SurfaceKey(sound_ground::At(position));
	for (const auto index : passed)
	{
		const auto& sound = sounds->sounds[index];
		// A person's clip sounds only while the person is alive
		if (sounds->soundType == clip_sounds::k_PeopleSounds && world_objects::LifeOf(entity) <= 0.0f)
		{
			return;
		}
		const audio::AnimEffectKeys keys {.size = size,
		                                  .alignment = k_ClipSoundAlignment,
		                                  .object = static_cast<audio::SoundObject>(sounds->soundType),
		                                  .surface = surface,
		                                  .action = static_cast<audio::SoundAction>(sound.action)};
		std::string_view bank = k_EditorBank;
		entt::entity owner = entity;
		glm::vec3 from = position;
		if (sound.action >= clip_sounds::k_HomeBanter && sound.action <= clip_sounds::k_LastBanter)
		{
			bank = k_BanterBank;
			if (sound.action == clip_sounds::k_HomeBanter)
			{
				// Only a villager has a home to banter from
				if (villager == nullptr || !registry.Valid(villager->abode))
				{
					continue;
				}
				owner = villager->abode;
				if (const auto* home = registry.TryGet<const Transform>(owner); home != nullptr)
				{
					from = home->position;
				}
			}
		}
		else if (villager != nullptr && action != nullptr)
		{
			// A thrown person screams only early in its flight
			const auto turns = action->turnsSinceStateChange;
			if ((clipId == AnimId::PThrown && turns >= clip_sounds::k_ThrownSoundTurns) ||
			    (clipId == AnimId::PThrownVortex && turns >= clip_sounds::k_ThrownVortexSoundTurns))
			{
				continue;
			}
		}
		Locator::audio::value().PlayAnimEffect(std::string(bank), keys.ToArray(), owner, from);
	}
}
