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

#include <optional>
#include <string_view>

/// The faces a creature pulls and why. Each face is a short expression played on top of the body: it plays through once,
/// holds the full expression for as long as it is pulled, then relaxes. What the creature is doing or feeling picks the
/// face and how long it is held, with a few feelings taking turns between two or three faces.
namespace openblack::creature_face
{
/// The expressions, in the order of the creature spec file's face animations
enum class Face : uint8_t
{
	Smile,
	Grimace,
	Growl,
	Scared,
	Sad,
	Amazed,
	Puzzled,
	Laugh,
	Ooh,
	Aah,
	Spare1,
	Spare2,
};
constexpr size_t k_FaceCount = 12;
/// The place of the first face in the creature spec file's animations
constexpr size_t k_FirstFaceAnimation = 16;
/// The idle creature pulls one of the first ten faces at random
constexpr uint32_t k_IdleFaceCount = 10;

[[nodiscard]] std::string_view Name(Face face);
[[nodiscard]] constexpr size_t AnimationOf(Face face)
{
	return k_FirstFaceAnimation + static_cast<size_t>(face);
}
/// The face an animation shows, if it is one
[[nodiscard]] std::optional<Face> FaceOf(size_t animation);

/// Why a face is pulled: what the creature is doing, or how it feels
enum class Cue : uint8_t
{
	/// No reason given
	None,
	/// Idling: a face picked at random
	Idle,
	/// Showing the player what it feels about them: a smile when it likes them, sad when it doesn't
	AttitudeToPlayer,
	/// Looking something over, or noticing something
	Curiosity,
	Anger,
	Fear,
	Compassion,
	Playfulness,
	/// Doing something that wants this face: smiling at a friend or having a poo, grimacing asleep or being sick,
	/// amazed at something it watches, puzzled at something strange
	Smile,
	Grimace,
	Amazed,
	Puzzled,
	/// Moods: frightened on the spot, sad and moping, exhausted and resting
	Frightened,
	Sad,
	Exhausted,
	/// Pleased at being stroked
	Stroked,
	/// Pleased or saddened at its tattoo being drawn
	Tattooed,
	/// Puzzled at finding no way to where it is going
	Lost,
	/// Told to by hand, from the debug tools
	Told,
	_Count
};
constexpr size_t k_CueCount = static_cast<size_t>(Cue::_Count);
[[nodiscard]] std::string_view Name(Cue cue);

/// What, besides the cue, picks the face
struct Feelings
{
	/// Feelings with two or three faces pick between them by this count, which moves on by a random step each time the
	/// creature pulls a face
	uint32_t variety {0};
	/// How much it likes the player, below 0 when it dislikes them
	float attitudeToPlayer {0.0f};
	/// From 0 to 9, the idle face's pick
	uint32_t idlePick {0};
};

/// A face to pull, for how long, and why
struct Request
{
	Face face {Face::Smile};
	float milliseconds {0.0f};
	Cue cue {Cue::None};
};

/// The face a cue pulls, if it pulls any by itself
[[nodiscard]] std::optional<Request> Choose(Cue cue, const Feelings& feelings);
} // namespace openblack::creature_face
