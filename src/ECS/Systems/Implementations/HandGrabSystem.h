/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include "ECS/Systems/HandGrabSystemInterface.h"

#if !defined(LOCATOR_IMPLEMENTATIONS)
#error "Locator interface implementations should only be included in Locator.cpp, use interface instead."
#endif

#include "Hand/HandGrabRules.h"
#include "Hand/HandGrabWorld.h"

namespace openblack::ecs::components
{
struct HandGrab;
}

namespace openblack::ecs::systems
{

class HandGrabSystem final: public HandGrabSystemInterface
{
public:
	/// The hand in the game's world
	HandGrabSystem();
	/// The hand in a world of its own, as tests give it
	explicit HandGrabSystem(std::unique_ptr<hand_grab::HandGrabWorldInterface> world);
	~HandGrabSystem() override;

	bool Press(uint32_t nowMs, uint32_t turn) override;
	std::optional<entt::entity> Release(uint32_t nowMs, uint32_t turn) override;
	glm::vec3 UpdateFrame(const Frame& frame) override;
	void ProcessTurn() override;
	void ForceDrop() override;
	[[nodiscard]] bool IsInInfluence() const override { return HandInInfluence(); }
	void Reset() override;

	[[nodiscard]] std::optional<entt::entity> GetHeld() const override;
	[[nodiscard]] bool IsBusy() const override;
	[[nodiscard]] std::optional<HeldPose> GetHeldPose() const override;
	[[nodiscard]] float GetCursorRaise() const override;

private:
	/// The hand's grab, made on the hand when first needed; none without a hand
	[[nodiscard]] components::HandGrab* Grab();
	[[nodiscard]] const components::HandGrab* Grab() const;
	[[nodiscard]] bool Exists(entt::entity object) const;
	/// How the hand treats a thing
	[[nodiscard]] hand_grab::GrabKind KindOf(entt::entity object) const;
	/// What deciding whether the hand may hold a thing needs to know of it
	[[nodiscard]] hand_grab::Holdable HoldableOf(entt::entity object) const;
	/// Whether the hand may take a thing now: what it is, what a script says of it, and the hand's influence
	[[nodiscard]] bool MayTake(entt::entity object) const;
	/// A press that doesn't take the thing taps it, when the player may touch it
	void Tap(entt::entity object);
	/// How a thing hangs in the hand
	[[nodiscard]] hand_grab::HoldFacts HoldOfObject(entt::entity object) const;
	/// The hand starts pulling at a thing: its base, the plane of the land the pull works in, and its lean
	void StartPull(components::HandGrab& grab, const Frame& frame);
	/// A frame of pulling; where the hand is, on the thing
	glm::vec3 Pull(components::HandGrab& grab, const Frame& frame);
	/// The hand takes a thing it waited for or pulled free; or, not from the world, a handful it made as it scoops
	void Take(components::HandGrab& grab, entt::entity object, bool fromTheWorld = true);
	/// A press on a pile scoops a handful out of it, which grows while the button is held; whether it did
	bool StartScoop(components::HandGrab& grab, entt::entity source);
	/// A game turn of scooping: whether the scoop goes on
	bool Scoop(components::HandGrab& grab);
	using FieldFacts = hand_grab::HandGrabWorldInterface::FieldFacts;
	/// A field gives its food the same way, half of it once ripe
	bool StartFieldScoop(components::HandGrab& grab, entt::entity field, const FieldFacts& facts);
	bool ScoopField(components::HandGrab& grab, const FieldFacts& facts);
	/// The scoop ends: its stream stops, and the hand holds its handful as anything else
	void EndScoop(components::HandGrab& grab);
	/// The second press with the pointer on something the held thing is used on (a store, a pile of the same): it is
	/// given to it at once, without being thrown. Whether it was.
	bool ApplyTo(components::HandGrab& grab, entt::entity target);
	/// The hand lets go of what it holds at a velocity, put down or thrown; made to, it starts from where it is held
	/// and is never planted again
	void LetGo(components::HandGrab& grab, glm::vec3 velocity, bool forced);
	/// The hand holds nothing, having let go before taking anything or let go of what it held
	static void Empty(components::HandGrab& grab);

	/// Whether the hand's point on the land is in its player's influence
	[[nodiscard]] bool HandInInfluence() const;

	std::unique_ptr<hand_grab::HandGrabWorldInterface> _world;
};

} // namespace openblack::ecs::systems
