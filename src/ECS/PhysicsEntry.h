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

#include <memory>
#include <optional>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"
#include "Physics/Body.h"

namespace openblack::ecs
{

/// One body of the physics: an object thrown, dropped, knocked or pushed (moving), or a resting obstacle near a moving
/// body
struct PhysicsEntry
{
	/// What the body is to the physics' own bookkeeping
	enum class Kind : uint8_t
	{
		Other,
		Villager,
		/// A tree a forester felled, still falling; it makes a sound as it topples
		FelledTree,
		FelledTreeToppled,
	};

	enum Flags : uint8_t
	{
		/// Something moves near it this turn
		k_Awake = 0x1,
		/// A living thing pushed or kicked it, or it is a felled tree: villagers' bodies pass through it
		k_PushedByLiving = 0x2,
		/// A hand let it go
		k_FromHand = 0x4,
		/// A hand put it down gently
		k_Landed = 0x8,
		/// Its points hit no other object (a villager's dropped load, building pieces)
		k_NoObjectCollision = 0x10,
		/// It stays in the physics whether or not anything moves near it
		k_AlwaysStays = 0x80,
	};

	entt::entity entity {entt::null};
	/// What threw it; the two pass through each other while it is in the physics
	entt::entity thrower {entt::null};
	/// The player whose hand let it go, or who threw what knocked it: credited with what it does
	std::optional<PlayerNames> player;
	std::unique_ptr<physics::Body> body;
	/// The body that hit it at the end of the last turn it was hit
	PhysicsEntry* hitBy {nullptr};
	/// The mean force of the last turn it was knocked
	float impact {0.0f};
	/// The force summed over the turn's steps it touched something
	glm::vec3 forceSum {0.0f};
	Kind kind {Kind::Other};
	uint8_t flags {0};
	/// Of its object's kind: its points are tested against other bodies' faces (buildings only take hits)
	bool checksPoints {true};
	/// Of its object's kind: moved by bones, drawn turned a quarter from its body
	bool animated {false};
	/// Of its object's kind: it takes only its body's heading and stands upright
	bool upright {false};

	[[nodiscard]] bool Has(Flags flag) const { return (flags & flag) != 0; }
	[[nodiscard]] bool IsFlying() const { return body != nullptr && !body->resting; }
};

/// What a body knows of its last turn's knock, for its object's kind to react to
struct ImpactInfo
{
	/// The knock in multiples of the body's own weight
	float g {0.0f};
	float impact {0.0f};
	/// The object whose body hit it, none when only the land or the sea did
	entt::entity hitBy {entt::null};
	/// The object that threw what hit it, and the player credited
	entt::entity thrower {entt::null};
	std::optional<PlayerNames> player;
	bool fromHand {false};
};

} // namespace openblack::ecs
