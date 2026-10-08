/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// What a creature's mind does about the miracles cast near it: it runs from a nasty one or goes to look at it, goes to
// look at a nice one, and learns them by doing so

#define LOCATOR_IMPLEMENTATIONS

#include <fmt/format.h>

#include "3D/CreatureBody.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureMindModel.h"
#include "Creature/CreatureMindTables.h"
#include "Creature/CreatureMiracleReactions.h"
#include "Creature/CreatureWatching.h"
#include "CreatureMindSystem.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Registry.h"
#include "ECS/Systems/LeashSystemInterface.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// A creature of size 1 is about this tall
constexpr float k_HeightOfSizeOne = 15.0f;
/// Seeing a nasty miracle adds this much to its fear
constexpr float k_FearOfScaryMagic = 0.5f;
/// The magic type no creature learns by watching
constexpr size_t k_NotLearnt = static_cast<size_t>(MagicType::LightningBoltPowerUpTwo);
} // namespace

void CreatureMindSystem::WatchMiracle(entt::entity creature, size_t miracle)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* tables = GetTables();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	// A mind stilled, as by the freeze spell, learns nothing
	if (tables == nullptr || mind == nullptr || body == nullptr || !mind->learnt.has_value() || mind->paused ||
	    miracle == k_NotLearnt || miracle >= tables->miracles.size())
	{
		return;
	}
	const auto progress = creature_watching::SeeMiracle(
	    mind->learnt->knowledge, miracle, tables->miracles, mind->developmentPhase, mind->turn,
	    mind->leash.miracleSightingWeight, creature_mind_tables::MiracleMultiplier(creature::InfoRow(body->species)));
	if (progress.learnt && !progress.ignored)
	{
		creature_mind_model::Think(*mind->learnt, fmt::format("I've learnt the miracle {}", tables->miracles.at(miracle).name));
	}
}

void CreatureMindSystem::ReactToNastyMagic(entt::entity creature, const glm::vec3& point, std::optional<size_t> learn)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	const auto* body = registry.TryGet<const Creature>(creature);
	if (mind == nullptr || body == nullptr || !mind->desires.has_value())
	{
		return;
	}
	auto& desires = *mind->desires;
	creature_desires::ChangeSource(desires, creature_desires::sources::k_FearFromScaryMagic, k_FearOfScaryMagic);
	// Not afraid, or on the learning leash, it goes to look; otherwise it runs
	const bool onRope = Locator::leashSystem::has_value() && Locator::leashSystem::value().IsLeashed(creature) &&
	                    Locator::leashSystem::value().TypeOf(creature) == LeashType::Rope;
	const bool curious = onRope || !(desires[creature_desires::Desire::Fear].value > 0.0f);
	const float height = body->size * k_HeightOfSizeOne;
	const auto random = [this](uint32_t range) { return Random(range); };
	const glm::vec2 at(point.x, point.z);
	Replan(creature, creature_mind::Activity::Planned,
	       curious ? creature_mind::ExamineMiracle(at, height, random) : creature_mind::RunAwayFromMiracle(at, height, random));
	if (learn.has_value())
	{
		WatchMiracle(creature, *learn);
	}
}

void CreatureMindSystem::ReactToNiceMagic(entt::entity creature, const glm::vec3& point, std::optional<size_t> learn)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	if (body == nullptr)
	{
		return;
	}
	const auto random = [this](uint32_t range) { return Random(range); };
	Replan(creature, creature_mind::Activity::Planned,
	       creature_mind::ExamineMiracle({point.x, point.z}, body->size * k_HeightOfSizeOne, random));
	if (learn.has_value())
	{
		WatchMiracle(creature, *learn);
	}
}
