/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#define LOCATOR_IMPLEMENTATIONS

#include "HandGrabSystem.h"

#include <algorithm>

#include <glm/geometric.hpp>

#include "ECS/Components/Animal.h"
#include "ECS/Components/AtHome.h"
#include "ECS/Components/CarriedByTornado.h"
#include "ECS/Components/Creature.h"
#include "ECS/Components/DeadTree.h"
#include "ECS/Components/HandGrab.h"
#include "ECS/Components/LivingAction.h"
#include "ECS/Components/Mobile.h"
#include "ECS/Components/OneOffSpellSeed.h"
#include "ECS/Components/Physics.h"
#include "ECS/Components/Pot.h"
#include "ECS/Components/StreetLantern.h"
#include "ECS/Components/TeleportStone.h"
#include "ECS/Components/Transform.h"
#include "ECS/Components/Tree.h"
#include "ECS/Components/Villager.h"
#include "ECS/Registry.h"
#include "ECS/Systems/DynamicsSystemInterface.h"
#include "GameHandGrabWorld.h"

using namespace openblack;
using namespace openblack::ecs;
using namespace openblack::ecs::components;
using namespace openblack::ecs::systems;
using hand_grab::GrabKind;

namespace
{
/// The hand's pick-up sound
constexpr uint32_t k_PickUpSample = 10;
/// The hand cries of the people, as many of each as there are: a child's, a woman's and a man's
constexpr uint32_t k_ChildCries = 180;
constexpr uint32_t k_WomanCries = 194;
constexpr uint32_t k_ManCries = 187;
constexpr uint32_t k_CriesEach = 7;
/// A tree pulled up creaks, one of three
constexpr uint32_t k_TreeCreaks = 32;
constexpr uint32_t k_TreeCreaksEach = 3;

/// The statics with kinds of their own the hand never takes: lanterns, bonfires, shields, vortices and teleport stones
bool StaticOfItsOwn(MobileStaticInfo type)
{
	switch (type)
	{
	case MobileStaticInfo::StreetLantern:
	case MobileStaticInfo::CountryLantern:
	case MobileStaticInfo::Bonfire:
	case MobileStaticInfo::PhysicalShield:
	case MobileStaticInfo::Vortex:
	case MobileStaticInfo::Teleport:
		return true;
	default:
		return false;
	}
}

/// Where a line meets a plane, none when it runs along it
std::optional<glm::vec3> LineMeetsPlane(glm::vec3 origin, glm::vec3 direction, glm::vec3 point, glm::vec3 normal)
{
	const float along = glm::dot(direction, normal);
	if (along == 0.0f)
	{
		return std::nullopt;
	}
	return origin + direction * (glm::dot(point - origin, normal) / along);
}
} // namespace

HandGrabSystem::HandGrabSystem()
    : HandGrabSystem(std::make_unique<GameHandGrabWorld>())
{
}

HandGrabSystem::HandGrabSystem(std::unique_ptr<hand_grab::HandGrabWorldInterface> world)
    : _world(std::move(world))
{
}

HandGrabSystem::~HandGrabSystem() = default;

bool HandGrabSystem::Exists(entt::entity object) const
{
	return object != entt::null && _world->Entities().Valid(object);
}

HandGrab* HandGrabSystem::Grab()
{
	const auto hand = _world->Hand();
	if (!Exists(hand))
	{
		return nullptr;
	}
	auto& registry = _world->Entities();
	if (auto* grab = registry.TryGet<HandGrab>(hand))
	{
		return grab;
	}
	return &registry.Assign<HandGrab>(hand);
}

const HandGrab* HandGrabSystem::Grab() const
{
	const auto hand = _world->Hand();
	return Exists(hand) ? _world->Entities().TryGet<const HandGrab>(hand) : nullptr;
}

GrabKind HandGrabSystem::KindOf(entt::entity object) const
{
	const auto& registry = _world->Entities();
	if (registry.AllOf<Creature>(object))
	{
		return GrabKind::None;
	}
	if (registry.AllOf<Villager>(object))
	{
		return GrabKind::Villager;
	}
	if (registry.AllOf<Animal>(object))
	{
		return GrabKind::Animal;
	}
	if (registry.AllOf<Tree>(object))
	{
		return GrabKind::Tree;
	}
	if (registry.AllOf<DeadTree>(object))
	{
		return GrabKind::DeadTree;
	}
	// A one-shot miracle's bubble is the magic's to take
	if (registry.AnyOf<OneOffSpellSeed, StreetLantern, TeleportStone>(object))
	{
		return GrabKind::None;
	}
	if (const auto* pot = registry.TryGet<const Pot>(object))
	{
		// Loose pots are picked up. Piles are scooped from, not picked up: the only pile the hand holds is a handful, of
		// its pile's own handful kind, while it holds something
		if (_world->IsLoosePot(pot->type))
		{
			return GrabKind::MobileObject;
		}
		const auto facts = _world->PotFactsOf(object);
		return facts.has_value() && facts->handful == pot->type && pot->amount > 0 ? GrabKind::MobileObject : GrabKind::None;
	}
	if (const auto* still = registry.TryGet<const MobileStatic>(object))
	{
		if (StaticOfItsOwn(still->type))
		{
			return GrabKind::None;
		}
		return _world->StaticKindOf(still->type) == MobileStaticInfo::Rock ? GrabKind::Rock : GrabKind::MobileStatic;
	}
	if (const auto* mobile = registry.TryGet<const MobileObject>(object))
	{
		return mobile->type == MobileObjectInfo::LumpOfPoo ? GrabKind::Poo : GrabKind::MobileObject;
	}
	return GrabKind::None;
}

hand_grab::Holdable HandGrabSystem::HoldableOf(entt::entity object) const
{
	const auto& registry = _world->Entities();
	const auto kind = KindOf(object);
	const auto* action = registry.TryGet<const LivingAction>(object);
	const bool hiding = action != nullptr && action->states[static_cast<size_t>(LivingAction::Index::Top)] ==
	                                             static_cast<uint8_t>(VillagerStates::GoAndHideInNearbyBuilding);
	return {
	    .kind = kind,
	    .available = Exists(object),
	    .atHome = registry.AllOf<AtHome>(object),
	    .inHand = registry.AllOf<InHand>(object),
	    .hiding = hiding,
	    .speciesAllows = kind == GrabKind::Animal && _world->SpeciesAllowsPickUp(object),
	    .radius = _world->SizeOf(object).radius,
	    .isRock = kind == GrabKind::Rock,
	};
}

bool HandGrabSystem::HandInInfluence() const
{
	const auto* grab = _world->Entities().TryGet<const HandGrab>(_world->Hand());
	return grab != nullptr && grab->handPoint.has_value() && _world->InInfluence(_world->HandPlayer(), *grab->handPoint);
}

bool HandGrabSystem::MayTake(entt::entity object) const
{
	const auto& registry = _world->Entities();
	return hand_grab::PassesGate({.spaceInHand = true,
	                              .alreadyInHand = registry.AllOf<InHand>(object),
	                              .valid = hand_grab::ValidForPlaceInHand(HoldableOf(object)),
	                              .cannotBePickedUp = registry.AllOf<CannotBePickedUp>(object),
	                              .carried = registry.AllOf<CarriedByTornado>(object),
	                              .inInfluence = HandInInfluence()});
}

hand_grab::HoldFacts HandGrabSystem::HoldOfObject(entt::entity object) const
{
	const auto& registry = _world->Entities();
	const auto size = _world->SizeOf(object);
	const auto* still = registry.TryGet<const MobileStatic>(object);
	const auto type = still != nullptr ? still->type : MobileStaticInfo::None;
	return hand_grab::HoldOf(KindOf(object), type, still != nullptr ? _world->StaticMeshOf(type) : MeshId::Dummy, size.height,
	                         size.radius);
}

void HandGrabSystem::Tap(entt::entity object)
{
	// Only within the player's influence, and not a thing a script holds out of reach
	const auto& registry = _world->Entities();
	if (!HandInInfluence() || registry.AllOf<CannotBePickedUp>(object))
	{
		return;
	}
	_world->TapThing(object, _world->PoseOf(_world->Hand()).origin, _world->HandPlayer());
}

bool HandGrabSystem::Press(uint32_t nowMs, uint32_t turn)
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return false;
	}
	switch (grab->state)
	{
	case HandGrab::State::Holding:
		// On something the held thing can be used on, it is given to it; otherwise ready to throw: the spring takes hold of
		// the hand the next frame, where the hand then is
		if (const auto target = _world->ObjectUnderCursor(); target.has_value() && HandInInfluence() && ApplyTo(*grab, *target))
		{
			return true;
		}
		grab->state = HandGrab::State::ReadyToThrow;
		grab->springPending = true;
		return true;
	case HandGrab::State::Grabbing:
	case HandGrab::State::ReadyToThrow:
		return true;
	case HandGrab::State::Empty:
		break;
	}

	const auto object = _world->ObjectUnderCursor();
	// A pile is scooped from at once
	if (object.has_value() && StartScoop(*grab, *object))
	{
		return true;
	}
	if (!object.has_value() || !MayTake(*object))
	{
		// A press the hand can't take is a tap on the thing (clicking and activating)
		if (object.has_value())
		{
			Tap(*object);
		}
		return false;
	}
	grab->state = HandGrab::State::Grabbing;
	grab->object = *object;
	grab->pressMs = nowMs;
	grab->pressTurn = turn;
	grab->pullSeconds = 0.0f;
	grab->tug.reset();
	// Something in flight isn't pulled: it is caught once the button has been held a while. Anything else is pulled at,
	// until it comes free.
	grab->pulling = !_world->IsFlying(*object);
	// The first frame of the pull never pulls
	grab->waits = true;
	return true;
}

std::optional<entt::entity> HandGrabSystem::Release(uint32_t nowMs, uint32_t turn)
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return std::nullopt;
	}
	switch (grab->state)
	{
	case HandGrab::State::Grabbing:
	{
		// Let go before the thing came free: it stays where it is, leaning as it was pulled, and a short press was a tap
		const auto object = grab->object;
		const bool tap = hand_grab::ElapsedMs(nowMs, grab->pressMs, turn, grab->pressTurn) <= hand_grab::k_GrabWaitMs;
		Empty(*grab);
		// It still becomes the thing last let go, which a flying thing's twist then finds
		grab->released = object;
		if (tap && Exists(object))
		{
			Tap(object);
			return object;
		}
		return std::nullopt;
	}
	case HandGrab::State::ReadyToThrow:
	{
		// Let go with its point on the land off the map or out of the player's influence, the hand keeps hold and its
		// spring lets go of it
		if (!grab->handPoint.has_value() || !_world->InBounds(*grab->handPoint) || !HandInInfluence())
		{
			grab->state = HandGrab::State::Holding;
			grab->springOn = false;
			grab->springPending = false;
			return std::nullopt;
		}
		LetGo(*grab, grab->springOn ? grab->spring.Velocity() : glm::vec3(0.0f), false);
		return std::nullopt;
	}
	case HandGrab::State::Holding:
		// Letting go of the button ends a scoop; the hand holds its handful
		if (grab->scoopSource != entt::null)
		{
			EndScoop(*grab);
		}
		break;
	case HandGrab::State::Empty:
		break;
	}
	return std::nullopt;
}

bool HandGrabSystem::StartScoop(HandGrab& grab, entt::entity source)
{
	if (const auto field = _world->FieldFactsOf(source))
	{
		return StartFieldScoop(grab, source, *field);
	}
	const auto facts = _world->PotFactsOf(source);
	// Only a pile is scooped from, never a handful, and only inside the player's influence
	if (!facts.has_value() || facts->potType == PotType::Pot || !HandInInfluence())
	{
		return false;
	}
	const auto& registry = _world->Entities();
	if (registry.Get<const Pot>(source).type == facts->handful)
	{
		return false;
	}
	const auto scoop = _world->ScoopFactsOf(facts->handful);
	const auto first = std::min(scoop.initial, facts->amount);
	if (first == 0)
	{
		return false;
	}
	const auto sourcePosition = _world->PoseOf(source).origin;
	_world->TakeFromPile(source, first);
	const auto hand = _world->PoseOf(_world->Hand()).origin;
	const auto handful = _world->MakeHandful(facts->handful, hand, first, facts->poisoned);
	if (!Exists(handful))
	{
		return false;
	}
	Take(grab, handful, false);
	if (grab.state != HandGrab::State::Holding)
	{
		return false;
	}
	grab.scoopSource = source;
	grab.scoopTurns = 0;
	grab.scoopAnchor = hand;
	// What is scooped streams from the source into the hand
	grab.scoopStreamSeconds = 0.0f;
	// The cursor is pinned while it scoops
	_world->PinCursor(true);
	grab.scoopStream = _world->StartScoopStream(facts->resource, sourcePosition, facts->poisoned);
	return true;
}

bool HandGrabSystem::StartFieldScoop(HandGrab& grab, entt::entity field, const FieldFacts& facts)
{
	if (!HandInInfluence())
	{
		return false;
	}
	// A field gives its first handful of food at once, half of it once ripe
	const auto scoop = _world->ScoopFactsOf(PotInfo::HandFood);
	auto first = std::min(scoop.initial, facts.food);
	if (facts.ripe)
	{
		first >>= 1u;
	}
	if (first == 0)
	{
		return false;
	}
	_world->TakeFromField(field, first);
	const auto hand = _world->PoseOf(_world->Hand()).origin;
	const auto handful = _world->MakeHandful(PotInfo::HandFood, hand, first, false);
	if (!Exists(handful))
	{
		return false;
	}
	Take(grab, handful, false);
	if (grab.state != HandGrab::State::Holding)
	{
		return false;
	}
	grab.scoopSource = field;
	grab.scoopTurns = 0;
	grab.scoopAnchor = hand;
	grab.scoopStreamSeconds = 0.0f;
	// The cursor is pinned while it scoops
	_world->PinCursor(true);
	grab.scoopStream = _world->StartScoopStream(ResourceType::Food, _world->PoseOf(field).origin, false);
	return true;
}

bool HandGrabSystem::ScoopField(HandGrab& grab, const FieldFacts& facts)
{
	auto& registry = _world->Entities();
	auto* handful = registry.TryGet<Pot>(grab.object);
	if (handful == nullptr)
	{
		return false;
	}
	const auto scoop = _world->ScoopFactsOf(PotInfo::HandFood);
	// As much as the ramp gives, no more than the field has, within what one handful holds, and half once ripe
	auto taken = std::min(hand_grab::ScoopAmount(grab.scoopTurns, scoop), facts.food);
	if (scoop.maxPickedUp != 0)
	{
		const auto room = static_cast<int64_t>(scoop.maxPickedUp) - static_cast<int64_t>(handful->amount);
		taken = static_cast<uint32_t>(std::clamp<int64_t>(std::min<int64_t>(room, taken), 0, taken));
	}
	if (facts.ripe)
	{
		taken /= 2u;
	}
	if (taken == 0)
	{
		if (grab.scoopStream.has_value())
		{
			_world->StopScoopStream(*grab.scoopStream);
			grab.scoopStream.reset();
		}
		return false;
	}
	_world->TakeFromField(grab.scoopSource, taken);
	handful->amount += taken;
	_world->ResizePot(grab.object);
	return true;
}

bool HandGrabSystem::Scoop(HandGrab& grab)
{
	if (const auto field = _world->FieldFactsOf(grab.scoopSource))
	{
		return ScoopField(grab, *field);
	}
	auto& registry = _world->Entities();
	const auto* handful = registry.TryGet<Pot>(grab.object);
	const auto source = _world->PotFactsOf(grab.scoopSource);
	// It scoops only into a handful of the pile's own kind
	if (handful == nullptr || !source.has_value() || handful->type != source->handful)
	{
		return false;
	}
	const auto scoop = _world->ScoopFactsOf(source->handful);
	const auto wanted = hand_grab::ScoopAmount(grab.scoopTurns, scoop);
	_world->PlayScoopSound(source->resource, _world->PoseOf(_world->Hand()).origin,
	                       hand_grab::ScoopRamp(grab.scoopTurns, scoop));
	const auto taken = hand_grab::ScoopTaken(wanted, source->amount, handful->amount, scoop);
	if (taken == 0)
	{
		if (grab.scoopStream.has_value())
		{
			_world->StopScoopStream(*grab.scoopStream);
			grab.scoopStream.reset();
		}
		return false;
	}
	const auto took = _world->TakeFromPile(grab.scoopSource, taken);
	registry.Get<Pot>(grab.object).amount += took;
	_world->ResizePot(grab.object);
	// A pile scooped empty stops streaming; the scoop ends with the next turn
	if (!Exists(grab.scoopSource) && grab.scoopStream.has_value())
	{
		_world->StopScoopStream(*grab.scoopStream);
		grab.scoopStream.reset();
	}
	return true;
}

void HandGrabSystem::EndScoop(HandGrab& grab)
{
	if (grab.scoopStream.has_value())
	{
		_world->StopScoopStream(*grab.scoopStream);
		grab.scoopStream.reset();
	}
	if (grab.scoopSource != entt::null)
	{
		_world->PinCursor(false);
	}
	grab.scoopSource = entt::null;
	grab.scoopTurns = 0;
}

bool HandGrabSystem::ApplyTo(HandGrab& grab, entt::entity target)
{
	const auto held = grab.object;
	if (!Exists(held) || target == held || !Exists(target))
	{
		return false;
	}
	// A pot is given to a store of what it holds, or poured into a pile or pot of the same
	const auto pot = _world->PotFactsOf(held);
	if (!pot.has_value())
	{
		// A tree, a dead tree, a fence, a mushroom or an animal goes whole into a store of what it is worth
		// TODO(stores): onto a worship totem, a teleport, a gate's plinth or a scaffold; openblack has none of those yet
		if (!_world->TakeIntoStore(target, held))
		{
			return false;
		}
		Empty(grab);
		return true;
	}
	if (_world->StoresResource(target, pot->resource))
	{
		_world->AddToStore(target, pot->resource, pot->amount, pot->poisoned);
	}
	else if (const auto other = _world->PotFactsOf(target); other.has_value() && other->resource == pot->resource)
	{
		_world->PourAt(pot->resource, _world->PoseOf(_world->Hand()).origin, pot->amount, _world->HandPlayer(), pot->poisoned);
	}
	else
	{
		return false;
	}
	_world->Entities().Remove<InHand>(held);
	_world->UseUp(held);
	Empty(grab);
	return true;
}

void HandGrabSystem::StartPull(HandGrab& grab, const Frame& frame)
{
	const auto object = grab.object;
	const auto pose = _world->PoseOf(object);
	hand_grab::Tug tug {.axes = pose.axes, .base = pose.origin};
	// The pull works in the plane of the land under the thing, at the height the hand's drop below the camera reaches
	// when scaled from the hand's distance across the ground to the base's. The hand is where the cursor puts it.
	grab.pullPlaneNormal = _world->LandNormalAt(pose.origin);
	const auto across = [](glm::vec3 a, glm::vec3 b) { return glm::length(glm::vec2(a.x - b.x, a.z - b.z)); };
	const float toBase = across(frame.camera, pose.origin);
	const float toHand = across(frame.camera, frame.target);
	grab.pullPlanePoint = pose.origin;
	grab.pullPlanePoint.y =
	    toHand > 0.0f ? frame.camera.y - (toBase / toHand) * (frame.camera.y - frame.target.y) : pose.origin.y;
	const auto hold = HoldOfObject(object);
	// The hand holds it the way its kind is held while it pulls
	grab.hold = hold;
	grab.holdDistance = hand_grab::HoldDistance(hold.loweringMultiplier, _world->SizeOf(object).height, grab.handSize);
	grab.stretch.Reset(1.0f);
	grab.tug = tug;
}

glm::vec3 HandGrabSystem::Pull(HandGrab& grab, const Frame& frame)
{
	const auto object = grab.object;
	auto& tug = *grab.tug;
	const auto size = _world->SizeOf(object);
	const bool leans = !_world->IsOfRockMaterial(object);
	grab.pullSeconds += frame.seconds;
	if (grab.pullSeconds < hand_grab::k_PullBlendSeconds || grab.waits)
	{
		// The pull starts the frame after the hand has faded into its pulling pose
		grab.waits = grab.pullSeconds < hand_grab::k_PullBlendSeconds;
	}
	else if (const auto hand = LineMeetsPlane(frame.rayOrigin, frame.rayDirection, grab.pullPlanePoint, grab.pullPlaneNormal))
	{
		const bool tree = KindOf(object) == GrabKind::Tree;
		const auto result = hand_grab::PullAt(tug, {
		                                               .hand = *hand,
		                                               .holdDistance = grab.holdDistance,
		                                               .maxForce = hand_grab::MaxForce(0.0f),
		                                               .weight = _world->WeightOf(object),
		                                               .height = size.height,
		                                               .tree = tree,
		                                               .leans = leans,
		                                               .seconds = frame.seconds,
		                                           });
		grab.stretch.SetDestination(result.stretchTarget, hand_grab::k_TugStretchSeconds);
		grab.stretch.Update(frame.seconds);
		if (result.comesFree)
		{
			// A standing tree pulled out of the ground leaves its roots in a hole
			if (tree && !_world->IsFlying(object))
			{
				_world->LeaveRootsHole(object);
			}
			grab.pulling = false;
		}
		// It is drawn leaning, and stretched up towards the hand
		auto axes = tug.axes;
		if (leans)
		{
			axes[1] *= grab.stretch.GetValue();
		}
		_world->SetPose(object, {.axes = axes, .origin = tug.base});
	}
	// How far up it the hand grips it, as the hand measures it every frame
	grab.holdDistance = HoldOfObject(object).loweringMultiplier * size.height;
	return hand_grab::TugHandPoint(tug, grab.holdDistance, grab.stretch.GetValue());
}

void HandGrabSystem::Take(HandGrab& grab, entt::entity object, bool fromTheWorld)
{
	auto& registry = _world->Entities();
	// The pull may have ended on a thing that can no longer be taken: the hand lets go of it
	if (!MayTake(object))
	{
		Empty(grab);
		return;
	}
	const bool wasFlying = _world->IsFlying(object);
	const auto kind = KindOf(object);
	const auto player = _world->HandPlayer();

	// Out of the physics without landing, and out of the map's cells; it no longer flies past anyone
	if (fromTheWorld)
	{
		_world->LeavePhysicsAndMap(object);
		_world->RemoveReactions(object, Reaction::ReactToFlyingObject);
	}
	const auto position = _world->PoseOf(object).origin;

	// Its pick-up sound: a standing tree pulled from the ground creaks instead; people cry out
	if (fromTheWorld && (kind != GrabKind::Tree || wasFlying))
	{
		_world->PlaySample(k_PickUpSample, position);
		if (const auto* villager = registry.TryGet<const Villager>(object);
		    villager != nullptr && _world->LifeOf(object) > 0.0f)
		{
			const uint32_t first = villager->lifeStage == Villager::LifeStage::Child ? k_ChildCries
			                       : villager->sex == Villager::Sex::FEMALE          ? k_WomanCries
			                                                                         : k_ManCries;
			_world->PlaySample(first + _world->LocalRandom(k_CriesEach), position);
		}
	}

	// The people and animals about the hand see it pick something up
	const auto handPosition = _world->PoseOf(_world->Hand()).origin;
	_world->CreateReaction(
	    {.initiator = _world->Hand(), .type = Reaction::ReactToHandPickUp, .player = player, .position = handPosition});
	// A burning thing keeps burning in the hand, out of its blaze; the people round it flee one that isn't a villager
	_world->FireStartedMoving(object, !registry.AllOf<Villager>(object));
	// TODO(hand): a firefly sitting on what is picked up is taken off it for a reward (openblack has no fireflies)

	// The thing's own part of going into the hand
	if (registry.AllOf<Villager>(object))
	{
		// TODO(hand): a villager in the middle of making love lets its partner go (openblack keeps no partners)
		// One of the hand's player's own people of an age to make love alarms those about it
		if (_world->IsSexuallyActive(object) && _world->PlayerOf(object) == player)
		{
			_world->CreateReaction(
			    {.initiator = object, .type = Reaction::ReactToVillagerInHand, .player = player, .position = position});
		}
		_world->VillagerIntoHand(object);
	}
	if (registry.AllOf<Animal>(object))
	{
		_world->AnimalIntoOwnFlock(object);
	}
	if (kind == GrabKind::Tree && !wasFlying)
	{
		_world->PlaySample(k_TreeCreaks + _world->LocalRandom(k_TreeCreaksEach), position);
		_world->TreeUprooted(player, object);
	}
	if (registry.AllOf<Pot>(object))
	{
		// A pot no longer calls people to it while it is held
		_world->RemovePotReaction(object);
	}
	// A static, a rock or a dead tree taken stops being its town's artefact
	_world->ArtefactTaken(object, player);

	// It is held at its own size
	auto pose = _world->PoseOf(object);
	pose.axes = _world->UnstretchedAxes(object, pose.axes);
	_world->SetPose(object, pose);
	registry.AssignOrReplace<InHand>(object, InHand {.hand = _world->Hand()});
	grab.state = HandGrab::State::Holding;
	grab.object = object;
	grab.tug.reset();
	grab.pulling = false;
	grab.hold = HoldOfObject(object);
	grab.lowering = grab.hold.loweringMultiplier * _world->SizeOf(object).height;
	grab.springOn = false;
	grab.springPending = false;
	grab.lastPickedUp = object;
	registry.SetDirty();
}

void HandGrabSystem::LetGo(HandGrab& grab, glm::vec3 velocity, bool forced)
{
	auto& registry = _world->Entities();
	const auto object = grab.object;
	Empty(grab);
	// It gets a twist a little after it leaves the hand, whatever becomes of it
	grab.released = object;
	grab.releaseSpinMs = static_cast<int32_t>(hand_grab::k_ReleaseSpinDelayMs);
	if (!Exists(object))
	{
		return;
	}
	registry.Remove<InHand>(object);
	if (!forced)
	{
		// It starts from where it is drawn in the hand, lifted out of the land, and laid along the slope when let go
		// slowly, unless it is any kind of tree
		const bool anyTree = registry.AnyOf<Tree, DeadTree>(object);
		if (const auto pose = _world->ReleasePose(object, !(anyTree || hand_grab::IsFastRelease(velocity))))
		{
			_world->SetPose(object, *pose);
		}
	}
	const auto player = _world->HandPlayer();
	if (!forced && registry.AllOf<Pot>(object))
	{
		// Let go over the land, a pot calls the people to it again before it is poured or thrown
		_world->SetUpPotReaction(object, player);
	}
	// A pot let go slowly is poured out where it is, onto what takes it or into a pile
	if (registry.AllOf<Pot>(object) && hand_grab::PotPours(velocity))
	{
		if (const auto pour = _world->PourPot(object, player))
		{
			grab.pourEffect = pour;
			grab.pourSeconds = 0.0f;
		}
		registry.SetDirty();
		return;
	}
	const auto result = _world->LetGoFromHand(object, {
	                                                      .velocity = velocity,
	                                                      .player = player,
	                                                      .dontReplant = forced,
	                                                  });
	if (result.accepted)
	{
		grab.lastDropped = object;
	}
	// A player a script made proof against the wind throws things that fly without the air's drag
	if (_world->PlayerHasNoWindResistance(player))
	{
		_world->DropDrag(object);
	}
	registry.SetDirty();
}

void HandGrabSystem::Empty(HandGrab& grab)
{
	grab.state = HandGrab::State::Empty;
	grab.object = entt::null;
	grab.waits = false;
	grab.pulling = false;
	grab.pullSeconds = 0.0f;
	grab.tug.reset();
	grab.rise = 0.0f;
	grab.springOn = false;
	grab.springPending = false;
	grab.scoopSource = entt::null;
	grab.scoopTurns = 0;
}

glm::vec3 HandGrabSystem::UpdateFrame(const Frame& frame)
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return frame.target;
	}
	grab->handSize = frame.handSize;
	grab->handPoint = frame.cursorGround;
	// A pour lasts three quarters of a second of the game's time
	if (grab->pourEffect.has_value())
	{
		grab->pourSeconds += static_cast<float>(frame.gameMs) * 0.001f;
		if (grab->pourSeconds > hand_grab::k_PourSeconds)
		{
			_world->StopScoopStream(*grab->pourEffect);
			grab->pourEffect.reset();
		}
	}
	// A scoop's stream flows into the hand where it now is, for at most a minute
	if (grab->scoopStream.has_value())
	{
		grab->scoopStreamSeconds += static_cast<float>(frame.gameMs) * 0.001f;
		if (grab->scoopStreamSeconds > hand_grab::k_ScoopStreamSeconds)
		{
			_world->StopScoopStream(*grab->scoopStream);
			grab->scoopStream.reset();
		}
		else
		{
			_world->MoveScoopStream(*grab->scoopStream, _world->PoseOf(_world->Hand()).origin);
		}
	}

	// The twist of what it let go, once the hand has moved on a little
	if (grab->state == HandGrab::State::Grabbing)
	{
		// While it pulls a thing the twist waits; while it waits to catch a flying one the twist is made ready again
		if (!grab->pulling)
		{
			grab->releaseSpinMs = static_cast<int32_t>(hand_grab::k_ReleaseSpinDelayMs);
		}
	}
	else if (grab->releaseSpinMs.has_value() && grab->state != HandGrab::State::Holding &&
	         grab->state != HandGrab::State::ReadyToThrow)
	{
		switch (hand_grab::CountDown(*grab->releaseSpinMs, frame.gameMs))
		{
		case hand_grab::Countdown::Running:
			break;
		case hand_grab::Countdown::RunOut:
			if (const auto body = _world->BodyOf(grab->released); body.has_value() && frame.cursorGround.has_value())
			{
				const auto moved = *frame.cursorGround - grab->lastTarget;
				_world->TwistBody(grab->released, hand_grab::ReleaseSpinTorque(body->mass, body->speed, moved));
			}
			grab->releaseSpinMs.reset();
			grab->released = entt::null;
			break;
		case hand_grab::Countdown::Cancelled:
			grab->releaseSpinMs.reset();
			grab->released = entt::null;
			break;
		}
	}

	switch (grab->state)
	{
	case HandGrab::State::Empty:
		grab->rise = 0.0f;
		return frame.target;
	case HandGrab::State::Grabbing:
	{
		if (!Exists(grab->object))
		{
			Empty(*grab);
			return frame.target;
		}
		// A thing in flight is caught once the button has been held a while; anything else is taken as soon as the hand
		// has pulled it free
		if (_world->IsFlying(grab->object))
		{
			if (hand_grab::ElapsedMs(frame.nowMs, grab->pressMs, frame.turn, grab->pressTurn) >= hand_grab::k_GrabWaitMs)
			{
				Take(*grab, grab->object);
			}
			return frame.target;
		}
		if (!grab->pulling)
		{
			Take(*grab, grab->object);
			return frame.target;
		}
		if (!grab->tug.has_value())
		{
			StartPull(*grab, frame);
		}
		return Pull(*grab, frame);
	}
	case HandGrab::State::Holding:
	case HandGrab::State::ReadyToThrow:
		break;
	}

	// The hand rises by how far what it holds hung last frame, which it then measures again
	const auto* tree = _world->Entities().TryGet<const Tree>(grab->object);
	const auto size = _world->SizeOf(grab->object);
	const std::optional<float> rooted = tree != nullptr ? std::optional(size.height) : std::nullopt;
	grab->rise = hand_grab::HandRise(grab->hold.type, grab->lowering, frame.handSize, rooted);
	grab->lowering = grab->hold.loweringMultiplier * size.height;
	// While it holds something its twist is made ready again, for when it lets go
	grab->releaseSpinMs = static_cast<int32_t>(hand_grab::k_ReleaseSpinDelayMs);
	grab->lastTarget = frame.target;
	auto target = frame.target + glm::vec3(0.0f, grab->rise, 0.0f);
	// The game time spent holding is counted whether the spring is on or not, scooping too
	grab->spring.Count(frame.gameMs);
	if (grab->scoopSource != entt::null && Exists(grab->scoopSource))
	{
		// Scooping, the hand stays where it began, over the land by the height of what it scoops from and three more
		target = grab->scoopAnchor;
		target.y = _world->LandHeightAt(target) + _world->SizeOf(grab->scoopSource).height + hand_grab::k_ScoopHoverAbove;
		grab->lastTarget = target;
		return target;
	}
	if (grab->springOn)
	{
		grab->spring.Step(target);
		return grab->spring.Position();
	}
	if (grab->springPending && grab->state == HandGrab::State::ReadyToThrow)
	{
		// The spring takes hold at the hand where it now is, at rest, and steps from the next frame
		grab->spring.Start(target);
		grab->springOn = true;
		grab->springPending = false;
	}
	return target;
}

void HandGrabSystem::ProcessTurn()
{
	auto* grab = Grab();
	if (grab == nullptr)
	{
		return;
	}
	if (!Exists(grab->lastPickedUp))
	{
		grab->lastPickedUp = entt::null;
	}
	if (!Exists(grab->lastDropped))
	{
		grab->lastDropped = entt::null;
	}
	if (grab->state != HandGrab::State::Holding && grab->state != HandGrab::State::ReadyToThrow)
	{
		return;
	}
	// What is no longer there leaves the hand
	if (!Exists(grab->object))
	{
		EndScoop(*grab);
		Empty(*grab);
		return;
	}
	// A scoop goes on while its source is there, the hand is in its player's influence and the source gives more
	if (grab->scoopSource != entt::null)
	{
		++grab->scoopTurns;
		const auto hand = _world->PoseOf(_world->Hand()).origin;
		if (!Exists(grab->scoopSource) || !_world->InInfluence(_world->HandPlayer(), hand) || !Scoop(*grab))
		{
			EndScoop(*grab);
		}
	}
	// The hold is asked again, as what is held may have changed
	grab->hold = HoldOfObject(grab->object);
	// What is held over a fire catches from it, inside the holder's influence only
	if (_world->InInfluence(_world->HandPlayer(), _world->PoseOf(grab->object).origin))
	{
		_world->HeatHeld(grab->object);
	}
}

void HandGrabSystem::ForceDrop()
{
	auto* grab = Grab();
	if (grab == nullptr || (grab->state != HandGrab::State::Holding && grab->state != HandGrab::State::ReadyToThrow))
	{
		return;
	}
	// A scoop ends with it
	if (grab->scoopSource != entt::null)
	{
		EndScoop(*grab);
	}
	// Put down from where it is held, with no speed, and never planted again
	LetGo(*grab, glm::vec3(0.0f), true);
}

void HandGrabSystem::Reset()
{
	if (auto* grab = Grab())
	{
		if (grab->scoopSource != entt::null)
		{
			_world->PinCursor(false);
		}
		*grab = HandGrab {};
	}
}

std::optional<entt::entity> HandGrabSystem::GetHeld() const
{
	const auto* grab = Grab();
	if (grab == nullptr || (grab->state != HandGrab::State::Holding && grab->state != HandGrab::State::ReadyToThrow))
	{
		return std::nullopt;
	}
	return grab->object;
}

bool HandGrabSystem::IsBusy() const
{
	const auto* grab = Grab();
	return grab != nullptr && grab->state != HandGrab::State::Empty;
}

std::optional<HandGrabSystemInterface::HeldPose> HandGrabSystem::GetHeldPose() const
{
	const auto held = GetHeld();
	if (!held.has_value() || !Exists(*held))
	{
		return std::nullopt;
	}
	const auto& grab = *Grab();
	// It hangs its height times its lowering below the hand
	return HeldPose {
	    .object = *held,
	    .hold = grab.hold.type,
	    .hang = grab.hold.loweringMultiplier * _world->SizeOf(*held).height,
	    .reach = grab.hold.holdRadius,
	};
}

std::optional<HandGrabSystemInterface::PullPose> HandGrabSystem::GetPullPose() const
{
	const auto* grab = Grab();
	if (grab == nullptr || grab->state != HandGrab::State::Grabbing || !grab->pulling || !grab->tug.has_value() ||
	    !Exists(grab->object))
	{
		return std::nullopt;
	}
	return PullPose {.hold = grab->hold.type, .reach = grab->hold.holdRadius};
}

float HandGrabSystem::GetCursorRaise() const
{
	const auto* grab = Grab();
	return grab != nullptr ? hand_grab::CursorRaise(grab->rise) : 0.0f;
}
