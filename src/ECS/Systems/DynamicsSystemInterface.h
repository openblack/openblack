/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>

#include <entt/entity/entity.hpp>
#include <glm/fwd.hpp>
#include <glm/mat3x3.hpp>
#include <glm/vec3.hpp>

#include "ECS/PhysicsEntry.h"

namespace openblack
{
class LandIslandInterface;
namespace ecs::components
{
struct Transform;
}
namespace particles::draw
{
struct Frame;
}
namespace physics
{
class Ground;
}

} // namespace openblack

namespace openblack::ecs::systems
{
class DynamicsSystemInterface;

/// How an object starts to move in the physics
struct PhysicsStart
{
	glm::vec3 velocity {0.0f};
	/// Its spin, about its own axes
	glm::vec3 spin {0.0f};
	/// What threw it, which it passes through while it is in the physics
	entt::entity thrower {entt::null};
	/// The player credited with what it does
	std::optional<PlayerNames> player;
	/// A body is made for it; without one, the object only leaves the map to be carried by something else
	bool add {true};
	bool fromHand {false};
};

/// How a hand, or a creature's hand, lets go of what it held
struct FromHand
{
	glm::vec3 velocity {0.0f};
	/// Its spin as it leaves the hand, as the bodies count turning
	glm::vec3 angularMomentum {0.0f};
	/// The player whose hand let it go, credited with what it does
	std::optional<PlayerNames> player;
	/// The creature whose hand let it go, none for a god's hand
	entt::entity creature {entt::null};
	/// It mustn't be planted again (a hand made to let go)
	bool dontReplant {false};
};

/// What letting go of a thing came to
struct FromHandResult
{
	/// It was let go into the physics: it wasn't in it already
	bool accepted {false};
	/// Its body, none when it couldn't move
	PhysicsEntry* entry {nullptr};
	/// It was put down where it was rather than thrown or dropped to fall
	bool landed {false};
};

/// Whether an object started to move, and its body when one was made
struct PhysicsStarted
{
	PhysicsEntry* entry {nullptr};
	bool started {false};
};

/// What each kind of object does as the physics moves it, beyond what any object does. The physics calls these at the
/// points the kinds' own rules take over; each default is what an ordinary object does.
class PhysicsClassHooks
{
public:
	virtual ~PhysicsClassHooks() = default;

	/// The kind's part of starting to move, which an ordinary object leaves to the object's own start
	virtual PhysicsStarted InitialisePhysics(DynamicsSystemInterface& dynamics, entt::entity object, const PhysicsStart& start);
	/// The kind's reaction to its last turn's knock
	virtual void ReactToImpact(DynamicsSystemInterface& dynamics, PhysicsEntry& entry, const ImpactInfo& impact);
	/// After the reaction: what the player's creature may learn from things the hand threw striking others
	virtual void ImpactFeedback(DynamicsSystemInterface& dynamics, PhysicsEntry& entry, bool hit);
	/// The kind's end of physics as its body comes to rest (an entry) or is taken out: the object that stays, none when
	/// nothing does. An ordinary object goes back into the map's cells inside the map and is deleted outside.
	virtual entt::entity EndPhysics(DynamicsSystemInterface& dynamics, PhysicsEntry* entry, entt::entity object, bool insert);
	/// Asked of a body low in the sea and denser than water: whether its kind has sunk, which ends its physics.
	/// Ordinary objects sink on until they are deleted far under the sea.
	virtual bool HasSunk(DynamicsSystemInterface& dynamics, PhysicsEntry& entry);
	/// The sound of a thing put down gently on land; only trees have one
	virtual void DropSound(entt::entity object);
	/// The kind of sound it makes as it hits or is hit
	[[nodiscard]] virtual SoundCollisionType CollideSoundType(entt::entity object) const;
	/// A felled tree, taller than the sound needs, has toppled
	virtual void FelledTreeToppled(entt::entity tree);
	/// A thrown thing starts to fly: creatures that can try to catch it
	virtual void OfferToCatchingCreatures(entt::entity object, PhysicsEntry& entry);
	/// An object that breaks buildings stops being in the physics: buildings forget it hit them
	virtual void ForgetBuildingHitter(entt::entity object);
	/// Whether a dropped object is raised up over this one (not over a vortex or a map shield)
	[[nodiscard]] virtual bool RaisesObjects(entt::entity object) const;
	/// The kind's part of a thing let go by a hand that didn't land: a person or an animal starts to fly
	virtual void StartFlyingFromHand(DynamicsSystemInterface& dynamics, PhysicsEntry& entry);
	/// A villager let go by a hand that didn't land drops what it carried, which flies on with it
	virtual void DropCarriedResource(DynamicsSystemInterface& dynamics, entt::entity villager, glm::vec3 velocity);
	/// A thing a player's hand put down: the player's creature may copy what the player did with it
	virtual void ConsiderMimickingLanding(entt::entity object, std::optional<PlayerNames> player);
	/// A toy a player's hand let go: the player's creature may think of playing with it
	virtual void ConsiderMimickingToyPlay(entt::entity toy, PlayerNames player);
};

/// The game's physics: thrown, dropped, knocked and pushed objects, simulated as the game simulates them in fixed steps
/// once a game turn (see physics::Body)
class DynamicsSystemInterface
{
public:
	virtual ~DynamicsSystemInterface() = default;

	// The objects' physics. Defaults leave a world without it, as the tests' stand-ins are.

	/// A new land: no bodies, no dust and nothing hit
	virtual void ResetSimulation() {}
	/// The kinds' own parts of the physics
	virtual void SetClassHooks([[maybe_unused]] std::unique_ptr<PhysicsClassHooks> hooks) {}
	/// Once a game turn while the game runs: the turn's start, its twenty steps and its end
	virtual void ProcessTurn() {}
	/// Every frame: where the moving bodies are drawn between turns, and the dust ageing with the game's time
	virtual void UpdateFrame([[maybe_unused]] float turnFraction, [[maybe_unused]] float gameSeconds) {}
	/// The dust the landings throw up, drawn with the particles
	virtual void CollectDrawFrame([[maybe_unused]] particles::draw::Frame& frame) const {}

	/// An object starts to move, through its kind's own start
	virtual PhysicsStarted InitialisePhysics([[maybe_unused]] entt::entity object, [[maybe_unused]] const PhysicsStart& start)
	{
		return {};
	}
	/// What any object does starting to move: refused when it is already in the physics; it leaves the map's cells,
	/// gets a body when asked, and a burning one leaves its fire's group
	virtual PhysicsStarted StartPhysicsAsObject([[maybe_unused]] entt::entity object,
	                                            [[maybe_unused]] const PhysicsStart& start)
	{
		return {};
	}
	/// A flying body for an object; none when it can't fly or flies already. A resting obstacle's body is replaced.
	virtual PhysicsEntry* AddObject([[maybe_unused]] entt::entity object, [[maybe_unused]] const PhysicsStart& start)
	{
		return nullptr;
	}
	[[nodiscard]] virtual PhysicsEntry* Find([[maybe_unused]] entt::entity object) { return nullptr; }
	/// In the physics and moving, not a resting obstacle
	[[nodiscard]] virtual bool IsFlying([[maybe_unused]] entt::entity object) const { return false; }
	/// Whether thrown it breaks buildings: rocks and the bowling ball
	[[nodiscard]] virtual bool PhysicallyDestroysAbodes([[maybe_unused]] entt::entity object) const { return false; }
	/// The object takes its body's place and the body goes, its kind's end of physics run when asked; nothing while the
	/// physics runs its turn. The object that stays.
	virtual entt::entity RemoveObject(entt::entity object, [[maybe_unused]] bool insert, [[maybe_unused]] bool endPhysics)
	{
		return object;
	}
	/// What any object does at the end of its physics: out of the physics and, when asked, back into the map's cells
	/// inside the map or deleted outside it; then what it set others reacting to as it flew is over. With a body, nothing
	/// for an object no longer in the physics.
	virtual entt::entity EndPhysicsAsObject(entt::entity object, [[maybe_unused]] bool insert, [[maybe_unused]] bool hasBody)
	{
		return object;
	}
	/// A body released over things is raised until nothing under it pushes it up
	virtual void RaiseClearOfWhatIsUnder([[maybe_unused]] PhysicsEntry& entry) {}
	/// Lays a body onto the land under it (see physics::Body::SettleOnLand)
	virtual void SettleOnLand([[maybe_unused]] PhysicsEntry& entry, [[maybe_unused]] bool noPullDown,
	                          [[maybe_unused]] bool alignToSlope)
	{
	}
	/// A hand, or a creature's hand, lets go of what it held, which starts from where it is drawn: put down where it is
	/// (lowered onto the land and lifted clear of what is under it) when let go slowly, else thrown or dropped to fly.
	/// Refused for a thing already in the physics.
	virtual FromHandResult LetGoFromHand([[maybe_unused]] entt::entity object, [[maybe_unused]] const FromHand& release)
	{
		return {};
	}
	/// Where a thing about to leave a hand starts from: its body at its place, lifted out of the land and, when asked, laid
	/// along its slope; its model's axes (scaled) and origin. None for a thing without a body.
	[[nodiscard]] virtual std::optional<std::pair<glm::mat3, glm::vec3>> ReleasePose([[maybe_unused]] entt::entity object,
	                                                                                 [[maybe_unused]] bool alignToSlope)
	{
		return std::nullopt;
	}
	/// A living thing pushes an object out of its way, which moves in the physics
	virtual float PushObject([[maybe_unused]] entt::entity object) { return 0.0f; }
	/// A puff of dust at a point, drifting by a velocity, of a size and colour (0xAARRGGBB), living a second of the game's
	/// time with the landings' dust
	virtual void AddPuff([[maybe_unused]] glm::vec3 position, [[maybe_unused]] glm::vec3 velocity, [[maybe_unused]] float size,
	                     [[maybe_unused]] uint32_t argb)
	{
	}
	/// The land as the physics feels it, none without a land
	[[nodiscard]] virtual const physics::Ground* GetGround() const { return nullptr; }

	/// The scripts' last thing hit and what hit it, each only while it still exists
	virtual void RecordHit([[maybe_unused]] entt::entity hit, [[maybe_unused]] entt::entity hitter) {}
	[[nodiscard]] virtual entt::entity GetHitObject() const { return entt::null; }
	[[nodiscard]] virtual entt::entity GetObjectWhichHit() const { return entt::null; }

	/// Every body, read only
	virtual void ForEachEntry([[maybe_unused]] const std::function<void(const PhysicsEntry&)>& visit) const {}
};

} // namespace openblack::ecs::systems
