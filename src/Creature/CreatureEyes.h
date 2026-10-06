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

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

/// A creature's eyes: an eyeball and an eyelid for each, set into the body at points on its triangles. The eyes look
/// where the creature looks, or ahead, but never back into the head; the lids blink every few seconds and, while the
/// creature is calm, follow the pupils up and down.
namespace openblack::creature_eyes
{
/// How the eyes look
enum class Mode : uint8_t
{
	/// Wide open, as when frightened
	Wide,
	Closed,
	/// Open, blinking, the lids following the pupils
	Calm,
	/// Wide open, darting about
	Stoned,
	/// As calm, but always looking ahead
	Ahead,
};

/// The angles, in units of pi, an eyelid is turned to: wide open, closed, and the angle it follows the pupil from
struct LidAngles
{
	float open;
	float closed;
	float calm;
};

/// The eyes blink by closing for k_BlinkMs and opening for as long again, then stay open for a while
constexpr int32_t k_BlinkMs = 200;
constexpr int32_t k_FirstBlinkMs = 1000;
constexpr int32_t k_BlinkIntervalMs = 5000;

struct Blink
{
	enum class State : uint8_t
	{
		Open,
		Closing,
		Opening,
	};
	State state {State::Open};
	/// Milliseconds until the next change
	int32_t timerMs {k_FirstBlinkMs};
	/// The eyes stay open for between half and one and a half times this between blinks
	int32_t intervalMs {k_BlinkIntervalMs};
};

/// The blink some milliseconds on. random(n) is a number from 0 to n - 1.
template <typename Random>
[[nodiscard]] Blink AdvanceBlink(Blink blink, int32_t deltaMs, Random&& random)
{
	blink.timerMs -= deltaMs;
	if (blink.timerMs >= 1)
	{
		return blink;
	}
	switch (blink.state)
	{
	case Blink::State::Open:
		blink.state = Blink::State::Closing;
		blink.timerMs = k_BlinkMs;
		break;
	case Blink::State::Closing:
		blink.state = Blink::State::Opening;
		blink.timerMs = k_BlinkMs;
		break;
	case Blink::State::Opening:
		blink.state = Blink::State::Open;
		blink.timerMs = static_cast<int32_t>(random(static_cast<uint32_t>(blink.intervalMs))) + (blink.intervalMs / 2);
		break;
	}
	return blink;
}

/// A point on a triangle and the triangle's normal, unnormalised: from the first vertex u of the way to the second and
/// v of the way to the third. The normal points into the body.
struct SurfacePoint
{
	glm::vec3 position;
	glm::vec3 normal;
};
[[nodiscard]] SurfacePoint PointOnTriangle(const glm::vec3& first, const glm::vec3& second, const glm::vec3& third, float u,
                                           float v);

/// How big an eye is drawn for a creature of a size, its species' eye size and how evil (-1) or good (1) its body is.
/// Smaller creatures have bigger eyes for their size.
[[nodiscard]] float EyeSize(float size, float speciesEyeScale, float evilGood, Mode mode);

/// The seconds the eyes take to turn to where they look
[[nodiscard]] float LookSeconds(float openness, Mode mode);

/// Which way the eye's back faces, the pupil facing the other way, kept from pointing too far out of the head: it is
/// pulled along the surface's inward normal until it is at least a little along it
[[nodiscard]] glm::vec3 ClampLook(const glm::vec3& away, const glm::vec3& inwardNormal);

/// An eye's axes, each a row of the game's matrix, and where it is
struct Frame
{
	glm::vec3 x;
	glm::vec3 y;
	glm::vec3 z;
	glm::vec3 origin;
};

/// The eyeball, centred where it sits and facing away from where it looks
[[nodiscard]] Frame EyeballFrame(const glm::vec3& centre, const glm::vec3& away);
/// The eyelid, centred on the eyeball, its back to the surface, turned towards the point the lid is anchored at
[[nodiscard]] Frame EyelidFrame(const glm::vec3& centre, const glm::vec3& inwardNormal, const glm::vec3& anchor, bool rightEye);
/// The eyelid's angle, in radians, for how the eyes look. A calm lid follows the pupil up and down, and swings shut and
/// open again over a blink. lidPitch is the sine of how far the pupil looks up out of the lid's plane.
[[nodiscard]] float EyelidAngle(Mode mode, const LidAngles& angles, float openness, float lidPitch, const Blink& blink);
/// The sine of how far the pupil looks out of the lid's plane, for EyelidAngle
[[nodiscard]] float LidPitch(const Frame& lid, const Frame& eyeball);
/// A frame turned about its x axis by an angle in radians
[[nodiscard]] Frame Turned(const Frame& frame, float angle);
/// The matrix that draws a mesh in a frame at a size
[[nodiscard]] glm::mat4 ToMatrix(const Frame& frame, float size);
} // namespace openblack::creature_eyes
