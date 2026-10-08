/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "LeashSystem.h"

#include <cmath>

#include <algorithm>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/gtx/transform.hpp>
#include <spdlog/spdlog.h>

#include "3D/CreatureBody.h"
#include "3D/L3DMesh.h"
#include "3D/LandIslandInterface.h"
#include "Audio/AudioManagerInterface.h"
#include "Audio/Sound.h"
#include "Creature/CreatureMorph.h"
#include "Creature/CreatureRig.h"
#include "Creature/LeashKeys.h"
#include "Creature/LeashOwnership.h"
#include "Creature/LeashRules.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/CreatureBody.h"
#include "ECS/Components/CreatureFight.h"
#include "ECS/Components/CreatureLeash.h"
#include "ECS/Components/CreatureLocomotion.h"
#include "ECS/Components/CreatureMind.h"
#include "ECS/Components/Mesh.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/Temple.h"
#include "ECS/Components/Town.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/CreatureLocomotionSystemInterface.h"
#include "ECS/Systems/HandSystemInterface.h"
#include "ECS/Systems/TimeSystemInterface.h"
#include "Input/GameActionMapInterface.h"
#include "Locator.h"
#include "Resources/ResourcesInterface.h"

using namespace openblack;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
using openblack::ecs::Registry;
namespace leash = openblack::creature_leash;

namespace
{
constexpr float k_TurnsPerSecond = 1.0f / std::chrono::duration<float>(TimeSystemInterface::k_TurnDuration).count();
/// Where the leash meets a creature with no collar bone, as a share of its height
constexpr float k_CollarHeightShare = 0.7f;
/// Where the leash meets something it is tied to, as a share of its height, and that height when it has no mesh
constexpr float k_TiedHeightShare = 0.5f;
constexpr float k_DefaultObjectHeight = 5.0f;
/// The leash can be tied to things only once the creature has grown up past this stage
constexpr uint32_t k_TyingPhase = 3;
/// How near the hand the creature walks
constexpr float k_HandArrival = leash::k_CloseToHand * 0.5f;
/// How far round a leash post is tapped
constexpr float k_PostRadius = 4.0f;
/// How far round a creature is tapped, as a share of its height
constexpr float k_CreatureTapShare = 0.4f;
/// How far a tap reaches
constexpr float k_TapReach = 1e6f;

std::optional<glm::vec3> HandPoint()
{
	if (!Locator::handSystem::has_value())
	{
		return std::nullopt;
	}
	return Locator::handSystem::value().GetPlayerHandPositions()[static_cast<size_t>(HandSystemInterface::Side::Left)];
}

float CreatureHeight(const Creature& creature)
{
	return creature_morph::k_HeightAtSizeOne * creature.size;
}

float GroundAt(glm::vec2 point)
{
	return Locator::terrainSystem::has_value() ? Locator::terrainSystem::value().GetHeightAt(point) : 0.0f;
}

/// Where the leash meets the creature: its collar bone as posed this frame, or high on its body when it has none
glm::vec3 CollarPoint(const Registry& registry, entt::entity entity)
{
	const auto& transform = registry.Get<const Transform>(entity);
	const auto& creature = registry.Get<const Creature>(entity);
	const auto fallback = transform.position + glm::vec3(0.0f, CreatureHeight(creature) * k_CollarHeightShare, 0.0f);
	const auto* animation = registry.TryGet<const CreatureAnimation>(entity);
	const auto& rigs = Locator::resources::value().GetCreatureRigs();
	const auto rigId = creature::GetRigId(creature.species);
	if (animation == nullptr || animation->boneMatrices.empty() || !rigs.Contains(rigId))
	{
		return fallback;
	}
	const auto bone = rigs.Handle(rigId)->leashBone;
	if (!bone.has_value() || *bone >= animation->boneMatrices.size())
	{
		return fallback;
	}
	const auto placement = creature::PlacementMatrix(transform.position, transform.rotation, transform.scale);
	return glm::vec3(creature::PosedBone(*bone, animation->boneMatrices, placement)[3]);
}

float ObjectHeight(const Registry& registry, entt::entity entity)
{
	const auto& transform = registry.Get<const Transform>(entity);
	if (const auto* creature = registry.TryGet<const Creature>(entity))
	{
		return CreatureHeight(*creature);
	}
	const auto* mesh = registry.TryGet<const Mesh>(entity);
	const auto& meshes = Locator::resources::value().GetMeshes();
	if (mesh == nullptr || !meshes.Contains(mesh->id))
	{
		return k_DefaultObjectHeight;
	}
	const auto box = meshes.Handle(mesh->id)->GetBoundingBox();
	return std::max((box.maxima.y - box.minima.y) * transform.scale.y, 0.0f);
}

/// Where the leash meets something it is tied to: a creature's collar, else halfway up it
glm::vec3 TiedPoint(const Registry& registry, entt::entity entity)
{
	if (registry.TryGet<const Creature>(entity) != nullptr)
	{
		return CollarPoint(registry, entity);
	}
	return registry.Get<const Transform>(entity).position +
	       glm::vec3(0.0f, ObjectHeight(registry, entity) * k_TiedHeightShare, 0.0f);
}

bool IsMobile(const Registry& registry, entt::entity entity)
{
	return registry.TryGet<const Creature>(entity) != nullptr || registry.TryGet<const Villager>(entity) != nullptr ||
	       registry.TryGet<const Mobile>(entity) != nullptr || registry.TryGet<const MobileObject>(entity) != nullptr;
}

/// The leash's lengths: by the creature's size in the hand, by what it is tied to otherwise
leash::Lengths LengthsOf(const Registry& registry, entt::entity creature, const CreatureLeash::Worn& worn)
{
	const auto& body = registry.Get<const Creature>(creature);
	if (!worn.tiedTo.has_value())
	{
		return leash::InHand(body.size);
	}
	if (IsMobile(registry, *worn.tiedTo))
	{
		return leash::TiedToMobile(CreatureHeight(body));
	}
	const auto& from = registry.Get<const Transform>(creature).position;
	const auto& to = registry.Get<const Transform>(*worn.tiedTo).position;
	return leash::TiedToStatic(glm::distance(glm::vec2(from.x, from.z), glm::vec2(to.x, to.z)));
}

/// How far along a ray it meets a ball, if it does
std::optional<float> RayBall(const glm::vec3& origin, const glm::vec3& direction, const glm::vec3& centre, float radius)
{
	const auto toCentre = centre - origin;
	const auto along = glm::dot(toCentre, direction);
	if (along < 0.0f)
	{
		return std::nullopt;
	}
	const auto apart = glm::dot(toCentre, toCentre) - (along * along);
	if (apart > radius * radius)
	{
		return std::nullopt;
	}
	return along;
}

void PlaySound(audio::SoundId sound, std::optional<glm::vec3> position)
{
	if (Locator::audio::has_value())
	{
		Locator::audio::value().PlaySoundEffect(static_cast<entt::id_type>(sound), position);
	}
}

CreatureMindState* MindOf(Registry& registry, entt::entity creature)
{
	return registry.TryGet<CreatureMindState>(creature);
}

/// What the leash shortcuts need to know about a creature
leash::KeyState KeyStateOf(const Registry& registry, entt::entity creature)
{
	const auto* leashes = registry.TryGet<const CreatureLeash>(creature);
	if (leashes == nullptr)
	{
		return {};
	}
	return {
	    .worn = leashes->worn.has_value(),
	    .tied = leashes->worn.has_value() && leashes->worn->tiedTo.has_value(),
	    .known = leashes->known,
	    .selected = leashes->worn.has_value() ? leashes->worn->type : leashes->selected,
	};
}

int PlayerNumber(PlayerNames player)
{
	return static_cast<int>(player) + 1;
}
} // namespace

bool LeashSystem::Knows(entt::entity creature, LeashType type) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* leashes = registry.TryGet<const CreatureLeash>(creature);
	const auto index = leash::IndexOf(type);
	return leashes != nullptr && index.has_value() && leashes->known.test(*index);
}

void LeashSystem::SetKnown(entt::entity creature, LeashType type, bool known)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto index = leash::IndexOf(type);
	if (!index.has_value() || registry.TryGet<Creature>(creature) == nullptr)
	{
		return;
	}
	auto& leashes = registry.TryGet<CreatureLeash>(creature) != nullptr ? registry.Get<CreatureLeash>(creature)
	                                                                    : registry.Assign<CreatureLeash>(creature);
	leashes.known.set(*index, known);
	// Forgetting the learning leash takes any leash off, as no leash can be worn without it
	if (!known && leashes.worn.has_value() && (leashes.worn->type == type || type == LeashType::Rope))
	{
		TakeOff(creature);
	}
}

std::vector<leash::Claim> LeashSystem::Claims() const
{
	const auto& registry = Locator::entitiesRegistry::value();
	std::vector<leash::Claim> claims;
	registry.Each<const Creature>([&claims](entt::entity entity, const Creature& creature) {
		claims.push_back({.creature = entt::to_integral(entity), .owner = creature.owner, .leashable = creature.leashable});
	});
	return claims;
}

bool LeashSystem::IsLeashable(entt::entity creature) const
{
	const auto* body = Locator::entitiesRegistry::value().TryGet<const Creature>(creature);
	return body != nullptr && body->leashable;
}

bool LeashSystem::SetLeashable(entt::entity creature, bool leashable)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* body = registry.TryGet<Creature>(creature);
	if (body == nullptr)
	{
		return false;
	}
	const auto owner = body->owner;
	if (!leashable)
	{
		if (body->leashable)
		{
			TakeOff(creature);
			registry.Get<Creature>(creature).leashable = false;
			SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} is no longer the one player {} leads",
			                   entt::to_integral(creature), PlayerNumber(owner));
		}
		return true;
	}
	if (!leash::CanLead(owner))
	{
		Refuse(owner, creature, leash::Refusal::NoPlayer);
		return false;
	}
	// A player leads one creature: the one chosen last
	for (const auto id : leash::Displaced(Claims(), entt::to_integral(creature), owner))
	{
		const auto other = static_cast<entt::entity>(id);
		TakeOff(other);
		registry.Get<Creature>(other).leashable = false;
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} is no longer the one player {} leads", id, PlayerNumber(owner));
	}
	if (!registry.Get<Creature>(creature).leashable)
	{
		registry.Get<Creature>(creature).leashable = true;
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} is now the one player {} leads", entt::to_integral(creature),
		                   PlayerNumber(owner));
	}
	return true;
}

void LeashSystem::SetOwner(entt::entity creature, PlayerNames owner)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* body = registry.TryGet<Creature>(creature);
	if (body == nullptr || body->owner == owner)
	{
		return;
	}
	TakeOff(creature);
	auto& changed = registry.Get<Creature>(creature);
	changed.owner = owner;
	// It stays the one its new owner leads only if they have no other
	if (changed.leashable &&
	    (!leash::CanLead(owner) || !leash::Displaced(Claims(), entt::to_integral(creature), owner).empty()))
	{
		registry.Get<Creature>(creature).leashable = false;
	}
}

void LeashSystem::ClaimOnArrival(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	if (body == nullptr || body->leashable)
	{
		return;
	}
	auto others = Claims();
	std::erase_if(others, [creature](const leash::Claim& claim) { return claim.creature == entt::to_integral(creature); });
	if (leash::ClaimsOnArrival(others, body->owner))
	{
		registry.Get<Creature>(creature).leashable = true;
	}
}

leash::Refusal LeashSystem::WhyNot(PlayerNames player, entt::entity creature, LeashType type) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.Valid(creature) ? registry.TryGet<const Creature>(creature) : nullptr;
	if (body == nullptr)
	{
		return leash::Refusal::NotACreature;
	}
	const auto* leashes = registry.TryGet<const CreatureLeash>(creature);
	return leash::WhyNot(
	    player,
	    {
	        .owner = body->owner,
	        .leashable = body->leashable,
	        .knowsLearningLeash = Knows(creature, LeashType::Rope),
	        .knowsType = Knows(creature, type),
	        .heldBy = leashes != nullptr && leashes->worn.has_value() ? std::optional(leashes->worn->holder) : std::nullopt,
	    });
}

std::optional<LeashSystemInterface::Refused> LeashSystem::LastRefusal() const
{
	return _lastRefusal;
}

void LeashSystem::Refuse(PlayerNames player, entt::entity creature, leash::Refusal why)
{
	_lastRefusal = Refused {.player = player, .creature = creature, .why = why};
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "Player {} can't leash creature {}: {}", PlayerNumber(player),
	                   entt::to_integral(creature), leash::Describe(why));
}

bool LeashSystem::PutOn(entt::entity creature, LeashType type)
{
	const auto* body = Locator::entitiesRegistry::value().TryGet<const Creature>(creature);
	if (body == nullptr)
	{
		Refuse(PlayerNames::NEUTRAL, creature, leash::Refusal::NotACreature);
		return false;
	}
	// Whoever puts it on, it is held by the creature's owner
	return PutOnFor(body->owner, creature, type);
}

bool LeashSystem::PutOnFor(PlayerNames player, entt::entity creature, LeashType type)
{
	if (const auto why = WhyNot(player, creature, type); why != leash::Refusal::None)
	{
		Refuse(player, creature, why);
		return false;
	}
	auto& registry = Locator::entitiesRegistry::value();
	auto* leashes = &registry.Get<CreatureLeash>(creature);
	if (!leashes->worn.has_value())
	{
		// Assigned rather than emplaced: clang can't yet see that the nested type is default constructible
		leashes->worn = CreatureLeash::Worn {};
		leashes->worn->holder = player;
	}
	leashes->worn->type = type;
	leashes->selected = type;
	leashes->control = CreatureLeash::Control::Idle;
	// The leash going on ends any drag the body still leans against
	if (auto* animation = registry.TryGet<CreatureAnimation>(creature))
	{
		creature_sway::SetLeashDrag(animation->sway, 0.0f);
	}
	// The rope is laid afresh from the hand to the collar next frame
	leashes->worn->ropeStarted = false;
	return true;
}

void LeashSystem::TakeOff(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* leashes = registry.TryGet<CreatureLeash>(creature);
	if (leashes == nullptr || !leashes->worn.has_value())
	{
		return;
	}
	leashes->worn.reset();
	leashes->control = CreatureLeash::Control::Idle;
	leashes->pull = 0.0f;
	leashes->confinementRadius = 0.0f;
	leashes->turnsWithOther = 0;
	if (auto* mind = MindOf(registry, creature))
	{
		mind->leash.obeying = false;
		mind->leash.forcedDesire.reset();
		mind->leash.forcedValue = 0.0f;
		mind->leash.learningInHand = false;
		mind->leash.miracleSightingWeight = leash::MiracleSightingWeight(false);
	}
}

bool LeashSystem::Toggle(entt::entity creature)
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	if (body == nullptr)
	{
		Refuse(PlayerNames::NEUTRAL, creature, leash::Refusal::NotACreature);
		return false;
	}
	// As the leash key does, for the creature's owner
	return Carry(body->owner, creature, leash::CommandFor(leash::LeashKey::Leash, KeyStateOf(registry, creature)));
}

bool LeashSystem::Carry(PlayerNames player, entt::entity creature, const leash::KeyCommand& command)
{
	using Kind = leash::KeyCommand::Kind;
	const auto checked = command.kind == Kind::PutOn || command.kind == Kind::ChangeType ? command.type : LeashType::Rope;
	if (const auto why = WhyNot(player, creature, checked); why != leash::Refusal::None)
	{
		Refuse(player, creature, why);
		return false;
	}
	switch (command.kind)
	{
	case Kind::None:
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} knows no other leash to pick", entt::to_integral(creature));
		return false;
	case Kind::PutOn:
		return PutOnFor(player, creature, command.type);
	case Kind::TakeOff:
		TakeOff(creature);
		return true;
	case Kind::UntieToHand:
		UntieToHand(creature);
		return true;
	case Kind::ChangeType:
		return ChangeType(creature, command.type);
	}
	return false;
}

bool LeashSystem::PressKey(PlayerNames player, leash::LeashKey key)
{
	const auto creature = PlayersCreature(player);
	if (!creature.has_value())
	{
		_lastRefusal = Refused {.player = player, .creature = entt::null, .why = leash::Refusal::NotACreature};
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Player {} has no creature they can lead", PlayerNumber(player));
		return false;
	}
	const auto& registry = Locator::entitiesRegistry::value();
	return Carry(player, *creature, leash::CommandFor(key, KeyStateOf(registry, *creature)));
}

bool LeashSystem::Shake(PlayerNames player)
{
	const auto creature = PlayersCreature(player);
	if (!creature.has_value() || !IsLeashed(*creature) || TiedTo(*creature).has_value())
	{
		SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "Player {} shook the hand with no leash held in it", PlayerNumber(player));
		return false;
	}
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "Player {} shook the leash off creature {}", PlayerNumber(player),
	                   entt::to_integral(*creature));
	TakeOff(*creature);
	return true;
}

bool LeashSystem::TapCreature(PlayerNames player, entt::entity creature)
{
	if (IsLeashed(creature))
	{
		// Already on: tapping its own creature again does nothing, and another's is refused
		if (const auto why = WhyNot(player, creature, TypeOf(creature)); why != leash::Refusal::None)
		{
			Refuse(player, creature, why);
		}
		return false;
	}
	const auto* leashes = Locator::entitiesRegistry::value().TryGet<const CreatureLeash>(creature);
	const auto picked = leashes != nullptr ? leashes->selected : LeashType::Rope;
	return PutOnFor(player, creature, Knows(creature, picked) ? picked : LeashType::Rope);
}

bool LeashSystem::ChangeType(entt::entity creature, LeashType type)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* leashes = registry.TryGet<CreatureLeash>(creature);
	if (leashes == nullptr || !Knows(creature, type))
	{
		return false;
	}
	leashes->selected = type;
	if (leashes->worn.has_value())
	{
		leashes->worn->type = type;
		leashes->worn->rope.look = leash::LookFor(type);
	}
	// The posts show the leash picked
	registry.Each<LeashPost>([leashes](LeashPost& post) { post.selected = post.type == leashes->selected; });
	SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {}'s picked leash is now the {} leash", entt::to_integral(creature),
	                   leash::Name(type));
	return true;
}

bool LeashSystem::TieTo(entt::entity creature, entt::entity object)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (object == creature || !registry.Valid(object) || registry.TryGet<const Transform>(object) == nullptr)
	{
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {}'s leash can't be tied to that", entt::to_integral(creature));
		return false;
	}
	if (!IsLeashed(creature))
	{
		const auto* picked = registry.TryGet<const CreatureLeash>(creature);
		if (!PutOn(creature, picked != nullptr ? picked->selected : LeashType::Rope))
		{
			return false;
		}
	}
	auto* leashes = &registry.Get<CreatureLeash>(creature);
	auto& worn = *leashes->worn;
	worn.tiedTo = object;
	worn.tiedTurn = Locator::time::has_value() ? Locator::time::value().GetTurn() : 0;
	const auto lengths = LengthsOf(registry, creature, worn);
	worn.rope.slackLength = lengths.slack;
	worn.rope.maxLength = lengths.max;
	worn.ropeStarted = false;
	leashes->control = CreatureLeash::Control::Idle;
	leashes->turnsWithOther = 0;

	// The two tying sounds in turn
	PlaySound(_secondAttachSound ? audio::SoundId::G_LeashAttach_01_2 : audio::SoundId::G_LeashAttach_01_1, std::nullopt);
	_secondAttachSound = !_secondAttachSound;

	// The creature learns that the player wants something done with what it is tied to
	if (auto* mind = MindOf(registry, creature))
	{
		mind->leash.obeying = false;
		const bool isCreature = registry.TryGet<const Creature>(object) != nullptr;
		mind->leash.shown.push_back({
		    .object = static_cast<uint32_t>(object),
		    .type = worn.type,
		    .lessons = leash::LessonsFor(worn.type, isCreature),
		});
		mind->leash.actOn.push_back(static_cast<uint32_t>(object));
	}
	return true;
}

void LeashSystem::UntieToHand(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* leashes = registry.TryGet<CreatureLeash>(creature);
	if (leashes == nullptr || !leashes->worn.has_value() || !leashes->worn->tiedTo.has_value())
	{
		return;
	}
	leashes->worn->tiedTo.reset();
	leashes->worn->ropeStarted = false;
	leashes->turnsWithOther = 0;
}

void LeashSystem::SetWorks(entt::entity creature, bool works)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (auto* leashes = registry.TryGet<CreatureLeash>(creature); leashes != nullptr && leashes->worn.has_value())
	{
		leashes->worn->works = works;
	}
}

void LeashSystem::SetDrawn(bool drawn)
{
	Locator::entitiesRegistry::value().Each<CreatureLeash>([drawn](CreatureLeash& leashes) { leashes.drawn = drawn; });
}

void LeashSystem::ConfineToHome(entt::entity creature, float radius)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (registry.TryGet<Creature>(creature) == nullptr)
	{
		return;
	}
	auto& leashes = registry.TryGet<CreatureLeash>(creature) != nullptr ? registry.Get<CreatureLeash>(creature)
	                                                                    : registry.Assign<CreatureLeash>(creature);
	if (!leashes.home.has_value())
	{
		// Its home is by its player's temple, or where it stands when there is none
		const auto owner = registry.Get<const Creature>(creature).owner;
		registry.Each<const Temple, const Transform>([&leashes, owner](const Temple& temple, const Transform& transform) {
			if (temple.owner == owner && !leashes.home.has_value())
			{
				leashes.home = transform.position;
			}
		});
		if (!leashes.home.has_value())
		{
			leashes.home = registry.Get<const Transform>(creature).position;
		}
	}
	leashes.confinementCentre = *leashes.home;
	leashes.confinementRadius = radius;
}

void LeashSystem::ClearConfinement(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	if (auto* leashes = registry.TryGet<CreatureLeash>(creature))
	{
		leashes->confinementRadius = 0.0f;
		leashes->returning = false;
	}
}

bool LeashSystem::FreeOfHome(entt::entity creature) const
{
	const auto& registry = Locator::entitiesRegistry::value();
	const auto* body = registry.TryGet<const Creature>(creature);
	const auto* transform = registry.TryGet<const Transform>(creature);
	if (body == nullptr || transform == nullptr)
	{
		return false;
	}
	std::optional<glm::vec3> temple;
	registry.Each<const Temple, const Transform>([&temple, body](const Temple& t, const Transform& at) {
		if (t.owner == body->owner && !temple.has_value())
		{
			temple = at.position;
		}
	});
	const auto* leashes = registry.TryGet<const CreatureLeash>(creature);
	const auto home = leashes != nullptr && leashes->home.has_value() ? leashes->home : temple;
	if (!home.has_value())
	{
		return false;
	}
	return leash::FreeOfHome(glm::distance(transform->position, *home), temple.has_value());
}

bool LeashSystem::IsLeashed(entt::entity creature) const
{
	const auto* leashes = Locator::entitiesRegistry::value().TryGet<const CreatureLeash>(creature);
	return leashes != nullptr && leashes->worn.has_value();
}

std::optional<entt::entity> LeashSystem::TiedTo(entt::entity creature) const
{
	const auto* leashes = Locator::entitiesRegistry::value().TryGet<const CreatureLeash>(creature);
	return leashes != nullptr && leashes->worn.has_value() ? leashes->worn->tiedTo : std::nullopt;
}

std::optional<glm::vec3> LeashSystem::HolderPoint(entt::entity creature) const
{
	const auto* leashes = Locator::entitiesRegistry::value().TryGet<const CreatureLeash>(creature);
	if (leashes == nullptr || !leashes->worn.has_value() || leashes->worn->tiedTo.has_value())
	{
		return std::nullopt;
	}
	return HandPoint();
}

LeashType LeashSystem::TypeOf(entt::entity creature) const
{
	const auto* leashes = Locator::entitiesRegistry::value().TryGet<const CreatureLeash>(creature);
	return leashes != nullptr && leashes->worn.has_value() ? leashes->worn->type : LeashType::None;
}

std::optional<entt::entity> LeashSystem::PlayersCreature(PlayerNames player) const
{
	if (const auto id = leash::LeashableOf(Claims(), player))
	{
		return static_cast<entt::entity>(*id);
	}
	return std::nullopt;
}

void LeashSystem::PlacePosts(PlayerNames owner, const std::array<glm::vec3, 3>& points)
{
	auto& registry = Locator::entitiesRegistry::value();
	// A player has one set of posts
	std::vector<entt::entity> old;
	registry.Each<const LeashPost>([&old, owner](entt::entity entity, const LeashPost& post) {
		if (post.owner == owner)
		{
			old.push_back(entity);
		}
	});
	registry.Destroy(old.begin(), old.end());
	const bool hasMesh = Locator::resources::value().GetMeshes().Contains(LeashPost::k_MeshId);
	for (size_t i = 0; i < points.size(); ++i)
	{
		const auto entity = registry.Create();
		registry.Assign<Transform>(entity, points.at(i), glm::mat3(1.0f), glm::vec3(1.0f));
		registry.Assign<LeashPost>(entity, leash::k_Types.at(i), owner, false);
		if (hasMesh)
		{
			registry.Assign<Mesh>(entity, LeashPost::k_MeshId, static_cast<int8_t>(0), static_cast<int8_t>(0));
		}
	}
	registry.SetDirty();
	_postsPlaced = true;
}

bool LeashSystem::TapPost(entt::entity post)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto* tapped = registry.TryGet<LeashPost>(post);
	if (tapped == nullptr)
	{
		return false;
	}
	const auto creature = PlayersCreature(tapped->owner);
	// Tapping the picked leash again puts it back
	if (tapped->selected)
	{
		tapped->selected = false;
		return true;
	}
	PlaySound(audio::SoundId::G_ClickOnSpell_01, std::nullopt);
	registry.Each<LeashPost>([tapped](LeashPost& other) {
		if (other.owner == tapped->owner)
		{
			other.selected = &other == tapped;
		}
	});
	if (creature.has_value())
	{
		ChangeType(*creature, tapped->type);
	}
	return true;
}

void LeashSystem::PlacePostsAtTemples()
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto& meshes = Locator::resources::value().GetMeshes();
	std::vector<std::pair<PlayerNames, std::array<glm::vec3, 3>>> found;
	bool anyTemple = false;
	registry.Each<const Temple, const Transform, const Mesh>([&](const Temple& temple, const Transform& transform,
	                                                             const Mesh& mesh) {
		anyTemple = true;
		if (!meshes.Contains(mesh.id))
		{
			return;
		}
		// The leashes hang at the first three points of the temple's heart
		const auto& points = meshes.Handle(mesh.id)->GetExtraMetrics();
		if (points.size() < 3)
		{
			SPDLOG_LOGGER_DEBUG(spdlog::get("game"), "The temple has {} points, too few to hang its leashes at", points.size());
			return;
		}
		const auto model = glm::translate(transform.position) * glm::mat4(transform.rotation) * glm::scale(transform.scale);
		std::array<glm::vec3, 3> at {};
		for (size_t i = 0; i < at.size(); ++i)
		{
			at.at(i) = glm::vec3(model * points.at(i)[3]);
		}
		found.emplace_back(temple.owner, at);
	});
	if (!anyTemple)
	{
		return;
	}
	for (const auto& [owner, points] : found)
	{
		PlacePosts(owner, points);
	}
	_postsPlaced = true;
}

void LeashSystem::Pull(entt::entity creature)
{
	auto& registry = Locator::entitiesRegistry::value();
	auto& leashes = registry.Get<CreatureLeash>(creature);
	auto* mind = MindOf(registry, creature);
	const auto hand = HandPoint();
	if (!hand.has_value() || !Locator::creatureLocomotionSystem::has_value())
	{
		return;
	}
	// Asleep or out cold, a pull doesn't wake it
	if (mind != nullptr && (creature_mind::IsAsleep(mind->idle) || creature_mind::IsUnconscious(mind->idle)))
	{
		return;
	}
	auto& locomotion = Locator::creatureLocomotionSystem::value();
	const auto* walk = registry.TryGet<const CreatureLocomotion>(creature);
	const auto position = registry.Get<const Transform>(creature).position;
	const bool walking = leashes.control == CreatureLeash::Control::WalkingToHand && locomotion.IsMoving(creature);
	const auto decision = leash::DecideLead(position, *hand, walk != nullptr ? walk->destination : std::nullopt, walking);
	if (decision != leash::Lead::GoToHand)
	{
		return;
	}
	// The first pull stops whatever it was doing; pulled away from the same desire twice, it is held back a while
	if (leashes.control == CreatureLeash::Control::Idle && mind != nullptr)
	{
		// The desire behind a plan it carries out, else behind what it does with nothing better to do
		const auto desire = mind->planActive && mind->planner.current.has_value()
		                        ? std::optional(mind->planner.current->desire)
		                        : leash::DesireBehind(mind->idle.activity, mind->idle.shown);
		mind->planActive = false;
		mind->planner.current.reset();
		if (desire.has_value() && mind->desires.has_value())
		{
			if (const auto seconds = leash::RecordPull(leashes.pulls, *desire))
			{
				creature_desires::Suppress(*mind->desires, *desire, *seconds, k_TurnsPerSecond);
			}
		}
		creature_mind::Plan(mind->idle, creature_mind::Activity::None, {});
	}
	if (locomotion.LeadTo(creature, glm::vec2(hand->x, hand->z), leashes.pull, k_HandArrival) ==
	    CreatureLocomotionSystemInterface::MoveResult::Started)
	{
		leashes.control = CreatureLeash::Control::WalkingToHand;
		// Once on its way, it is pulled along as fast as it goes
		leashes.pull = 1.0f;
		// and its body leans against the drag
		if (auto* animation = registry.TryGet<CreatureAnimation>(creature))
		{
			creature_sway::SetLeashDrag(animation->sway, 1.0f);
		}
		if (mind != nullptr)
		{
			mind->leash.obeying = true;
		}
	}
}

void LeashSystem::ProcessTurn()
{
	auto& registry = Locator::entitiesRegistry::value();
	if (!_postsPlaced)
	{
		PlacePostsAtTemples();
	}
	std::vector<entt::entity> leashed;
	registry.Each<CreatureLeash>([&leashed](entt::entity entity, CreatureLeash& /*leashes*/) { leashed.push_back(entity); });
	for (const auto entity : leashed)
	{
		auto& leashes = registry.Get<CreatureLeash>(entity);
		auto* mind = MindOf(registry, entity);
		const auto* body = registry.TryGet<const Creature>(entity);
		const auto* transform = registry.TryGet<const Transform>(entity);
		// Fighting or knocked out, the leash doesn't pull it about
		if (body == nullptr || transform == nullptr || registry.AnyOf<CreatureFighting, CreatureKnockedOut>(entity))
		{
			continue;
		}
		const auto position = glm::vec2(transform->position.x, transform->position.z);
		const bool moving =
		    Locator::creatureLocomotionSystem::has_value() && Locator::creatureLocomotionSystem::value().IsMoving(entity);

		// Arrived at the hand, or stopped on the way, its mind takes over again
		if (leashes.control == CreatureLeash::Control::WalkingToHand && !moving)
		{
			leashes.control = CreatureLeash::Control::Idle;
			if (mind != nullptr)
			{
				mind->leash.obeying = false;
			}
		}
		if (leashes.control == CreatureLeash::Control::Idle)
		{
			leashes.pull = leash::FadePull(leashes.pull);
		}

		if (!leashes.worn.has_value())
		{
			if (mind != nullptr)
			{
				mind->leash.obeying = false;
				mind->leash.forcedDesire.reset();
				mind->leash.learningInHand = false;
				mind->leash.miracleSightingWeight = leash::MiracleSightingWeight(false);
			}
			// Kept within its home, it walks back when it strays
			if (leash::IsConfined(leashes.confinementRadius, false, true) &&
			    leash::OutsideArea(position, glm::vec2(leashes.confinementCentre.x, leashes.confinementCentre.z),
			                       leashes.confinementRadius))
			{
				if (!moving && Locator::creatureLocomotionSystem::has_value())
				{
					Locator::creatureLocomotionSystem::value().MoveTo(
					    entity, glm::vec2(leashes.confinementCentre.x, leashes.confinementCentre.z),
					    CreatureLocomotionSystemInterface::Pace::Walk, 0.0f, leashes.confinementRadius * 0.5f);
					leashes.returning = true;
				}
			}
			else
			{
				leashes.returning = false;
			}
			continue;
		}

		// Made someone else's or no longer the one its owner leads, the leash comes off
		if (!body->leashable || leashes.worn->holder != body->owner)
		{
			SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {}'s leash comes off: it isn't its holder's to lead",
			                   entt::to_integral(entity));
			TakeOff(entity);
			continue;
		}
		auto& worn = *leashes.worn;
		if (worn.tiedTo.has_value() &&
		    (!registry.Valid(*worn.tiedTo) || registry.TryGet<const Transform>(*worn.tiedTo) == nullptr))
		{
			worn.tiedTo.reset();
			worn.ropeStarted = false;
		}
		const auto hand = HandPoint();
		// It is kept as near what holds the leash as the leash is long
		if (worn.tiedTo.has_value())
		{
			leashes.confinementCentre = registry.Get<const Transform>(*worn.tiedTo).position;
		}
		else if (hand.has_value())
		{
			leashes.confinementCentre = *hand;
		}
		leashes.confinementRadius = worn.rope.maxLength;

		// The leash's feelings, every turn
		if (mind != nullptr)
		{
			mind->leash.forcedDesire = leash::ForcedDesireFor(worn.type);
			mind->leash.forcedValue = mind->leash.forcedDesire.has_value() ? leash::k_ForcedDesireValue : 0.0f;
			mind->leash.learningInHand = worn.type == LeashType::Rope && !worn.tiedTo.has_value();
			mind->leash.miracleSightingWeight = leash::MiracleSightingWeight(worn.type == LeashType::Rope);
		}

		if (worn.tiedTo.has_value())
		{
			const auto object = *worn.tiedTo;
			// Tied to someone else's village, or its own, on any leash but aggression, it wants to impress it
			if (registry.TryGet<const Town>(object) != nullptr && worn.type != LeashType::Evil && mind != nullptr)
			{
				mind->leash.forcedDesire = creature_desires::Desire::Impress;
				mind->leash.forcedValue = leash::k_ImpressTownValue;
			}
			// Tied to another creature: anger spreads on the aggression leash, and they warm or cool to each other
			if (registry.TryGet<const Creature>(object) != nullptr)
			{
				++leashes.turnsWithOther;
				auto* otherMind = MindOf(registry, object);
				const auto otherAt = registry.Get<const Transform>(object).position;
				if (worn.type == LeashType::Evil && otherMind != nullptr &&
				    glm::distance(transform->position, otherAt) < leash::k_AngerOtherReach * CreatureHeight(*body))
				{
					otherMind->leash.forcedDesire = creature_desires::Desire::Anger;
					otherMind->leash.forcedValue = leash::k_ForcedDesireValue;
				}
				if (const auto change = leash::AttitudeChange(worn.type, leashes.turnsWithOther); change != 0.0f)
				{
					if (mind != nullptr)
					{
						mind->leash.attitudes.push_back({.creature = static_cast<uint32_t>(object), .change = change});
					}
					if (otherMind != nullptr)
					{
						otherMind->leash.attitudes.push_back({.creature = static_cast<uint32_t>(entity), .change = change});
					}
				}
			}
			// Strayed beyond the leash's length, it walks back to what it is tied to
			if (worn.works && !moving &&
			    leash::OutsideArea(position, glm::vec2(leashes.confinementCentre.x, leashes.confinementCentre.z),
			                       worn.rope.slackLength) &&
			    Locator::creatureLocomotionSystem::has_value())
			{
				Locator::creatureLocomotionSystem::value().LeadTo(
				    entity, glm::vec2(leashes.confinementCentre.x, leashes.confinementCentre.z), leashes.pull,
				    worn.rope.slackLength * 0.5f);
			}
			continue;
		}

		// Held in the hand: a taut rope pulls it to the hand
		if (worn.works && leash::ShouldPull(worn.rope.tension))
		{
			Pull(entity);
		}
	}
}

void LeashSystem::Update(float seconds)
{
	auto& registry = Locator::entitiesRegistry::value();
	const auto hand = HandPoint();
	registry.Each<CreatureLeash, const Creature, const Transform>(
	    [&](entt::entity entity, CreatureLeash& leashes, const Creature& /*creature*/, const Transform& /*transform*/) {
		    if (!leashes.worn.has_value())
		    {
			    return;
		    }
		    auto& worn = *leashes.worn;
		    std::optional<glm::vec3> start;
		    if (worn.tiedTo.has_value() && registry.Valid(*worn.tiedTo) &&
		        registry.TryGet<const Transform>(*worn.tiedTo) != nullptr)
		    {
			    start = TiedPoint(registry, *worn.tiedTo);
		    }
		    else
		    {
			    start = hand;
		    }
		    if (!start.has_value())
		    {
			    // The hand is off the land: the rope keeps its last place
			    return;
		    }
		    const auto end = CollarPoint(registry, entity);
		    // Held in the hand, its length follows the creature's size
		    if (!worn.tiedTo.has_value() || !worn.ropeStarted)
		    {
			    const auto lengths = LengthsOf(registry, entity, worn);
			    worn.rope.slackLength = lengths.slack;
			    worn.rope.maxLength = lengths.max;
		    }
		    if (!worn.ropeStarted)
		    {
			    worn.rope =
			        leash_rope::Create(*start, end, worn.rope.slackLength, worn.rope.maxLength, leash::LookFor(worn.type));
			    worn.ropeStarted = true;
			    return;
		    }
		    worn.rope.look = leash::LookFor(worn.type);
		    leash_rope::Step(worn.rope, *start, end, seconds, GroundAt);
	    });
}

void LeashSystem::HandleInput(const glm::vec3& rayOrigin, const glm::vec3& rayDirection, bool actionTaken)
{
	if (!Locator::gameActionSystem::has_value())
	{
		return;
	}
	using input::BindableActionMap;
	const auto& actions = Locator::gameActionSystem::value();
	const auto player = PlayerNames::PLAYER_ONE;
	const auto creature = PlayersCreature(player);
	auto& registry = Locator::entitiesRegistry::value();
	const auto pressed = [&actions](BindableActionMap action) { return actions.GetChanged(action) && actions.Get(action); };

	// The leash shortcuts
	for (const auto action :
	     {BindableActionMap::LEASH_UNLEASH_CREATURE, BindableActionMap::PREVIOUS_LEASH, BindableActionMap::NEXT_LEASH})
	{
		if (const auto key = leash::KeyFor(action); key.has_value() && pressed(action))
		{
			PressKey(player, *key);
		}
	}

	if (actionTaken || !pressed(BindableActionMap::ACTION) || glm::length(rayDirection) <= 0.0f)
	{
		return;
	}
	const auto direction = glm::normalize(rayDirection);
	// What the Action button taps: the nearest post, creature or other thing along the ray
	std::optional<entt::entity> tapped;
	float nearest = k_TapReach;
	const auto consider = [&](entt::entity entity, const glm::vec3& centre, float radius) {
		if (const auto along = RayBall(rayOrigin, direction, centre, radius); along.has_value() && *along < nearest)
		{
			nearest = *along;
			tapped = entity;
		}
	};
	registry.Each<const LeashPost, const Transform>([&](entt::entity entity, const LeashPost& post, const Transform& at) {
		if (post.owner == player)
		{
			consider(entity, at.position, k_PostRadius);
		}
	});
	registry.Each<const Creature, const Transform>([&](entt::entity entity, const Creature& body, const Transform& at) {
		const auto height = CreatureHeight(body);
		consider(entity, at.position + glm::vec3(0.0f, height * 0.5f, 0.0f), height * k_CreatureTapShare);
	});
	// Other things only when the leash is worn, to tie it to
	const bool wearing = creature.has_value() && IsLeashed(*creature);
	if (wearing)
	{
		registry.Each<const Mesh, const Transform>([&](entt::entity entity, const Mesh& /*mesh*/, const Transform& at) {
			if (registry.TryGet<const Creature>(entity) != nullptr || registry.TryGet<const LeashPost>(entity) != nullptr ||
			    registry.TryGet<const Temple>(entity) != nullptr)
			{
				return;
			}
			const auto height = ObjectHeight(registry, entity);
			consider(entity, at.position + glm::vec3(0.0f, height * 0.5f, 0.0f), std::max(height * 0.5f, 1.0f));
		});
	}
	if (!tapped.has_value())
	{
		return;
	}
	if (registry.TryGet<const LeashPost>(*tapped) != nullptr)
	{
		TapPost(*tapped);
		return;
	}
	// A creature is clicked or held by the right button as it is let go (see TapCreature and the creature hand): only
	// with the leash on does pressing it on another creature tie the leash to that one
	const bool isCreature = registry.TryGet<const Creature>(*tapped) != nullptr;
	if (!wearing || (isCreature && *tapped == *creature))
	{
		return;
	}
	if (TiedTo(*creature) == tapped)
	{
		UntieToHand(*creature);
		return;
	}
	// Grown up enough, it can be tied to other things
	const auto* mind = registry.TryGet<const CreatureMindState>(*creature);
	if (mind == nullptr || mind->developmentPhase > k_TyingPhase)
	{
		TieTo(*creature, *tapped);
	}
	else
	{
		SPDLOG_LOGGER_INFO(spdlog::get("game"), "Creature {} is too young to have its leash tied to things",
		                   entt::to_integral(*creature));
	}
}
