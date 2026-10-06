/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <span>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Creature/CreatureFace.h"

/// How the player's hand rewards and punishes a creature. The hand can't pick a creature up: clicking one holds the
/// hand to it instead. Held still over its body, the hand strokes it, and the creature plays a pleased animation for
/// the part of the body stroked. Swept fast across it, the hand slaps it, high or low and hard or gently, and it reels.
/// Each stroke and slap adds to a running sum, which the creature's mind is told of as the hand lets go.
namespace openblack::creature_feedback
{
/// The parts of the body a stroke can land on, each nearest one of the body's bones
enum class BodyPart : uint8_t
{
	Head,
	RightArmpit,
	LeftArmpit,
	Belly,
	Groin,
	RightFoot,
	LeftFoot,
	RightHand,
	LeftHand,
};
constexpr size_t k_BodyPartCount = 9;

/// For each part stroked: the pleased animation, whether it plays mirrored (the left side's are the right's mirrored),
/// and the face pulled for two seconds: an aah for the head and belly, an ooh for the groin and a smile elsewhere
constexpr std::array<size_t, k_BodyPartCount> k_RewardAnimations {154, 155, 155, 157, 158, 159, 159, 161, 161};
constexpr std::array<bool, k_BodyPartCount> k_RewardMirrored {false, false, true, false, false, false, true, false, true};
constexpr std::array<creature_face::Face, k_BodyPartCount> k_RewardFaces {
    creature_face::Face::Aah,   creature_face::Face::Smile, creature_face::Face::Smile,
    creature_face::Face::Aah,   creature_face::Face::Ooh,   creature_face::Face::Smile,
    creature_face::Face::Smile, creature_face::Face::Smile, creature_face::Face::Smile,
};
constexpr float k_RewardFaceMs = 2000.0f;
/// The face pulled being stroked on a part
[[nodiscard]] constexpr creature_face::Request RewardFace(BodyPart part)
{
	return {.face = k_RewardFaces.at(static_cast<size_t>(part)),
	        .milliseconds = k_RewardFaceMs,
	        .cue = creature_face::Cue::Stroked};
}

/// The part nearest a point the hand touches, given where each part's bone is, in part order
[[nodiscard]] BodyPart NearestPart(const glm::vec3& touch, std::span<const glm::vec3, k_BodyPartCount> parts);

/// The hand has to rest on the body this long before it strokes, and strokes again only on another part after this long
constexpr float k_StrokeHoldMs = 1000.0f;
constexpr float k_StrokeIntervalMs = 2000.0f;
/// A stroke interrupts nothing that is less than this far through; a slap nothing less than this far
constexpr float k_StrokeInterruptsAfter = 0.8f;
constexpr float k_SlapInterruptsAfter = 0.35f;
/// Slaps are at least this long apart
constexpr float k_SlapIntervalMs = 1000.0f;

/// Whether a stroke plays now: on a part other than the last stroked, long enough after the last
[[nodiscard]] bool StrokeDue(std::optional<BodyPart> last, BodyPart part, float msSinceLast);

/// The slapping animations, from the head to the feet, and their gentle versions, all to the right side
constexpr size_t k_SlapHead = 165;
constexpr size_t k_SlapWaist = 167;
constexpr size_t k_SlapFeet = 169;
constexpr size_t k_GentleOffset = 6;
/// Heights up the creature, as shares of its height, below which a slap is at the feet, then at the waist; above that
/// at the head, and nothing at or above the last
constexpr float k_FeetBelow = 0.4f;
constexpr float k_WaistBelow = 0.7f;
constexpr float k_SlapAbove = 1.1f;
/// The hand slaps when it moves faster than this many of the creature's heights a second, gently below the second
constexpr float k_SlapSpeed = 5.0f;
constexpr float k_HardSlapSpeed = 9.0f;

struct Slap
{
	size_t animation;
	bool gentle;
	/// Played to the left side, by which way the hand swept across the screen
	bool mirrored;
};

/// The slap a hand moving across the body makes, if any: by its height above the ground and its speed, against the
/// creature's height, and which way it moves across the screen
[[nodiscard]] std::optional<Slap> ClassifySlap(float handHeight, float speed, float creatureHeight, bool sweepsRight);

/// What each stroke and slap adds to the running sum of the player's feedback, which stays from -1 to 1
constexpr float k_StrokeAmount = 0.1f;
constexpr float k_GentleSlapAmount = 0.1f;
constexpr float k_HardSlapAmount = 0.2f;
/// Slapping a creature that was enjoying being stroked, the sum past this, takes twice as much off
constexpr float k_EnjoyingSum = 0.25f;

[[nodiscard]] float AfterStroke(float sum);
[[nodiscard]] float AfterSlap(float sum, bool gentle);
/// What the mind is told when the hand lets go
[[nodiscard]] float Delivered(float sum);
/// Feedback this slight only makes the creature look at the player
constexpr float k_SlightFeedback = 0.01f;

/// How the creature's attitude to the player follows feedback: mostly what it was, nudged by what it got
[[nodiscard]] float AttitudeAfter(float attitude, float feedback);
/// The average of the feedback it has had, the newest counting most
[[nodiscard]] float AverageAfter(float average, float feedback);

/// A piece of the body the hand can touch: a capsule round the line from one bone to the next
struct Capsule
{
	glm::vec3 from;
	glm::vec3 to;
	float radius;
};
/// A creature of size 1 is about this many units tall
constexpr float k_HeightAtSizeOne = 15.0f;
/// The body is taken as capsules round its bones this thick, as shares of its height
constexpr float k_BodyRadiusShare = 0.12f;
/// The body as posed: a capsule from each bone to its parent, the bones' matrices in the mesh's space placed in the world
[[nodiscard]] std::vector<Capsule> BodyCapsules(std::span<const uint32_t> parents, std::span<const glm::mat4> boneMatrices,
                                                const glm::mat4& placement, float radius);
/// How far a point is outside the body, or less than 0 inside it
[[nodiscard]] float DistanceOutside(const glm::vec3& point, std::span<const Capsule> capsules);
/// Where a line of sight first touches the body, as how many of the direction's lengths along it, if it does
[[nodiscard]] std::optional<float> RayHit(const glm::vec3& origin, const glm::vec3& direction,
                                          std::span<const Capsule> capsules);
} // namespace openblack::creature_feedback
