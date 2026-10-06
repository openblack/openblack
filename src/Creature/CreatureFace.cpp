/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureFace.h"

#include <array>

using namespace openblack;
using namespace openblack::creature_face;

namespace
{
constexpr std::array<std::string_view, k_FaceCount> k_FaceNames {
    "smile", "grimace", "growl", "scared", "sad", "amazed", "puzzled", "laugh", "ooh", "aah", "spare1", "spare2",
};
constexpr std::array<std::string_view, k_CueCount> k_CueNames {
    "none",       "idle",        "attitude to player",
    "curiosity",  "anger",       "fear",
    "compassion", "playfulness", "smile",
    "grimace",    "amazed",      "puzzled",
    "frightened", "sad",         "exhausted",
    "stroked",    "tattooed",    "lost",
    "told",
};

/// One of two faces by the variety
Request Alternate(uint32_t variety, Request even, Request odd)
{
	return variety % 2 == 0 ? even : odd;
}
} // namespace

std::string_view creature_face::Name(Face face)
{
	return k_FaceNames.at(static_cast<size_t>(face));
}

std::optional<Face> creature_face::FaceOf(size_t animation)
{
	if (animation < k_FirstFaceAnimation || animation >= k_FirstFaceAnimation + k_FaceCount)
	{
		return std::nullopt;
	}
	return static_cast<Face>(animation - k_FirstFaceAnimation);
}

std::string_view creature_face::Name(Cue cue)
{
	return k_CueNames.at(static_cast<size_t>(cue));
}

std::optional<Request> creature_face::Choose(Cue cue, const Feelings& feelings)
{
	const auto face = [cue](Face pulled, float milliseconds) {
		return Request {.face = pulled, .milliseconds = milliseconds, .cue = cue};
	};
	switch (cue)
	{
	case Cue::Idle:
		return face(static_cast<Face>(feelings.idlePick % k_IdleFaceCount), 3000.0f);
	case Cue::AttitudeToPlayer:
		return feelings.attitudeToPlayer >= 0.0f ? face(Face::Smile, 2000.0f) : face(Face::Sad, 2000.0f);
	case Cue::Curiosity:
		switch (feelings.variety % 3)
		{
		case 0:
			return face(Face::Puzzled, 1000.0f);
		case 1:
			return face(Face::Amazed, 1000.0f);
		default:
			return face(Face::Aah, 800.0f);
		}
	case Cue::Anger:
		return Alternate(feelings.variety, face(Face::Growl, 2000.0f), face(Face::Grimace, 1000.0f));
	case Cue::Fear:
	case Cue::Frightened:
		return Alternate(feelings.variety, face(Face::Scared, 2000.0f), face(Face::Ooh, 500.0f));
	case Cue::Compassion:
		return Alternate(feelings.variety, face(Face::Smile, 2000.0f), face(Face::Aah, 1000.0f));
	case Cue::Playfulness:
		return Alternate(feelings.variety, face(Face::Smile, 1000.0f), face(Face::Puzzled, 1000.0f));
	case Cue::Smile:
		return face(Face::Smile, 2000.0f);
	case Cue::Grimace:
		return face(Face::Grimace, 2000.0f);
	case Cue::Amazed:
		return face(Face::Amazed, 3000.0f);
	case Cue::Puzzled:
		return face(Face::Puzzled, 3000.0f);
	case Cue::Sad:
		return face(Face::Sad, 2000.0f);
	case Cue::Exhausted:
		return face(Face::Grimace, 1000.0f);
	case Cue::None:
	case Cue::Stroked:
	case Cue::Tattooed:
	case Cue::Lost:
	case Cue::Told:
	case Cue::_Count:
	default:
		return std::nullopt;
	}
}
