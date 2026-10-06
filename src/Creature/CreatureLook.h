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

#include <glm/vec3.hpp>

/// What an idle creature looks at as it looks about: the most interesting thing it can see, the nearer the better.
/// The longer it has watched one thing, the less anything else catches its eye, until what it watches goes out of sight.
namespace openblack::creature_look
{
/// The kinds of things a creature finds interesting, from most to least
enum class Interest : uint8_t
{
	Dove,
	Citadel,
	Creature,
	Animal,
	Villager,
	Abode,
	Tree,
	Fixed,
};
[[nodiscard]] float InterestOf(Interest kind);

/// After this many seconds watching one thing, nothing else catches the creature's eye
constexpr float k_BoredSeconds = 20.0f;
/// How high a creature's eyes are above its feet at size 1
constexpr float k_HeadHeight = 15.0f;

/// How far a creature looks about, in metres: bigger creatures see further
[[nodiscard]] float LookRange(float size);
/// Things within half the range are as interesting as they get; further ones less so, down to nothing at the range
[[nodiscard]] float DistanceFactor(float distance, float range);

/// The creature looking about
struct Viewer
{
	glm::vec3 position;
	/// Which way it faces, on the ground
	glm::vec3 ahead;
	float size;
};
/// Whether a creature can see a point: in front of it, within its range
[[nodiscard]] bool CanSee(const Viewer& viewer, const glm::vec3& point);

struct Candidate
{
	uint32_t id;
	Interest kind;
	/// Where the creature looks to look at it
	glm::vec3 point;
};

/// What the creature watches and for how many turns it has
struct Target
{
	std::optional<uint32_t> id;
	Interest kind {Interest::Fixed};
	glm::vec3 point {0.0f};
	uint32_t watchedTurns {0};
};

/// How much a thing catches the creature's eye from where it is
[[nodiscard]] float Priority(const Viewer& viewer, Interest kind, const glm::vec3& point);

/// One game turn of looking about: what it watches, if still in sight, is watched a turn longer; then each candidate in
/// sight, its priority less as the creature grows bored, takes over if it beats what is watched. Candidates should
/// include the thing watched, where it is now; if it isn't among them, it is no longer there.
[[nodiscard]] Target LookAbout(Target target, std::span<const Candidate> candidates, const Viewer& viewer,
                               float turnsPerSecond);

/// Where to look when nothing is interesting: straight ahead at head height
[[nodiscard]] glm::vec3 PointAhead(const Viewer& viewer);
} // namespace openblack::creature_look
