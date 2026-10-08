/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "VillagerReactions.h"

#include <cmath>

#include <algorithm>
#include <array>
#include <numbers>
#include <optional>

#include <fmt/format.h>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/InfluenceCircle.h"
#include "3D/L3DMesh.h"
#include "Audio/AudioManagerInterface.h"
#include "Common/GameRandom.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/LivingReaction.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/MiracleImpression.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/TownDesire.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Components/WallHug.h"
#include "ECS/Registry.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/LivingActionSystemInterface.h"
#include "ECS/Systems/ParticleSystemInterface.h"
#include "ECS/Systems/ReactionSystemInterface.h"
#include "ECS/Systems/TeleportSystemInterface.h"
#include "ECS/TownDesire.h"
#include "ECS/VillagerSpeed.h"
#include "InfoConstants.h"
#include "Locator.h"
#include "Magic/ReactionRules.h"
#include "MagicLiving.h"
#include "Resources/ResourceManager.h"
#include "Resources/ResourcesInterface.h"
#include "VillagerFire.h"
#include "VillagerHome.h"
#include "VillagerPhysics.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;

namespace
{
/// A villager's health out of this is its life
constexpr float k_VillagerHealthScale = 100.0f;
/// A watching villager turns this much a turn towards what it watches: an eighth of a half turn
constexpr float k_WatchTurn = std::numbers::pi_v<float> / 8.0f;

/// The share a town takes is padded by this, so that an empty town takes a share at all
constexpr float k_BeliefShareSmall = 0.001f;
/// A symbol of belief is worth the belief by this many
constexpr float k_BeliefSpriteScale = 10000.0f;
/// The guidance's voices of a town's awe, good and evil, the most awed first, by their place in its sound bank
constexpr std::array<uint32_t, 3> k_GoodAwe {187, 188, 189};
constexpr std::array<uint32_t, 3> k_EvilAwe {174, 175, 176};
/// A voice of awe is heard no sooner than this many turns after the last, and up to this many more
constexpr uint32_t k_VoiceLeastGap = 30;
constexpr float k_VoiceGapSpread = 150.0f;
/// The guidance's first words seed their clock up to this many turns back
constexpr uint32_t k_GuidanceLeastGap = 50;
/// A voice is wanted less within this many turns of the guidance speaking
constexpr float k_GuidanceQuietTurns = 50.0f;
/// The belief shares are padded by this
constexpr float k_VoiceShareSmall = 0.0001f;
/// A voice is heard within this many metres of the hand, fully within a third of it, and only when wanted more than this
constexpr float k_VoiceReach = 200.0f;
constexpr float k_VoiceNearShare = 0.333f;
constexpr float k_VoiceWanted = 0.3f;
constexpr uint32_t k_VoicePitch = 100;

ecs::Registry& EntityRegistry()
{
	return Locator::entitiesRegistry::value();
}

const GVillagerInfo& InfoOf(const Villager& villager)
{
	const auto& infos = Locator::infoConstants::value().villager;
	return infos.at(static_cast<size_t>(GVillagerInfo::Find(villager.tribe, villager.number)));
}

/// The land balances of how fast villagers go and how belief speeds them
constexpr size_t k_SpeedBalance = 4;
constexpr size_t k_BeliefSpeedBalance = 7;

/// The state a villager is to end up in: the state it is in if that is an end in itself, else where it is going
VillagerStates FinalStateOf(const LivingAction& action);

const GVillagerStateTableInfo& StateInfo(VillagerStates state)
{
	return Locator::infoConstants::value().villagerStateTable.at(static_cast<size_t>(state));
}

VillagerStates FinalStateOf(const LivingAction& action)
{
	const auto top = static_cast<VillagerStates>(action.states.at(static_cast<size_t>(LivingAction::Index::Top)));
	return StateInfo(top).isFinalState != 0
	           ? top
	           : static_cast<VillagerStates>(action.states.at(static_cast<size_t>(LivingAction::Index::Final)));
}

VillagerStates StateOf(const LivingAction& action, LivingAction::Index index)
{
	return Locator::livingActionSystem::value().VillagerGetState(action, index);
}

void SetTop(LivingAction& action, VillagerStates state)
{
	Locator::livingActionSystem::value().VillagerSetState(action, LivingAction::Index::Top, state, false);
}

/// What the villager reacts to and where it is now, none once its reaction has gone
std::optional<ReactionSystemInterface::Active> ReactionOf(entt::entity villager)
{
	const auto* state = EntityRegistry().TryGet<const LivingReaction>(villager);
	if (state == nullptr || state->reaction == 0 || !Locator::reactionSystem::has_value())
	{
		return std::nullopt;
	}
	return Locator::reactionSystem::value().Find(state->reaction);
}

/// The villager stops reacting and goes back to what it was doing
uint32_t GiveUp(LivingAction& action)
{
	const auto villager = EntityRegistry().ToEntity(action);
	if (auto* state = EntityRegistry().TryGet<LivingReaction>(villager); state != nullptr && state->reaction != 0)
	{
		villager_reactions::Stop(villager, *state, true);
		state->reaction = 0;
		state->type = Reaction::None;
	}
	else
	{
		SetTop(action, VillagerStates::DecideWhatToDo);
	}
	return 0;
}

/// The villager turns towards a point by at most a step a turn
void TurnTowards(entt::entity villager, const glm::vec3& point)
{
	auto& registry = EntityRegistry();
	auto& transform = registry.Get<Transform>(villager);
	auto& wallHug = registry.Get<WallHug>(villager);
	const auto diff = glm::xz(point) - glm::xz(transform.position);
	if (diff == glm::vec2(0.0f))
	{
		return;
	}
	const float wanted = std::atan2(diff.y, diff.x);
	float turn = std::remainder(wanted - wallHug.yAngle, 2.0f * std::numbers::pi_v<float>);
	if (std::abs(turn) > k_WatchTurn)
	{
		turn = std::copysign(k_WatchTurn, turn);
	}
	wallHug.yAngle += turn;
	transform.rotation = glm::eulerAngleY(-wallHug.yAngle - std::numbers::pi_v<float> * 0.5f);
}
/// The awe of a villager's people at a miracle, heard now and then near the hand
void BeliefVoiceOf(entt::entity villager, PlayerNames player, GuidanceAlignment alignment,
                   villager_reactions::BeliefVoice& voice, uint32_t turn)
{
	auto& registry = EntityRegistry();
	const auto& component = registry.Get<const Villager>(villager);
	if (!Locator::gameRandom::has_value() || !Locator::audio::has_value() || component.town == entt::null ||
	    !registry.Valid(component.town))
	{
		return;
	}
	auto& random = Locator::gameRandom::value();
	if (!voice.started)
	{
		voice.started = true;
		voice.lastVoice = turn - random.LocalRand(static_cast<int32_t>(k_VoiceLeastGap));
		voice.lastGuidance = turn - random.LocalRand(static_cast<int32_t>(k_GuidanceLeastGap));
	}
	// Only where the player doesn't lead the town's belief already
	const auto* impression = registry.TryGet<const TownImpression>(component.town);
	if (impression == nullptr)
	{
		return;
	}
	float most = 0.0f;
	for (const float belief : impression->belief.belief)
	{
		most = std::max(most, belief);
	}
	const float believed = impression->belief.belief.at(static_cast<size_t>(player));
	if (!(believed < most))
	{
		return;
	}
	// Now and then: a while after the last, more often a longer while
	const float r = random.LocalFloatRand(1.0f);
	const auto spread = static_cast<int32_t>(k_VoiceGapSpread * (1.0f - r * r * r));
	const uint32_t gap = k_VoiceLeastGap + random.LocalRand(spread);
	if (!(turn - voice.lastVoice > gap))
	{
		return;
	}
	float share = (believed + k_VoiceShareSmall) / (most + k_VoiceShareSmall);
	share -= random.LocalFloatRand(share / 3.0f);
	if (alignment != GuidanceAlignment::Good && alignment != GuidanceAlignment::Evil)
	{
		alignment = random.LocalRand(2) != 0 ? GuidanceAlignment::Evil : GuidanceAlignment::Good;
	}
	const auto& samples = alignment == GuidanceAlignment::Good ? k_GoodAwe : k_EvilAwe;
	std::optional<uint32_t> sample;
	if (share >= 0.7f)
	{
		sample = samples[0];
	}
	else if (share >= 0.4f)
	{
		sample = samples[1];
	}
	else if (share > 0.05f)
	{
		sample = samples[2];
	}
	if (!sample.has_value())
	{
		return;
	}
	// Heard near enough the hand, and not straight after the guidance has spoken
	const auto& at = registry.Get<const Transform>(villager).position;
	float distance = k_VoiceReach;
	if (Locator::handSystem::has_value())
	{
		const auto hand = Locator::handSystem::value().GetPlayerHands()[static_cast<size_t>(HandSystemInterface::Side::Left)];
		if (const auto* handAt = registry.Valid(hand) ? registry.TryGet<const Transform>(hand) : nullptr)
		{
			distance = glm::distance(glm::xz(handAt->position), glm::xz(at));
		}
	}
	const float away = std::min(distance / k_VoiceReach, 1.0f);
	const float since = std::min(static_cast<float>(turn - voice.lastGuidance) / k_GuidanceQuietTurns, 1.0f);
	if (!((1.0f - away * away) * since > k_VoiceWanted))
	{
		return;
	}
	voice.lastVoice = turn;
	Locator::audio::value().StartSoundEffect(resources::HashIdentifier(fmt::format("Guidance.sad/{}", *sample)),
	                                         {.position = at,
	                                          .pitchPercent = k_VoicePitch,
	                                          .minDistance = k_VoiceReach * k_VoiceNearShare,
	                                          .maxDistance = k_VoiceReach});
}
} // namespace

bool villager_reactions::Available(entt::entity villager, Reaction type)
{
	auto& registry = EntityRegistry();
	const auto* component = registry.TryGet<const Villager>(villager);
	const auto* action = registry.TryGet<const LivingAction>(villager);
	if (component == nullptr || action == nullptr || !Locator::livingActionSystem::has_value())
	{
		return false;
	}
	// Judged by the state it is to end up in, which must let it react, and not be dying, dead or past reacting
	const auto state = FinalStateOf(*action);
	if (StateInfo(state).availableForReaction == 0)
	{
		return false;
	}
	switch (state)
	{
	case VillagerStates::InDance:
	case VillagerStates::SetDying:
	case VillagerStates::Dying:
	case VillagerStates::Dead:
	case VillagerStates::Drowning:
	case VillagerStates::Downed:
	case VillagerStates::BeingEaten:
	case VillagerStates::WaitForAnimation:
		return false;
	default:
		break;
	}
	// Nor dying where it is now
	const auto now = StateOf(*action, LivingAction::Index::Top);
	if (now == VillagerStates::SetDying || now == VillagerStates::Dying)
	{
		return false;
	}
	// Too weak to react to anything but food
	// TODO(raffclar): the game also keeps a villager scripts control, one on a structure and two flags whose meaning is
	// unknown from reacting; none of them is modelled
	return type == Reaction::ReactToFood ||
	       static_cast<float>(component->health) / k_VillagerHealthScale > InfoOf(*component).lifeWhenCrawlsWounded;
}

void villager_reactions::Start(entt::entity villager, Reaction type, LivingReaction& state, bool wasReacting)
{
	auto& registry = EntityRegistry();
	auto* action = registry.TryGet<LivingAction>(villager);
	if (action == nullptr)
	{
		return;
	}
	if (!wasReacting)
	{
		state.previousState = StateOf(*action, LivingAction::Index::Top);
	}
	switch (type)
	{
	case Reaction::FleeFromSpell:
		SetTop(*action, VillagerStates::FleeingFromObjectReaction);
		break;
	case Reaction::LookAtNiceSpell:
	case Reaction::ReactToImpressiveSpell:
		SetTop(*action, VillagerStates::LookingAtObjectReaction);
		break;
	case Reaction::ReactToTeleport:
		// It turns aside into the stone
		if (Locator::teleportSystem::has_value())
		{
			if (const auto reaction = Locator::reactionSystem::value().Find(state.reaction))
			{
				Locator::teleportSystem::value().SetupReact(reaction->source.initiator, villager);
			}
		}
		break;
	case Reaction::ReactToFlyingObject:
		// It points at what flies by, or runs from it coming too near
		if (Locator::reactionSystem::has_value())
		{
			if (const auto reaction = Locator::reactionSystem::value().Find(state.reaction))
			{
				villager_physics::SetupReactToFlyingObject(villager, reaction->source.initiator);
			}
		}
		break;
	case Reaction::ReactToFire:
		// The fire's own states take over, from the object burning
		if (Locator::reactionSystem::has_value())
		{
			if (const auto reaction = Locator::reactionSystem::value().Find(state.reaction))
			{
				villager_fire::SetupReactToFire(villager, reaction->source.initiator);
			}
		}
		break;
	default:
		break;
	}
}

void villager_reactions::Stop(entt::entity villager, LivingReaction& state, bool resetState)
{
	auto& registry = EntityRegistry();
	auto* action = registry.TryGet<LivingAction>(villager);
	auto* wallHug = registry.TryGet<WallHug>(villager);
	if (!resetState || action == nullptr)
	{
		if (const auto* component = registry.TryGet<const Villager>(villager); component != nullptr && wallHug != nullptr)
		{
			wallHug->speed = GetSpeedStateSpeed(InfoOf(*component).speedGroup.speedDefault);
		}
		return;
	}
	// Back to the state it was in, or the one that state goes back to; anything else has it decide afresh
	auto resume = state.previousState != VillagerStates::InvalidState
	                  ? static_cast<VillagerStates>(StateInfo(state.previousState).resumeState)
	                  : VillagerStates::InvalidState;
	if (resume == VillagerStates::InvalidState || StateInfo(resume).isReactionState != 0)
	{
		resume = VillagerStates::DecideWhatToDo;
	}
	state.previousState = VillagerStates::InvalidState;
	Locator::livingActionSystem::value().VillagerSetState(*action, LivingAction::Index::Final, VillagerStates::InvalidState,
	                                                      true);
	SetTop(*action, resume);
	SetStateSpeed(villager, FinalStateOf(*action));
}

uint32_t villager_reactions::Fleeing(LivingAction& action)
{
	auto& registry = EntityRegistry();
	const auto villager = registry.ToEntity(action);
	const auto reaction = ReactionOf(villager);
	const auto* info = reaction.has_value() && Locator::infoConstants::has_value()
	                       ? &Locator::infoConstants::value().reaction.at(static_cast<size_t>(reaction->source.type))
	                       : nullptr;
	if (info == nullptr)
	{
		return GiveUp(action);
	}
	const auto here = registry.Get<const Transform>(villager).position;
	const auto object = reaction->source.position;
	const float distance = glm::distance(here, object);
	if (distance > info->maxDistanceToRunAwayFromObject)
	{
		return GiveUp(action);
	}
	const bool moving = glm::length(reaction->velocity) > 0.0f;
	// Far enough from what isn't coming at it, it turns and watches
	if (distance > info->minDistanceToRunAwayFromObject && !magic::ComingTowards(here, object, reaction->velocity))
	{
		SetTop(action, VillagerStates::FleeingAndLookingAtObjectReaction);
		TurnTowards(villager, object);
		return 0;
	}
	glm::vec3 to = magic::FleePointFromStill(here, object);
	if (moving && Locator::gameRandom::has_value())
	{
		auto& random = Locator::gameRandom::value();
		const float x = random.GameFloatRand(magic::k_FleeJitter);
		const float z = random.GameFloatRand(magic::k_FleeJitter);
		to = magic::FleePointFromMoving(here, object, reaction->velocity, x, z);
	}
	// It runs there at its speed for fleeing and watching, then watches
	villager_home::SetupMoveTo(action, glm::xz(to), VillagerStates::FleeingAndLookingAtObjectReaction);
	SetStateSpeed(villager, FinalStateOf(action));
	return 0;
}

void villager_reactions::LookAt(entt::entity villager, const glm::vec3& point)
{
	TurnTowards(villager, point);
}

uint32_t villager_reactions::Watching(LivingAction& action)
{
	auto& registry = EntityRegistry();
	const auto villager = registry.ToEntity(action);
	const auto reaction = ReactionOf(villager);
	if (!reaction.has_value() || !Locator::infoConstants::has_value())
	{
		return GiveUp(action);
	}
	const auto& info = Locator::infoConstants::value().reaction.at(static_cast<size_t>(reaction->source.type));
	const auto here = registry.Get<const Transform>(villager).position;
	if (glm::distance(here, reaction->source.position) > info.maxDistanceToRunAwayFromObject)
	{
		return GiveUp(action);
	}
	TurnTowards(villager, reaction->source.position);
	return 0;
}

float villager_reactions::TownShare(entt::entity town)
{
	auto& registry = EntityRegistry();
	uint32_t people = 0;
	registry.Each<const Villager>([&](entt::entity, const Villager& villager) {
		if (villager.town == town)
		{
			++people;
		}
	});
	const float unmodified = Locator::infoConstants::value().town.populationForUnmodifiedBelief;
	return (unmodified + k_BeliefShareSmall) / (static_cast<float>(people) + k_BeliefShareSmall);
}

ecs::components::TownImpression& villager_reactions::ImpressionOf(entt::entity town)
{
	auto& registry = EntityRegistry();
	if (auto* impression = registry.TryGet<TownImpression>(town))
	{
		return *impression;
	}
	auto& impression = registry.Assign<TownImpression>(town);
	impression.belief.belief.at(static_cast<size_t>(PlayerNames::NEUTRAL)) =
	    Locator::infoConstants::value().town.beliefInNeutralPlayer;
	return impression;
}

void villager_reactions::SetStateSpeed(entt::entity villager, VillagerStates state)
{
	auto& registry = EntityRegistry();
	const auto* component = registry.TryGet<const Villager>(villager);
	auto* wallHug = registry.TryGet<WallHug>(villager);
	if (component == nullptr || wallHug == nullptr || !Locator::infoConstants::has_value())
	{
		return;
	}
	const auto& info = InfoOf(*component);
	const auto& townInfo = Locator::infoConstants::value().town;
	const auto& globals = registry.Context().mapScriptGlobals;
	villager_speed::Inputs inputs {
	    .speeds = {static_cast<int32_t>(info.speedGroup.speedDefault), static_cast<int32_t>(info.speedGroup.speedFleeing),
	               static_cast<int32_t>(info.speedGroup.speed2), static_cast<int32_t>(info.speedGroup.speed3),
	               static_cast<int32_t>(info.speedGroup.speed4), static_cast<int32_t>(info.speedGroup.speed5)},
	    .speedIndex = StateInfo(state).speedIndex,
	    .life = magic_living::LifeOf(villager).value_or(1.0f),
	    .lifeWhenWalksWounded = info.lifeWhenWalksWounded,
	    .lifeWhenCrawlsWounded = info.lifeWhenCrawlsWounded,
	    .landSpeedBalance = globals.landBalance.at(k_SpeedBalance),
	    .landBeliefSpeedScale = globals.landBalance.at(k_BeliefSpeedBalance),
	    .beliefSpeedScaleMultiPlayer = townInfo.beliefSpeedScaleMultiPlayer,
	    .beliefSpeedScaleStory = townInfo.beliefSpeedScaleStory,
	    .landNumber = static_cast<uint32_t>(globals.landNumber),
	    .baseForTownNeedsSpeedMod = info.baseForTownNeedsSpeedMod,
	    .divisorForTownNeedsSpeedMod = info.divisorForTownNeedsSpeedMod,
	    .maxWood = static_cast<float>(info.maxWoodCarried),
	    .maxFood = static_cast<float>(info.maxFoodCarried),
	    .speedModWhenFullLoadOfWood = info.speedModWhenFullLoadOfWood,
	    .speedModWhenFullLoadOfFood = info.speedModWhenFullLoadOfFood,
	    .foodPowerupIncrease = info.foodPowerupIncrease,
	};
	const auto* town = registry.Valid(component->town) ? registry.TryGet<const Town>(component->town) : nullptr;
	if (town != nullptr)
	{
		if (const auto* wants = registry.TryGet<const TownDesire>(component->town))
		{
			inputs.townNeeds = town_desire::TownNeedsSum(*wants);
		}
		// A player's villager goes by the player's Indian tribal power, 1 with no wonder raising it
		if (town->owner != PlayerNames::NEUTRAL)
		{
			inputs.indianPower = 1.0f;
			const auto& belief = ImpressionOf(component->town).belief.belief;
			inputs.belief = villager_speed::Inputs::Belief {.player = belief.at(static_cast<size_t>(town->owner)),
			                                                .neutral = belief.at(static_cast<size_t>(PlayerNames::NEUTRAL))};
		}
	}
	const auto speed = villager_speed::StateSpeed(inputs, [](float max) {
		return Locator::gameRandom::has_value() ? Locator::gameRandom::value().GameFloatRand(max) : 0.0f;
	});
	wallHug->speed = GetSpeedStateSpeed(static_cast<SpeedState>(villager_speed::FinalSpeed(speed, 1.0f)));
}

void villager_reactions::ShowTownBelief(const glm::vec3& centre, PlayerNames player, float belief)
{
	const auto amount = static_cast<int32_t>(belief * k_BeliefSpriteScale);
	if (!Locator::particleSystem::has_value())
	{
		return;
	}
	const auto colour = influence::k_PlayerColours.at(static_cast<size_t>(player) & (influence::k_PlayerColours.size() - 1));
	Locator::particleSystem::value().AddBeliefSprite({.position = centre, .amount = amount, .colour = colour});
}

void villager_reactions::ShowBelief(entt::entity villager, PlayerNames player, float belief, GuidanceAlignment alignment,
                                    BeliefVoice& voice, uint32_t turn)
{
	auto& registry = EntityRegistry();
	const auto& transform = registry.Get<const Transform>(villager);
	// A symbol worth at least one ten thousandth rises from the top of the villager, in the player's colour
	const auto amount = static_cast<int32_t>(belief * k_BeliefSpriteScale);
	if (amount >= 1 && Locator::particleSystem::has_value())
	{
		float height = 0.0f;
		if (const auto* mesh = registry.TryGet<const Mesh>(villager);
		    mesh != nullptr && Locator::resources::value().GetMeshes().Contains(mesh->id))
		{
			height = Locator::resources::value().GetMeshes().Handle(mesh->id)->GetBoundingBox().Size().y * transform.scale.y;
		}
		const auto colour =
		    influence::k_PlayerColours.at(static_cast<size_t>(player) & (influence::k_PlayerColours.size() - 1));
		Locator::particleSystem::value().AddBeliefSprite(
		    {.position = transform.position + glm::vec3(0.0f, height, 0.0f), .amount = amount, .colour = colour});
	}
	BeliefVoiceOf(villager, player, alignment, voice, turn);
}
