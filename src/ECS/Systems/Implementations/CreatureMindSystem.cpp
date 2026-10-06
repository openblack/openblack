/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "CreatureMindSystem.h"

#include <cstring>

#include <algorithm>
#include <array>
#include <chrono>
#include <iterator>
#include <random>
#include <ranges>
#include <vector>

#include "3D/CreatureBody.h"
#include "Creature/CreatureDesires.h"
#include "Creature/CreatureIdleMind.h"
#include "Creature/CreatureLayers.h"
#include "Creature/CreatureLook.h"
#include "ECS/Components/Abode.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "InfoConstants.h"
#include "Locator.h"

using namespace openblack;
using namespace openblack::ecs::systems;
using namespace openblack::ecs::components;
namespace animations = openblack::creature_layers::animations;
using creature_desires::Desire;

namespace
{
constexpr float k_TurnSeconds = std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
constexpr float k_TurnsPerSecond = 1.0f / k_TurnSeconds;

/// Made-up rates until the creature eats, sleeps and tires itself out: its energy runs out in ten minutes, and it is
/// fully exhausted in fifteen
constexpr float k_EnergyLossPerSecond = 1.0f / 600.0f;
constexpr float k_ExhaustionPerSecond = 1.0f / 900.0f;
/// The creature wants the player's attention more the longer it is alone, fully after a minute, and from lack of
/// anything to do with the player, fully after two
constexpr float k_LonelySeconds = 60.0f;
constexpr float k_UninterestedSeconds = 120.0f;
/// The desire to show how it is grows with all its desires added up, fully at this much
constexpr float k_ManifestSum = 3.0f;

/// Just woken up, the eyelids droop and blink slowly; otherwise they are as they start
constexpr float k_SleepyOpenness = -0.7f;
constexpr int32_t k_SleepyBlinkIntervalMs = 2500;

/// How high above their feet creatures look at other things
constexpr float k_VillagerHeadHeight = 1.8f;
constexpr float k_AbodeLookHeight = 4.0f;
constexpr float k_TreeLookHeight = 6.0f;
constexpr float k_CitadelLookHeight = 30.0f;

float Clamp01(float value)
{
	return std::clamp(value, 0.0f, 1.0f);
}

/// The 17 per species values of a creature table row
template <typename Row>
float PerSpecies(const Row& row, size_t species)
{
	static_assert(sizeof(Row) == 17 * sizeof(float));
	std::array<float, 17> values {};
	std::memcpy(values.data(), &row, sizeof(values));
	return values.at(std::min(species, values.size() - 1));
}

/// How a species' desires start, from the game's creature tables
std::array<creature_desires::DesireSetup, creature_desires::k_DesireCount> SetupFor(CreatureType species)
{
	std::array<creature_desires::DesireSetup, creature_desires::k_DesireCount> setup {};
	if (!Locator::infoConstants::has_value())
	{
		return setup;
	}
	const auto& info = Locator::infoConstants::value();
	const auto row = creature::InfoRow(species);
	for (size_t d = 0; d < setup.size(); ++d)
	{
		const auto& initial = info.creatureInitialDesire.at(d);
		auto& desire = setup.at(d);
		desire.max = initial.field0x3c;
		desire.decayMin = initial.field0x40;
		desire.decayMax = initial.field0x44;
		desire.increaseSeconds = PerSpecies(info.creatureDesireForType.at(d), row);
		const std::array<uint32_t, creature_desires::k_MaxSources> sourceTypes {
		    initial.field0x0,  initial.field0x4,  initial.field0x8,  initial.field0xc,
		    initial.field0x10, initial.field0x14, initial.field0x18, initial.field0x1c,
		};
		for (const auto type : sourceTypes)
		{
			if (type >= creature_desires::k_NoSource)
			{
				continue;
			}
			desire.sources.push_back({
			    .type = type,
			    .value = PerSpecies(info.creatureInitialSource1.at(type), row),
			    .threshold = PerSpecies(info.creatureInitialSource2.at(type), row),
			    .multiplier = info.desireSourceTable.at(type).field0x8,
			});
		}
	}
	return setup;
}

/// The desires each stage of growing up brings and takes away, from the game's tables
std::vector<creature_desires::PhaseDesires> Phases()
{
	std::vector<creature_desires::PhaseDesires> phases;
	if (!Locator::infoConstants::has_value())
	{
		return phases;
	}
	const auto valid = [](uint32_t desire) { return desire < creature_desires::k_DesireCount; };
	for (const auto& entry : Locator::infoConstants::value().creatureDevelopmentPhaseEntry)
	{
		auto& phase = phases.emplace_back();
		const std::array<uint32_t, 10> add {entry.field0x3c, entry.field0x40, entry.field0x44, entry.field0x48,
		                                    entry.field0x4c, entry.field0x50, entry.field0x54, entry.field0x58,
		                                    entry.field0x5c, entry.field0x60};
		const std::array<uint32_t, 4> remove {entry.field0x64, entry.field0x68, entry.field0x6c, entry.field0x70};
		for (const auto desire : add | std::views::filter(valid))
		{
			phase.add.push_back(static_cast<Desire>(desire));
		}
		for (const auto desire : remove | std::views::filter(valid))
		{
			phase.remove.push_back(static_cast<Desire>(desire));
		}
	}
	return phases;
}

/// The sources that follow the creature's state
std::optional<float> ReadSource(uint32_t type, const creature_desires::Desires& desires, const CreatureMindState& mind)
{
	namespace sources = creature_desires::sources;
	switch (type)
	{
	case sources::k_HungerFromLowEnergy:
		return 1.0f - mind.energy;
	case sources::k_TirednessFromExhaustion:
		return mind.exhaustion;
	case sources::k_FearFromDark:
	case sources::k_TirednessFromNight:
		// TODO: follow the time of day once creatures sleep at night
		return 0.0f;
	case sources::k_AttentionFromLoneliness:
		return Clamp01(mind.secondsAlone / k_LonelySeconds);
	case sources::k_AttentionFromLackOfInteraction:
		return Clamp01(mind.secondsAlone / k_UninterestedSeconds);
	case sources::k_ManifestState:
		return Clamp01(desires.sum / k_ManifestSum);
	case sources::k_AngerFromSadness:
	case sources::k_PlayFromSadness:
	case sources::k_TirednessFromSadness:
		return creature_desires::SourceValue(desires[Desire::Sadness], sources::k_Sadness).value_or(0.0f);
	default:
		return std::nullopt;
	}
}

/// Everything on the land a creature might look at
std::vector<creature_look::Candidate> GatherCandidates(ecs::Registry& registry)
{
	std::vector<creature_look::Candidate> candidates;
	const auto add = [&candidates](entt::entity entity, creature_look::Interest kind, const glm::vec3& point) {
		candidates.push_back({.id = entt::to_integral(entity), .kind = kind, .point = point});
	};
	registry.Each<const Creature, const Transform>([&](entt::entity entity, const Creature& creature, const Transform& at) {
		add(entity, creature_look::Interest::Creature,
		    at.position + glm::vec3(0.0f, creature_look::k_HeadHeight * creature.size, 0.0f));
	});
	registry.Each<const Villager, const Transform>([&](entt::entity entity, const Villager&, const Transform& at) {
		add(entity, creature_look::Interest::Villager, at.position + glm::vec3(0.0f, k_VillagerHeadHeight, 0.0f));
	});
	registry.Each<const Abode, const Transform>([&](entt::entity entity, const Abode&, const Transform& at) {
		add(entity, creature_look::Interest::Abode, at.position + glm::vec3(0.0f, k_AbodeLookHeight, 0.0f));
	});
	registry.Each<const Tree, const Transform>([&](entt::entity entity, const Tree&, const Transform& at) {
		add(entity, creature_look::Interest::Tree, at.position + glm::vec3(0.0f, k_TreeLookHeight, 0.0f));
	});
	registry.Each<const Temple, const Transform>([&](entt::entity entity, const Temple&, const Transform& at) {
		add(entity, creature_look::Interest::Citadel, at.position + glm::vec3(0.0f, k_CitadelLookHeight, 0.0f));
	});
	return candidates;
}

void ApplyEyes(CreatureEyes* eyes, creature_mind::Eyes look)
{
	if (eyes == nullptr || look == creature_mind::Eyes::Unchanged)
	{
		return;
	}
	const bool sleepy = look == creature_mind::Eyes::Sleepy;
	eyes->openness = sleepy ? k_SleepyOpenness : 0.0f;
	eyes->blink.intervalMs = sleepy ? k_SleepyBlinkIntervalMs : creature_eyes::k_BlinkIntervalMs;
}

void Apply(const creature_mind::Commands& commands, CreatureAnimation& animation, CreatureEyes* eyes)
{
	if (commands.endSit)
	{
		animation.body = creature_layers::EndLoop(animation.body);
	}
	if (commands.playOnce.has_value())
	{
		if (auto body = creature_layers::PlayOnce(animation.body, *commands.playOnce, commands.mirrored))
		{
			animation.body = *body;
		}
	}
	if (commands.startSit)
	{
		if (auto body =
		        creature_layers::PlaySequence(animation.body, animations::k_StartSit, animations::k_Sit, animations::k_EndSit))
		{
			animation.body = *body;
		}
	}
	if (commands.face.has_value())
	{
		animation.face.wanted = *commands.face;
	}
	ApplyEyes(eyes, commands.eyes);
}
} // namespace

void CreatureMindSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto random = [this](uint32_t range) {
		return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
	};
	const auto uniform = [this](float low, float high) {
		return high > low ? std::uniform_real_distribution<float>(low, high)(_random) : low;
	};
	std::optional<std::vector<creature_look::Candidate>> candidates;

	registry.Each<const Creature, CreatureMindState, CreatureAnimation, const Transform>(
	    [&](entt::entity entity, const Creature& creature, CreatureMindState& mind, CreatureAnimation& animation,
	        const Transform& transform) {
		    if (!mind.desires.has_value())
		    {
			    mind.desires = creature_desires::Create(SetupFor(creature.species), uniform);
		    }
		    if (mind.desiresPhase != mind.developmentPhase)
		    {
			    creature_desires::ActivateForPhase(*mind.desires, Phases(), mind.developmentPhase);
			    mind.desiresPhase = mind.developmentPhase;
		    }
		    mind.energy = Clamp01(mind.energy - (k_EnergyLossPerSecond * k_TurnSeconds));
		    mind.exhaustion = Clamp01(mind.exhaustion + (k_ExhaustionPerSecond * k_TurnSeconds));
		    mind.secondsAlone += k_TurnSeconds;
		    if (mind.feedbackSeconds.has_value())
		    {
			    *mind.feedbackSeconds += k_TurnSeconds;
		    }
		    creature_desires::UpdateSources(*mind.desires, [&mind](uint32_t type, const creature_desires::Desires& desires) {
			    return ReadSource(type, desires, mind);
		    });
		    creature_desires::UpdateDesires(*mind.desires, k_TurnsPerSecond);

		    auto* eyes = registry.TryGet<CreatureEyes>(entity);
		    if (mind.paused)
		    {
			    return;
		    }
		    const creature_mind::Senses senses {
		        .seconds = k_TurnSeconds,
		        .bodyBusy = creature_layers::IsPlaying(animation.body),
		        .bodyLooping = creature_layers::IsLooping(animation.body),
		        .strongest = creature_desires::StrongestShowable(*mind.desires, creature_mind::k_MinDesireShown),
		        .feedbackSeconds = mind.feedbackSeconds,
		        .feedbackWasStroke = mind.feedbackWasStroke,
		    };
		    const auto commands = creature_mind::Think(mind.idle, senses, random);
		    Apply(commands, animation, eyes);

		    // Looking about, the head turns to the most interesting thing in sight, or ahead
		    mind.lookingAbout = commands.lookAbout;
		    if (commands.lookAbout)
		    {
			    if (!candidates.has_value())
			    {
				    candidates = GatherCandidates(registry);
			    }
			    const auto self = entt::to_integral(entity);
			    std::vector<creature_look::Candidate> others;
			    others.reserve(candidates->size());
			    std::ranges::copy_if(*candidates, std::back_inserter(others),
			                         [self](const creature_look::Candidate& candidate) { return candidate.id != self; });
			    const creature_look::Viewer viewer {
			        .position = transform.position,
			        .ahead = -(transform.rotation * glm::vec3(0.0f, 0.0f, 1.0f)),
			        .size = creature.size,
			    };
			    mind.look = creature_look::LookAbout(mind.look, others, viewer, k_TurnsPerSecond);
			    animation.lookAt = mind.look.id.has_value() ? mind.look.point : creature_look::PointAhead(viewer);
		    }
		    else
		    {
			    animation.lookAt.reset();
		    }
		    if (eyes != nullptr)
		    {
			    eyes->lookAt = animation.lookAt;
		    }
	    });
}

bool CreatureMindSystem::PlayAction(entt::entity creature, size_t animation)
{
	auto* body = Locator::entitiesRegistry::value().TryGet<CreatureAnimation>(creature);
	if (body == nullptr)
	{
		return false;
	}
	const auto played = creature_layers::PlayOnce(body->body, animation, std::bernoulli_distribution(0.5)(_random));
	if (played.has_value())
	{
		body->body = *played;
	}
	return played.has_value();
}

bool CreatureMindSystem::PlayGesture(entt::entity creature, size_t animation)
{
	auto* body = Locator::entitiesRegistry::value().TryGet<CreatureAnimation>(creature);
	if (body == nullptr)
	{
		return false;
	}
	const auto played = creature_layers::PlayGesture(body->gesture, animation);
	if (played.has_value())
	{
		body->gesture = *played;
	}
	return played.has_value();
}

void CreatureMindSystem::PullFace(entt::entity creature, size_t animation)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (auto* body = registry.TryGet<CreatureAnimation>(creature))
	{
		body->face.wanted = animation;
	}
	if (auto* mind = registry.TryGet<CreatureMindState>(creature))
	{
		mind->idle.faceSeconds = creature_mind::k_FaceSeconds;
	}
}

bool CreatureMindSystem::SitDown(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* body = registry.TryGet<CreatureAnimation>(creature);
	auto* mind = registry.TryGet<CreatureMindState>(creature);
	if (body == nullptr || mind == nullptr || creature_layers::IsPlaying(body->body))
	{
		return false;
	}
	// A paused mind won't get it up again: it sits until told to stand
	if (mind->paused)
	{
		body->body =
		    *creature_layers::PlaySequence(body->body, animations::k_StartSit, animations::k_Sit, animations::k_EndSit);
		return true;
	}
	const auto random = [this](uint32_t range) {
		return range == 0 ? 0u : std::uniform_int_distribution<uint32_t>(0, range - 1)(_random);
	};
	creature_mind::Plan(mind->idle, creature_mind::Activity::Told, {creature_mind::SitDown(random)});
	return true;
}

void CreatureMindSystem::Feedback(entt::entity creature, bool stroke)
{
	auto* mind = Locator::entitiesRegistry::value().TryGet<CreatureMindState>(creature);
	if (mind == nullptr)
	{
		return;
	}
	mind->feedbackSeconds = 0.0f;
	mind->feedbackWasStroke = stroke;
	mind->secondsAlone = 0.0f;
	// The game reacts through its planner, which isn't here yet: the creature shows how it feels as soon as it is free
	mind->idle.showDesireSeconds = 0.0f;
	if (mind->idle.activity != creature_mind::Activity::ShowDesire)
	{
		mind->idle.agenda.resize(std::min(mind->idle.agenda.size(), mind->idle.step + (mind->idle.stepStarted ? 1 : 0)));
	}
}

void CreatureMindSystem::StandUp(entt::entity creature)
{
	if (auto* body = Locator::entitiesRegistry::value().TryGet<CreatureAnimation>(creature))
	{
		body->body = creature_layers::EndLoop(body->body);
	}
}
