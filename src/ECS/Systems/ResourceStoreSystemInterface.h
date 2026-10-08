/*******************************************************************************
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

#include <entt/fwd.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack::ecs::systems
{

/// The stores of food and wood and their piles, shared by the miracles that pour into them, the hand that gives and
/// scoops, and the physics when something thrown hits one: what each stores, what it takes of what it is given, what
/// giving and taking count for in its town, and the things it takes whole
class ResourceStoreSystemInterface
{
public:
	virtual ~ResourceStoreSystemInterface() = default;

	/// What an object would give a store: its kind of resource and how much of it, nothing for what isn't a resource
	struct ObjectResource
	{
		ResourceType type {ResourceType::None};
		uint32_t amount {0};
		/// It poisons what it is given to: a toadstool, or a poisoned pot
		bool poisoned {false};
	};
	[[nodiscard]] virtual ObjectResource ResourceOf(entt::entity object) const = 0;

	/// Whether something stores a resource: a storage pit stores anything; a pile only through the store it is part of
	[[nodiscard]] virtual bool IsStore(entt::entity store, ResourceType type) const = 0;
	/// The store, or the pile's store, takes what it will of an amount; giving it counts for the giver in its town. What
	/// it took.
	virtual uint32_t AddToStore(entt::entity store, ResourceType type, uint32_t amount, std::optional<PlayerNames> giver,
	                            bool poisoned) = 0;
	/// A pile takes what it will of an amount of its own resource, with its thud; a poisoned gift poisons it. What it took.
	virtual uint32_t AddToPile(entt::entity pile, ResourceType type, uint32_t amount, bool poisoned) = 0;
	/// What is taken from a pile: a store's pile gives what its store can spare, the rest coming from the store's other
	/// piles, and the taking counts against the taker in the store's town. What was taken.
	virtual uint32_t TakeFromPile(entt::entity pile, ResourceType type, uint32_t amount, std::optional<PlayerNames> taker) = 0;
	/// A store takes a thing whole, for all the resource it is worth, and the thing goes; whether it took it. Thrown by a
	/// player, the giving is that player's.
	virtual bool TakeObject(entt::entity store, entt::entity object, std::optional<PlayerNames> giver) = 0;
	/// A resource poured at a point goes to the stores and piles of it about the point, each taking what it will; what is
	/// left makes a new pile of the player's unless the point is in the water. A power-up's food sparkles over its new
	/// pile. A poisoned pour poisons every pile it goes into or makes. Whether anything was taken or made.
	virtual bool PourAt(ResourceType type, glm::vec3 point, uint32_t amount, bool speedUp, PlayerNames player,
	                    bool poisoned) = 0;
	/// The store a pile is part of, none for a pile on its own
	[[nodiscard]] virtual std::optional<entt::entity> StoreOf(entt::entity pile) const = 0;
};

} // namespace openblack::ecs::systems
