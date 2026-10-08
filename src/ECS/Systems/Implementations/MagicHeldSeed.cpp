/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// The seed in the player's hand as the hand shows it. Until it is ready the hand holds it as a miracle not yet ready,
// shows no model and draws nothing of its in-hand effect, which runs all the same; once ready it is held as its record
// says, its model (for the seeds that show one) sits in the hand and the effect shows, centred in the fingers. A tribe's
// power behind a miracle it casts is announced.

#define LOCATOR_IMPLEMENTATIONS

#include <string>

#include <entt/core/hashed_string.hpp>
#include <fmt/format.h>

#include "Audio/AudioManagerInterface.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/SpellSeed.h"
#include "ECS/Registry.h"
#include "ECS/Systems/MiracleFxSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/HandHoldPose.h"
#include "Magic/MagicTables.h"
#include "MagicSystem.h"
#include "Particles/ParticleEffect.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// The announcer's line for a tribe's power is this sample of the spell dialogue, plus the tribe
constexpr int k_TribalPowerVoice = 27;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}
} // namespace

void MagicSystem::ShowHeldSeed()
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held) || !Locator::infoConstants::has_value())
	{
		return;
	}
	const auto entity = *_held;
	auto& seed = registry.Get<SpellSeed>(entity);
	const auto& seedInfo = magic::GetSpellSeedInfo(Info(), seed.seedType);
	seed.holdType = magic::hand_hold::HoldTypeOf(seed.ready, seedInfo.holdType);

	// Its model is drawn only once it is ready, and only for a miracle that shows one in the hand
	const auto type = magic::GetMagicTypeFromPowerUpLevel(seedInfo, seed.powerUp);
	const bool drawn = seed.ready && magic::GetMagicInfo(Info(), type).isSpellSeedDrawnInHand == 1;
	if (drawn && !registry.AllOf<Mesh>(entity))
	{
		registry.Assign<Mesh>(entity, resources::HashIdentifier(seedInfo.mesh), static_cast<int8_t>(0), static_cast<int8_t>(0));
	}
	else if (!drawn && registry.AllOf<Mesh>(entity))
	{
		registry.Remove<Mesh>(entity);
	}

	if (seed.handEffect != ParticleSystemInterface::k_NoEffect && Locator::particleSystem::has_value())
	{
		if (auto* effect = Locator::particleSystem::value().Find(seed.handEffect))
		{
			effect->SetHidden(!seed.ready);
		}
	}
}

particles::ProcessInfo MagicSystem::HandEffectInfo(const SpellSeed& seed) const
{
	auto info = HandInfo();
	info.handPosition = _handEffectPoint.value_or(_hand.handPosition);
	info.power = seed.power;
	return info;
}

void MagicSystem::PlaceHandEffect(glm::vec3 point, float handScale)
{
	_handEffectPoint = point;
	_handScale = handScale;
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held) || !Locator::particleSystem::has_value())
	{
		return;
	}
	const auto& seed = registry.Get<const SpellSeed>(*_held);
	if (seed.handEffect == ParticleSystemInterface::k_NoEffect)
	{
		return;
	}
	auto& particles = Locator::particleSystem::value();
	// The effect is as large as the hand is drawn, every frame, and is drawn where its point has moved to since it stepped
	if (auto* effect = particles.Find(seed.handEffect))
	{
		effect->SetMagnitude(handScale);
	}
	particles.SetDrawOffset(seed.handEffect, point - _handAtStep);
}

bool MagicSystem::PlayerHasSpellIcon(PlayerNames /*player*/, SpellSeedType /*seed*/) const
{
	// A player's icons stand on the worship sites of their citadel, one for each miracle they have learnt; a seed taken
	// from a globe is made at the best icon the player has for it. openblack's citadels have no worship sites yet, so
	// no player has an icon and a globe's seed is always made without one.
	return false;
}

bool MagicSystem::IsHandInInfluence() const
{
	auto& registry = EntityRegistry();
	if (!_held.has_value() || !registry.Valid(*_held))
	{
		return false;
	}
	if (_ignoreInfluence)
	{
		return true;
	}
	const auto& seed = registry.Get<const SpellSeed>(*_held);
	return _hand.point.has_value() && _world.InInfluence(seed.player, *_hand.point);
}

std::optional<Tribe> MagicSystem::TribalPowerTribe(PlayerNames player, MagicType type) const
{
	return magic::GetTribalPowerTribe(magic::GetMagicEffectInfo(Info(), type), PlayerTribalMultipliers(player));
}

void MagicSystem::AfterSeedCast(const SpellSeed& seed, MagicType type)
{
	const auto tribe = TribalPowerTribe(seed.player, type);
	if (!tribe.has_value())
	{
		return;
	}
	// The name round the hand is let go and rises from where the hand is
	if (Locator::miracleFxSystem::has_value())
	{
		Locator::miracleFxSystem::value().ReleaseTribalPowerRing(*tribe, _hand.handPosition);
	}
	if (!Locator::audio::has_value())
	{
		return;
	}
	// The announcer names the tribe whose power is behind the miracle
	const auto sample = k_TribalPowerVoice + static_cast<int>(*tribe);
	Locator::audio::value().StartSoundEffect(entt::hashed_string(fmt::format("SpellDialogue.sad/{}", sample).c_str()).value(),
	                                         {});
}
