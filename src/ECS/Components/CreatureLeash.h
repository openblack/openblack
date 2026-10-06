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

#include <bitset>
#include <optional>

#include <entt/core/hashed_string.hpp>
#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

#include "Creature/LeashRope.h"
#include "Creature/LeashRules.h"
#include "Enums.h"

namespace openblack::ecs::components
{

/// A creature's leashes: which it can be put on, the one it wears if any, how it is being led, and the area it is kept
/// within
struct CreatureLeash
{
	/// The leash texture and its alpha, Data/Textures/leash.raw and leasha.raw, which the raw textures load as
	static constexpr entt::id_type k_TextureId = entt::hashed_string("raw/leash");
	static constexpr entt::id_type k_AlphaTextureId = entt::hashed_string("raw/leasha");

	/// The leashes the creature knows, by their place in creature_leash::k_Types. Without the learning leash it can't
	/// be leashed at all.
	std::bitset<creature_leash::k_Types.size()> known;

	/// The leash it wears
	struct Worn
	{
		LeashType type {LeashType::Rope};
		/// The player holding it
		PlayerNames holder {PlayerNames::PLAYER_ONE};
		/// Whether the leash makes the creature do anything, as scripts set
		bool works {true};
		/// What it is tied to, or held in the hand when none
		std::optional<entt::entity> tiedTo;
		/// The game turn it was tied
		uint32_t tiedTurn {0};
		/// The rope from the hand, or what it is tied to, to the creature's collar
		leash_rope::Rope rope;
		/// Whether the rope has been placed from its ends yet
		bool ropeStarted {false};
	};
	std::optional<Worn> worn;
	/// The leash picked to put on next, at the citadel or by the hotkeys
	LeashType selected {LeashType::Rope};
	/// Whether the rope is drawn
	bool drawn {true};

	/// How the creature is being led: not at all, or walking to the hand
	enum class Control : uint8_t
	{
		Idle,
		WalkingToHand,
	};
	Control control {Control::Idle};
	/// How hard it is pulled, which speeds it up from walking to running
	float pull {0.0f};
	/// How often it has been pulled away from acting on each desire
	creature_leash::PullMemory pulls {};
	/// Turns leashed to another creature
	uint32_t turnsWithOther {0};

	/// The area the creature is kept within, none when the radius is 0
	glm::vec3 confinementCentre {0.0f};
	float confinementRadius {0.0f};
	/// Where its home is, which it is kept at while it starts to grow up
	std::optional<glm::vec3> home;
	/// Walking back into the area it is kept within
	bool returning {false};
};

/// One of the three leashes hanging at a citadel, which its player taps to pick that leash
struct LeashPost
{
	/// The collar mesh the posts are drawn with, Data/Misc/leash.l3d
	static constexpr entt::id_type k_MeshId = entt::hashed_string("misc/leash");

	LeashType type {LeashType::Rope};
	PlayerNames owner {PlayerNames::PLAYER_ONE};
	bool selected {false};
};

} // namespace openblack::ecs::components
