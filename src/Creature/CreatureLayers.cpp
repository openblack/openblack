/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureLayers.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>

using namespace openblack;
using namespace openblack::creature_layers;

namespace
{
/// Every animation but the stand plays this fast at size 0, and this much more slowly for each size bigger
constexpr float k_PlaybackRateAtSizeZero = 1.6f;
constexpr float k_PlaybackSlowingPerSize = 0.425f;
/// Faces run back to their start this much more slowly when no other face is wanted
constexpr float k_FaceRelaxSlowing = 4.0f;
/// A face pulled with no time given is held for an hour
constexpr float k_FaceHeldUnlessTold = 3'600'000.0f;
/// The head settles on its target, and looks only sideways when its target is this far round
constexpr float k_Settled = 1e-4f;

constexpr std::array<std::string_view, animations::k_FaceCount> k_FaceNames {
    "smile", "grimace", "growl", "scared", "sad", "amazed", "puzzled", "laugh", "ooh", "aah", "spare1", "spare2",
};
constexpr std::array<std::string_view, animations::k_ActionCount> k_ActionNames {
    "summon",      "angry",      "hungry", "happy",    "sad",           "tired",       "hot",        "cold",
    "scratch",     "frightened", "sneeze", "confused", "feeling_nice",  "impress",     "need_a_poo", "feel_playful",
    "play_action", "look_at_me", "taunt",  "drink",    "friendly_wave", "embarrassed", "pick_me",
};
constexpr std::array<std::string_view, animations::k_GestureCount> k_GestureNames {
    "nod", "shake", "yawn", "thirsty", "squirt_water", "talk", "spare1",
};
} // namespace

std::string_view animations::Name(size_t animation)
{
	if (animation == k_Stand)
	{
		return "stand";
	}
	if (animation >= k_FirstFace && animation < k_FirstFace + k_FaceCount)
	{
		return k_FaceNames.at(animation - k_FirstFace);
	}
	if (animation >= k_FirstAction && animation < k_FirstAction + k_ActionCount)
	{
		return k_ActionNames.at(animation - k_FirstAction);
	}
	if (animation >= k_FirstGesture && animation < k_FirstGesture + k_GestureCount)
	{
		return k_GestureNames.at(animation - k_FirstGesture);
	}
	switch (animation)
	{
	case k_StartSleep:
		return "start_sleep";
	case k_Sleep:
		return "sleep";
	case k_EndSleep:
		return "end_sleep";
	case k_StartPoo:
		return "start_poo";
	case k_Poo:
		return "poo";
	case k_EndPoo:
		return "end_poo";
	case k_StartPuke:
		return "start_puke";
	case k_Puke:
		return "puke";
	case k_EndPuke:
		return "end_puke";
	case k_Eat:
		return "eat";
	case k_Faint:
		return "faint";
	case k_GetUp:
		return "get_up";
	case k_StartSit:
		return "start_sit";
	case k_Sit:
		return "sit";
	case k_EndSit:
		return "end_sit";
	case k_LookRightLeft:
		return "right_left";
	case k_LookDownUp:
		return "down_up";
	case k_SitLookRightLeft:
		return "sit_r_l";
	case k_SitLookDownUp:
		return "sit_d_u";
	default:
		return {};
	}
}

float creature_layers::PlaybackRate(float size)
{
	return k_PlaybackRateAtSizeZero - (k_PlaybackSlowingPerSize * size);
}

bool creature_layers::IsPlaying(const BodyAction& body)
{
	return body.kind != BodyAction::Kind::Stand;
}

bool creature_layers::IsLooping(const BodyAction& body)
{
	return body.kind == BodyAction::Kind::Sequence && body.phase == BodyAction::Phase::Loop;
}

size_t creature_layers::CurrentAnimation(const BodyAction& body)
{
	switch (body.kind)
	{
	case BodyAction::Kind::Once:
		return body.animations[0];
	case BodyAction::Kind::Sequence:
		return body.animations.at(static_cast<size_t>(body.phase));
	case BodyAction::Kind::Stand:
	default:
		return animations::k_Stand;
	}
}

std::optional<BodyAction> creature_layers::PlayOnce(const BodyAction& body, size_t animation, bool mirrored)
{
	if (IsPlaying(body))
	{
		return std::nullopt;
	}
	return BodyAction {.kind = BodyAction::Kind::Once,
	                   .phase = BodyAction::Phase::Start,
	                   .animations = {animation, animation, animation},
	                   .timeMs = 0.0f,
	                   .mirrored = mirrored,
	                   .endWanted = false};
}

std::optional<BodyAction> creature_layers::PlaySequence(const BodyAction& body, size_t start, size_t loop, size_t end,
                                                        bool holdLoop)
{
	if (IsPlaying(body))
	{
		return std::nullopt;
	}
	return BodyAction {.kind = BodyAction::Kind::Sequence,
	                   .phase = BodyAction::Phase::Start,
	                   .animations = {start, loop, end},
	                   .timeMs = 0.0f,
	                   .mirrored = false,
	                   .endWanted = false,
	                   .holdLoop = holdLoop};
}

BodyAction creature_layers::EndLoop(BodyAction body)
{
	if (body.kind == BodyAction::Kind::Sequence)
	{
		body.endWanted = true;
	}
	return body;
}

BodyAction creature_layers::AdvanceBody(BodyAction body, float milliseconds, std::optional<uint32_t> duration)
{
	if (!IsPlaying(body))
	{
		body.timeMs = 0.0f;
		return body;
	}
	if (body.timedByPlayer)
	{
		return body;
	}
	const auto length = static_cast<float>(duration.value_or(0));
	const auto stand = [] { return BodyAction {}; };
	if (body.kind == BodyAction::Kind::Once)
	{
		body.timeMs += milliseconds;
		return body.timeMs >= length ? stand() : body;
	}

	switch (body.phase)
	{
	case BodyAction::Phase::Start:
		body.timeMs += milliseconds;
		if (body.timeMs >= length)
		{
			body.phase = BodyAction::Phase::Loop;
			body.timeMs = 0.0f;
		}
		break;
	case BodyAction::Phase::Loop:
		if (body.endWanted || (length <= 0.0f && !body.holdLoop))
		{
			body.phase = BodyAction::Phase::End;
			body.timeMs = 0.0f;
			break;
		}
		body.timeMs = body.holdLoop ? std::max(length - 1.0f, 0.0f) : std::fmod(body.timeMs + milliseconds, length);
		break;
	case BodyAction::Phase::End:
		body.timeMs += milliseconds;
		if (body.timeMs >= length)
		{
			return stand();
		}
		break;
	}
	return body;
}

FaceLayer creature_layers::PullFace(FaceLayer face, size_t animation, float milliseconds, creature_face::Cue cue)
{
	face.wanted = animation;
	face.remainingMs = milliseconds > 0.0f ? milliseconds : k_FaceHeldUnlessTold;
	face.cue = cue;
	return face;
}

FaceLayer creature_layers::RelaxFace(FaceLayer face)
{
	face.wanted.reset();
	face.remainingMs = 0.0f;
	face.cue = creature_face::Cue::None;
	return face;
}

FaceLayer creature_layers::AdvanceFace(FaceLayer face, float milliseconds, std::optional<uint32_t> duration)
{
	// The wanted face is let go of once its time is up
	face.remainingMs = std::max(face.remainingMs - milliseconds, 0.0f);
	if (face.remainingMs <= 0.0f)
	{
		face = RelaxFace(face);
	}

	if (face.current == face.wanted)
	{
		// The expression plays through once and holds its last frame
		if (face.current.has_value() && duration.has_value())
		{
			face.timeMs = std::min(face.timeMs + milliseconds, static_cast<float>(*duration));
		}
		return face;
	}
	if (face.current.has_value())
	{
		face.timeMs -= face.wanted.has_value() ? milliseconds : milliseconds / k_FaceRelaxSlowing;
		if (face.timeMs <= 0.0f)
		{
			face.timeMs = 0.0f;
			face.current.reset();
		}
	}
	if (!face.current.has_value())
	{
		face.current = face.wanted;
		face.timeMs = 0.0f;
	}
	return face;
}

std::optional<GestureLayer> creature_layers::PlayGesture(const GestureLayer& gesture, size_t animation)
{
	if (gesture.animation.has_value())
	{
		return std::nullopt;
	}
	return GestureLayer {.animation = animation, .timeMs = 0.0f};
}

GestureLayer creature_layers::AdvanceGesture(GestureLayer gesture, float milliseconds, std::optional<uint32_t> duration)
{
	if (!gesture.animation.has_value())
	{
		return gesture;
	}
	gesture.timeMs += milliseconds;
	if (!duration.has_value() || gesture.timeMs >= static_cast<float>(*duration))
	{
		return {};
	}
	return gesture;
}

LookAxis creature_layers::TurnHead(LookAxis axis, float target, float acceleration, float seconds, float limit)
{
	const auto error = std::abs(target - axis.angle);
	if (error < k_Settled)
	{
		return {.angle = target, .velocity = 0.0f};
	}
	if (seconds <= 0.0f)
	{
		return axis;
	}
	// The fastest it may go: able to stop in time at this acceleration, and not past the target in this step
	const auto maxSpeed = std::min(std::sqrt(2.0f * error * acceleration), error / seconds);
	if (axis.angle < target)
	{
		axis.velocity += acceleration * seconds;
	}
	else
	{
		axis.velocity -= acceleration * seconds;
	}
	axis.velocity = std::clamp(axis.velocity, -maxSpeed, maxSpeed);
	axis.angle = std::clamp(axis.angle + (axis.velocity * seconds), -limit, limit);
	return axis;
}

LookAngles creature_layers::AnglesTowards(const glm::vec3& head, const glm::vec3& ahead, const glm::vec3& target)
{
	const auto offset = target - head;
	const auto length = glm::length(offset);
	if (length <= 0.0f)
	{
		return {.yaw = 0.0f, .pitch = 0.0f};
	}
	const auto direction = offset / length;
	const auto pitch = std::asin(std::clamp(direction.y, -1.0f, 1.0f));
	const glm::vec2 flat {direction.x, direction.z};
	const glm::vec2 forward {ahead.x, ahead.z};
	if (glm::length(flat) <= k_Settled || glm::length(forward) <= 0.0f)
	{
		return {.yaw = 0.0f, .pitch = pitch};
	}
	// The world is left-handed: seen from above with y up, x to the right, z points away, so a target clockwise from
	// ahead is on the creature's right and one anticlockwise on its left
	const auto cross = (forward.x * flat.y) - (forward.y * flat.x);
	const auto dot = glm::dot(forward, flat);
	return {.yaw = std::atan2(cross, dot), .pitch = pitch};
}

uint32_t creature_layers::LookTime(float angle, float limit, uint32_t duration)
{
	if (limit <= 0.0f)
	{
		return duration / 2;
	}
	const auto fraction = std::clamp((angle + limit) / (2.0f * limit), 0.0f, 1.0f);
	return static_cast<uint32_t>(fraction * static_cast<float>(duration));
}
