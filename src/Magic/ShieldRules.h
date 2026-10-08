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

#include <array>
#include <optional>
#include <span>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Enums.h"

namespace openblack
{
struct GMagicShieldInfo;
}

// The two shield miracles' own rules. The spiritual shield raises a sphere a little bigger than the circle drawn for it,
// which turns away the other miracles; the physical shield raises a solid dome that the things thrown at it bounce off,
// each blow costing it prayer power by how hard it struck. Both keep the enemy's hand from having any influence under
// them, have the people near them shelter under them, and are walked round by other players' creatures. Pure rules,
// tested without the game.

namespace openblack::magic::shield
{

/// The spiritual shield's sphere is this much bigger than the circle drawn for it, and never bigger than the most
inline constexpr float k_SphereRadiusScale = 1.11062f;
inline constexpr float k_MaxSphereRadius = 1000.0f;
/// The sphere a spiritual shield of a radius raises
[[nodiscard]] float SphereRadius(float radius);

/// The people react to a shield this far beyond its edge
inline constexpr float k_ReactionMargin = 30.0f;
[[nodiscard]] constexpr float ReactionReach(float radius)
{
	return radius + k_ReactionMargin;
}
/// The town a shield protects is the nearest within this distance of it
inline constexpr float k_NearestTownDistance = 500.0f;

/// The physical shield's dome as it is raised: how big it grows to and starts at, how fast it starts and ends spinning,
/// how far it is sunk into the land and how far it bobs
struct DomeShape
{
	float finalScale {0.0f};
	float startScale {0.0f};
	float startSpin {0.0f};
	float endSpin {0.0f};
	float raiseWithScale {0.0f};
	float shieldHeight {0.0f};
	float bobMagnitude {0.0f};
};
/// The dome's model is drawn at this scale for each unit of the shield's radius, starting at a hundredth of that
inline constexpr float k_DomeScalePerRadius = 0.017f;
inline constexpr float k_DomeStartFraction = 0.01f;
/// The spin the caster's hand gives it, at most this fast either way in radians a second, slowing to the end spin
inline constexpr float k_DomeMaxSpin = 3.0f;
inline constexpr float k_DomeEndSpin = 0.15f;
[[nodiscard]] DomeShape MakeDomeShape(const GMagicShieldInfo& info, float radius, float castSpin);

/// What the dome keeps from turn to turn
struct DomeState
{
	float angle {0.0f};
	float bob {0.0f};
	/// Seconds since it started dying, once its miracle let go of it
	float dieTime {0.0f};
	bool dying {false};
};

/// The dome this turn
struct DomePose
{
	float scale {0.0f};
	/// Its turn about the vertical
	float angle {0.0f};
	/// How far above the land it stands, negative for sunk into it
	float height {0.0f};
	/// Drawn, once its first half second has passed
	bool drawn {false};
	/// Gone, once it has died away
	bool gone {false};
};
/// The dome is hidden for this long, grows for this long, slows its spin over this long, and bobs this fast
inline constexpr float k_DomeHiddenSeconds = 0.5f;
inline constexpr float k_DomeGrowSeconds = 1.5f;
inline constexpr float k_DomeSpinDownSeconds = 6.0f;
inline constexpr float k_DomeBobSpeed = 1.3f;
/// Dying, it fades over this long and is gone after half as long again
inline constexpr float k_DomeFadeSeconds = 1.5f;
inline constexpr float k_DomeGoneSeconds = k_DomeFadeSeconds * 1.5f;
/// The ease the dome grows and slows by: x + x^2 - x^3, which rises from 0 to 1 and overshoots a little on the way
[[nodiscard]] constexpr float Ease(float x)
{
	return x + (x * x) - (x * x * x);
}
/// A turn of the dome, `age` seconds after it was raised and `dt` seconds long
[[nodiscard]] DomePose StepDome(const DomeShape& shape, DomeState& state, float age, float dt);
/// How opaque the dome and its effect are drawn, 0 to 255: by its strength, never fainter than 40
inline constexpr float k_DomeLeastAlpha = 40.0f;
[[nodiscard]] float DomeAlpha(float strength);
/// What a thing thrown at the dome weighs: its kind's weight times the cube of its size, never under a hundredth
[[nodiscard]] float ThrownMass(float weight, float scale);
/// The dome's effect keeps its alpha while the dome fades, then goes at once
[[nodiscard]] float DyingEffectAlpha(float alpha, float dieTime);
/// The dome's solid shape follows its drawn size only once they differ by this much
inline constexpr float k_DomeRescaleStep = 0.3f;
[[nodiscard]] bool NeedsRescale(float drawnScale, float solidScale);

/// The dome's solid shape: a cone as wide as its model's widest and twice its model's half height tall
struct DomeVolume
{
	float radius {0.0f};
	float height {0.0f};
};
[[nodiscard]] DomeVolume VolumeOf(glm::vec3 meshHalfExtent, float scale);
/// Whether a point is surely within a spiritual shield: nearer its middle than its radius, measured in three dimensions
/// between the two points on the land
[[nodiscard]] bool WithinSpiritualShield(glm::vec3 shieldOnLand, float radius, glm::vec3 pointOnLand);
/// Whether a point a height above the land is surely within a physical shield's dome: nearer across than its radius,
/// and below the cone from its height at the middle to nothing at its edge
[[nodiscard]] bool WithinPhysicalShield(const DomeVolume& volume, float distanceAcross, float heightAboveLand);
/// Whether a point is inside the dome: its distance across the land from the middle and its height above the land
[[nodiscard]] bool InsideDome(const DomeVolume& volume, float distanceAcross, float heightAboveLand);
/// The dome's solid shape as it is raised: its model's physics triangles at its full size, at a height above the land of
/// its raise with scale (at a scale of 1, as the game takes it) and shield height, unturned
/// Where the dome's solid shape stands, its model's origin, from the land under it
[[nodiscard]] glm::vec3 DomeHullOrigin(glm::vec3 ground, const DomeShape& shape);
[[nodiscard]] std::vector<std::array<glm::vec3, 3>> DomeHull(std::span<const std::array<glm::vec3, 3>> modelTriangles,
                                                             glm::vec3 ground, const DomeShape& shape);
/// Where a thing moving from one point to another crosses into the dome's solid shape, and the face's outward normal:
/// only a face it moves against counts, so a thing inside passes out freely
struct HullHit
{
	glm::vec3 point;
	glm::vec3 normal;
};
[[nodiscard]] std::optional<HullHit> CrossHull(std::span<const std::array<glm::vec3, 3>> hull, glm::vec3 from, glm::vec3 to);

/// What a blow costs the dome: its prayer power per momentum times the speed and mass of what struck it
inline constexpr float k_ImpactCostScale = 0.0001f;
[[nodiscard]] constexpr float ImpactCost(float costPerMomentum, float speed, float mass)
{
	return speed * mass * costPerMomentum * k_ImpactCostScale;
}

/// Whether a point is under a shield, its edge drawn in by a margin
[[nodiscard]] bool IsUnder(glm::vec2 point, glm::vec2 centre, float radius, float margin);

/// A villager reacting to a shield goes under it unless it is already well inside, within this share of its radius
inline constexpr float k_ShelterInside = 0.8f;
[[nodiscard]] bool NeedsShelter(float distance, float radius);
/// Where a villager shelters: on its own side of the shield's middle, turned by up to an eighth of a half turn either
/// way (`turn`, from a random number up to a quarter turn), at a distance a random cube (`u`, 0 to 1) in from 0.8 of the
/// radius. It then faces a point a step further out, turned afresh.
inline constexpr float k_ShelterTurnRange = 0.785398163f;
struct Shelter
{
	glm::vec2 point;
	glm::vec2 faceTowards;
};
[[nodiscard]] Shelter ShelterAt(glm::vec2 centre, glm::vec2 villager, float radius, float turn, float u, float faceTurn);
/// Where a sheltering villager looks next: a step on from where it stands, away from the shield's middle, turned by the
/// same spread
[[nodiscard]] glm::vec2 LookOut(glm::vec2 centre, glm::vec2 villager, float turn);
/// The animation a sheltering villager plays, by a roll of 0 to 4: pointing up, looking at its hand, or standing; after
/// pointing up it talks and points
[[nodiscard]] uint32_t AmazedAnimation(uint32_t roll);
inline constexpr uint32_t k_PointingAnimation = 286;
inline constexpr uint32_t k_TalkingAndPointingAnimation = 395;
/// A sheltering villager takes up a new animation after 20 to 30 seconds (10 times a roll of 0 to 1, and 20)
inline constexpr float k_AnimationSecondsRange = 10.0f;
inline constexpr float k_AnimationSecondsLeast = 20.0f;
/// One whose shield has gone waits 20 and a roll of 0 to 59 turns before deciding what to do
inline constexpr uint32_t k_GiveUpWaitLeast = 20;
inline constexpr uint32_t k_GiveUpWaitRange = 60;
/// How urgent a shield is to a villager, before its distance counts: its reaction's priority for one without a town;
/// for one with a town, only while its town wants protection at all and was attacked within the villager's kind's
/// turns of interest
[[nodiscard]] uint8_t VillagerPriority(uint8_t priority, bool hasTown, float protectionSignificance, uint32_t sinceAttacked,
                                       uint32_t interestedFor);
/// How long a villager keeps reacting to a shield, rolled afresh each turn: for ever, three turns in four (a roll of 1
/// to 3 of 0 to 3), while its town was attacked within its turns of interest less a roll of 0 to 49; otherwise the
/// reaction's own turns. The rolls are only made for a villager with a town, the second only after a first of 1 to 3.
[[nodiscard]] uint32_t VillagerReactTurns(bool hasTown, uint32_t roll4, uint32_t roll50, uint32_t sinceAttacked,
                                          uint32_t interestedFor, uint32_t standardTurns);
/// How long before a villager reacts to a shield again: never while its town was last attacked longer ago than its
/// turns of interest; at once while where it is walking to is not under the shield; otherwise the reaction's own turns
[[nodiscard]] uint32_t VillagerAgainTurns(bool hasTown, uint32_t sinceAttacked, uint32_t interestedFor, bool goalUnder,
                                          uint32_t standardTurns);
/// A sheltering villager turns this much of a full turn each game turn to face where it looks
inline constexpr float k_LookTurnPerTurn = 0x40 / static_cast<float>(0x800);

/// Whether a creature walks round a shield: any creature of another player than the shield's, unless a script is
/// moving it
[[nodiscard]] constexpr bool CreatureMustAvoid(PlayerNames creature, PlayerNames shield, bool scripted)
{
	return !scripted && creature != shield;
}

/// How impressive a reaction to a shield is, times its miracle's usual value: nothing to the people of a town the
/// shield's own player attacked as it went up or was struck, four times as much as it is destroyed
[[nodiscard]] float ImpressiveMultiplier(Reaction reaction, bool townAttackedByShieldPlayer, bool physical);
inline constexpr float k_DestroyedImpressiveness = 4.0f;

} // namespace openblack::magic::shield
