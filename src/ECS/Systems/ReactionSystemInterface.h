/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstdint>

#include <optional>
#include <span>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// Something happening that the living around it react to: a miracle cast or striking, something crushed. Each has a
/// kind from the game's reaction table, which says how far it reaches, how urgent it is, how long the living react and
/// whether it impresses. It is spread to the villagers and creatures in reach as it is made and again in turn with the
/// others, one a game turn; each takes it up when it is urgent enough to it and it may react to that kind again, or in
/// place of what it reacts to when more urgent and of another kind. A villager flees a miracle that frightens or watches
/// one that pleases; a creature runs from one or examines it, and learns it. Taking up a reaction that impresses gives
/// belief in its player to a villager's town and impresses a creature; a town grows weary of seeing the same kind again
/// and again.
class ReactionSystemInterface
{
public:
	/// A reaction as it is made
	struct Source
	{
		/// What it comes from, which takes it away again: a miracle, a crushed thing
		entt::entity initiator {entt::null};
		Reaction type {Reaction::None};
		PlayerNames player {PlayerNames::NEUTRAL};
		glm::vec3 position {0.0f};
		/// How impressive its initiator is, from its table, and how strong it is now
		float impressiveValue {0.0f};
		float power {1.0f};
		/// How strong its initiator is: a miracle's strength, 1 for anything else. It scales the cells its spread covers.
		float strength {1.0f};
		/// For a miracle, its magic type, which the creatures in reach watch and learn from; None for anything else
		MagicType magicType {MagicType::None};
		/// The creature that cast it, if a creature did: it neither flees its own miracle nor is impressed by it
		entt::entity casterCreature {entt::null};
		/// How far it reaches, when not as far as its table says: a shield's reaches its edge and a little beyond
		std::optional<float> reach;
		/// Made by what belongs to no player, such as a crushed tree: it impresses no one and moves no alignment
		bool playerless {false};
		/// Made as its miracle was cast: its time starts at once rather than when someone first takes it up
		bool onCast {false};
	};

	/// A reaction going on
	struct Active
	{
		uint32_t id {0};
		Source source;
		/// Game turns since it was made
		uint32_t age {0};
		/// How far it reaches, from the table
		float reach {0.0f};
		/// How its initiator moves, which the living fleeing it heed
		glm::vec3 velocity {1.0f, 0.0f, 0.0f};
		/// The age at which its time started: as it was made for one made as its miracle was cast, else as the first
		/// living thing took it up; none until then
		std::optional<uint32_t> firstTaken;
	};

	virtual ~ReactionSystemInterface() = default;

	/// A new reaction, its id
	virtual uint32_t Create(const Source& source) = 0;
	/// A reaction follows its initiator, as a miracle moves to where its last event was, with how it moves and how strong
	/// it is now, which sets how far it spreads
	virtual void Move(uint32_t id, const glm::vec3& position, const glm::vec3& velocity, float strength) = 0;
	/// A reaction going on, by its id
	[[nodiscard]] virtual std::optional<Active> Find(uint32_t id) const = 0;
	/// Every reaction an initiator made goes, as when a miracle goes
	virtual void RemoveFrom(entt::entity initiator) = 0;
	/// The reactions of one kind an initiator made go, as a magic tree that catches fire stops being looked at
	virtual void RemoveFrom(entt::entity initiator, Reaction type) = 0;
	/// One reaction goes, as a fire that has cooled stops alarming the living
	virtual void Remove(uint32_t id) = 0;
	/// Whether a reaction is still going
	[[nodiscard]] virtual bool IsActive(uint32_t id) const = 0;
	/// Whether an initiator has a reaction going
	[[nodiscard]] virtual bool HasReaction(entt::entity initiator) const = 0;
	/// Once a game turn: the reactions age, and impress who is in their reach
	virtual void ProcessTurn() = 0;
	/// A new land: none going
	virtual void Reset() = 0;
	/// The balance of impressiveness the land sets, 1 unless a script changes it
	virtual void SetLandBalance(float balance) = 0;
	[[nodiscard]] virtual float GetLandBalance() const = 0;

	[[nodiscard]] virtual std::span<const Active> GetReactions() const = 0;
	/// The reactions whose reach takes in a point
	[[nodiscard]] virtual std::vector<Active> ReactionsAt(const glm::vec3& point) const = 0;
};

} // namespace openblack::ecs::systems
