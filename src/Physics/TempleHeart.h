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

#include <span>
#include <vector>

#include <entt/entity/entity.hpp>
#include <glm/vec3.hpp>

/// What a temple's heart does when something thrown strikes it: it passes the blow on to one of its player's buildings
/// or homeless people, or takes the harm itself
namespace openblack::physics::temple_heart
{

/// A building the blow might pass to
struct Building
{
	entt::entity entity {entt::null};
	/// Still in the world to be struck
	bool available {false};
	float life {0.0f};
	float built {0.0f};
	bool field {false};
	bool footballPitch {false};
};
/// A homeless villager the blow might pass to
struct Homeless
{
	entt::entity entity {entt::null};
	/// Still in the world and not on its way to dying
	bool available {false};
};
/// One of the heart's player's towns: its buildings, the newest first, and its homeless people, the newest first
struct Town
{
	std::vector<Building> buildings;
	std::vector<Homeless> homeless;
};

/// A building with more life than this takes the blow at once
inline constexpr float k_SoundLife = 0.25f;

enum class TargetKind : uint8_t
{
	/// Nothing to pass it to: the heart itself
	Heart,
	Building,
	Villager,
};
struct Target
{
	TargetKind kind {TargetKind::Heart};
	entt::entity entity {entt::null};
};
/// Over the player's towns in the order the player gained them: the first building standing, built and alive that isn't a
/// field or a football pitch, at once when its life is over a quarter; else, after every town, the first such building
/// found; else the first available homeless villager of any town; else the heart
[[nodiscard]] Target Choose(std::span<const Town> towns);

/// The harm a striking body does the heart: its speed times its mass times a fraction, at most a fifth
inline constexpr float k_HarmPerMomentum = 0.000005f;
inline constexpr float k_MostHarm = 0.2f;
[[nodiscard]] float Harm(glm::vec3 velocity, float mass);

/// A villager the blow passes to is flung straight up, spinning, by no one
inline constexpr glm::vec3 k_VillagerLaunch {0.0f, 30.0f, 0.0f};
inline constexpr glm::vec3 k_VillagerLaunchSpin {1.0f, 0.0f, 1.0f};

} // namespace openblack::physics::temple_heart
