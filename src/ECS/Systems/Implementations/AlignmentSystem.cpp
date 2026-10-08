/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "AlignmentSystem.h"

#include <algorithm>

#include "3D/CreatureBody.h"
#include "ECS/Components/Alignment.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/Player.h"
#include "ECS/Registry.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/AreaEffect.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;

namespace
{
/// The game moves the sky by a hundredth of a tenth of each millisecond of game time, from 0,
/// good, to 2, evil: a whole of -1 to 1 a second
constexpr float k_SkyTurnPerMillisecond = 0.001f;
} // namespace

std::optional<entt::entity> AlignmentSystem::FindPlayer(PlayerNames player)
{
	std::optional<entt::entity> found;
	Locator::entitiesRegistry::value().Each<const Player>([player, &found](const entt::entity entity, const Player& component) {
		if (component.name == player)
		{
			found = entity;
		}
	});
	return found;
}

float AlignmentSystem::GetPlayerAlignment(PlayerNames player) const
{
	const auto entity = FindPlayer(player);
	if (!entity.has_value())
	{
		return 0.0f;
	}
	const auto* alignment = Locator::entitiesRegistry::value().TryGet<const Alignment>(*entity);
	return alignment != nullptr ? alignment->value : 0.0f;
}

void AlignmentSystem::SetPlayerAlignment(PlayerNames player, float alignment)
{
	if (const auto entity = FindPlayer(player); entity.has_value())
	{
		auto& registry = Locator::entitiesRegistry::value();
		auto* component = registry.TryGet<Alignment>(*entity);
		if (component == nullptr)
		{
			component = &registry.Assign<Alignment>(*entity);
		}
		component->value = std::clamp(alignment, -1.0f, 1.0f);
	}
}

void AlignmentSystem::AddPendingAlignment(PlayerNames player, float change)
{
	if (const auto entity = FindPlayer(player); entity.has_value())
	{
		auto& registry = Locator::entitiesRegistry::value();
		auto* component = registry.TryGet<Alignment>(*entity);
		if (component == nullptr)
		{
			component = &registry.Assign<Alignment>(*entity);
		}
		component->pending += change;
	}
}

float AlignmentSystem::GetPendingAlignment(PlayerNames player) const
{
	const auto entity = FindPlayer(player);
	if (!entity.has_value())
	{
		return 0.0f;
	}
	const auto* alignment = Locator::entitiesRegistry::value().TryGet<const Alignment>(*entity);
	return alignment != nullptr ? alignment->pending : 0.0f;
}

void AlignmentSystem::AddPlayerAlignment(PlayerNames player, float change)
{
	SetPlayerAlignment(player, GetPlayerAlignment(player) + change);
}

void AlignmentSystem::UpdateTurn()
{
	// What the players' and the creatures' deeds changed comes through: each turn the change waiting, held to a whole one,
	// moves the alignment by that share of the owner's change a turn, and the rest is gone
	if (Locator::infoConstants::has_value())
	{
		const auto& info = Locator::infoConstants::value();
		const float change = info.player.maxAlignmentChangePerGameTurn;
		auto& registry = Locator::entitiesRegistry::value();
		registry.Each<Alignment>([change](entt::entity, Alignment& alignment) {
			alignment.value = magic::StepPendingAlignment(alignment.value, alignment.pending, change);
		});
		registry.Each<Creature>([&info](entt::entity, Creature& creature) {
			const auto row = creature::InfoRow(creature.species);
			if (row < info.creature.size())
			{
				creature.alignment = magic::StepPendingAlignment(creature.alignment, creature.pendingAlignment,
				                                                 info.creature.at(row).alignmentChangePerTurn);
			}
		});
	}

	// The game takes the most influential player at the camera's eye, the neutral player where none has any, and
	// keeps their alignment as a goodness from 0 to 1, held there
	// TODO(raffclar): once influence is simulated; until then it is the player's own
	const float goodness = std::clamp((GetPlayerAlignment(PlayerNames::PLAYER_ONE) + 1.0f) * 0.5f, 0.0f, 1.0f);
	_camera = (goodness * 2.0f) - 1.0f;
}

void AlignmentSystem::Update(std::chrono::duration<float, std::milli> gameTime)
{
	const float step = gameTime.count() * k_SkyTurnPerMillisecond;
	_sky = _sky < _camera ? std::min(_sky + step, _camera) : std::max(_sky - step, _camera);
}
