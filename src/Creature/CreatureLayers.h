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
#include <numbers>
#include <optional>
#include <string_view>

#include <glm/vec3.hpp>

/// What a creature's body plays, layer by layer. The body plays one animation at a time: standing and breathing, an
/// action that plays once (a yawn of tiredness, a wave), or a start, loop and end (sitting down, sitting, getting up).
/// On top of it, the head turns towards what the creature looks at, the face pulls an expression, and a gesture such as
/// a nod can play once. Animations are known by their place in the creature spec file.
namespace openblack::creature_layers
{
/// The places of the animations the creatures' minds play in the creature spec file (Data/ctrspec27.txt, its lines
/// counted from the first animation after the version, the section lines starting with '=' skipped)
namespace animations
{
constexpr size_t k_Stand = 0;
/// The faces: smile, grimace, growl, scared, sad, amazed, puzzled, laugh, ooh, aah, and two spare
constexpr size_t k_FirstFace = 16;
constexpr size_t k_FaceCount = 12;
/// The ten faces the idle creature pulls at random
constexpr size_t k_IdleFaceCount = 10;
constexpr size_t k_StartSit = 37;
constexpr size_t k_Sit = 38;
constexpr size_t k_EndSit = 39;
/// The actions that play once, from summoning to "pick me"
constexpr size_t k_FirstAction = 52;
constexpr size_t k_ActionCount = 23;
constexpr size_t k_Summon = 52;
constexpr size_t k_Angry = 53;
constexpr size_t k_Hungry = 54;
constexpr size_t k_Happy = 55;
constexpr size_t k_Sad = 56;
constexpr size_t k_Tired = 57;
constexpr size_t k_Hot = 58;
constexpr size_t k_Cold = 59;
constexpr size_t k_Scratch = 60;
constexpr size_t k_Frightened = 61;
constexpr size_t k_Confused = 63;
constexpr size_t k_FeelingNice = 64;
constexpr size_t k_Impress = 65;
constexpr size_t k_NeedAPoo = 66;
constexpr size_t k_FeelPlayful = 67;
constexpr size_t k_Taunt = 70;
constexpr size_t k_FriendlyWave = 72;
/// Turning the head right to left and looking down to up, standing and sitting. The middle keyframe looks ahead.
constexpr size_t k_LookRightLeft = 75;
constexpr size_t k_LookDownUp = 76;
constexpr size_t k_SitLookRightLeft = 77;
constexpr size_t k_SitLookDownUp = 78;
/// The gestures played on top of the body: nod, shake, yawn, thirsty, squirt water, talk
constexpr size_t k_FirstGesture = 200;
constexpr size_t k_GestureCount = 7;
constexpr size_t k_Yawn = 202;

/// The spec file's name of an animation the minds play, without its type letter, or empty for others
[[nodiscard]] std::string_view Name(size_t animation);
} // namespace animations

/// How fast every animation but the stand plays: bigger creatures move more slowly
[[nodiscard]] float PlaybackRate(float size);

/// What the body plays
struct BodyAction
{
	enum class Kind : uint8_t
	{
		/// Standing, breathing
		Stand,
		/// An action played once, then back to standing
		Once,
		/// A start, a loop for as long as wanted, and an end, then back to standing
		Sequence,
	};
	enum class Phase : uint8_t
	{
		Start,
		Loop,
		End,
	};
	Kind kind {Kind::Stand};
	Phase phase {Phase::Start};
	/// The action played once, or the start, loop and end
	std::array<size_t, 3> animations {};
	float timeMs {0.0f};
	/// Played left to right
	bool mirrored {false};
	/// A sequence ends once its loop is told to
	bool endWanted {false};
};

[[nodiscard]] bool IsPlaying(const BodyAction& body);
/// Whether the body is in the loop of a sequence, such as sitting
[[nodiscard]] bool IsLooping(const BodyAction& body);
/// The animation the body plays now
[[nodiscard]] size_t CurrentAnimation(const BodyAction& body);
/// An action that plays once, unless the body already plays one: nothing starts while one plays
[[nodiscard]] std::optional<BodyAction> PlayOnce(const BodyAction& body, size_t animation, bool mirrored);
/// A start, loop and end, unless the body already plays an action
[[nodiscard]] std::optional<BodyAction> PlaySequence(const BodyAction& body, size_t start, size_t loop, size_t end);
/// A sequence's loop ends and its end plays
[[nodiscard]] BodyAction EndLoop(BodyAction body);
/// The body some milliseconds on, given how long the animation it plays now lasts, or nothing when the species has no
/// such animation. Each animation starts from its beginning, without blending into the last.
[[nodiscard]] BodyAction AdvanceBody(BodyAction body, float milliseconds, std::optional<uint32_t> duration);

/// The face's expression, played on top of the body and looping. Changing it runs the expression back to its start
/// first, a quarter as fast when no other is wanted, then plays the new one.
struct FaceLayer
{
	std::optional<size_t> current;
	float timeMs {0.0f};
	std::optional<size_t> wanted;
};
[[nodiscard]] FaceLayer AdvanceFace(FaceLayer face, float milliseconds, std::optional<uint32_t> duration);

/// A gesture played once on top of the body, such as a nod or a yawn
struct GestureLayer
{
	std::optional<size_t> animation;
	float timeMs {0.0f};
};
/// A gesture, unless one already plays
[[nodiscard]] std::optional<GestureLayer> PlayGesture(const GestureLayer& gesture, size_t animation);
[[nodiscard]] GestureLayer AdvanceGesture(GestureLayer gesture, float milliseconds, std::optional<uint32_t> duration);

/// How far the head is turned on one axis, and how fast it is turning
struct LookAxis
{
	float angle {0.0f};
	float velocity {0.0f};
};
/// The head turns right and left as far as half a turn each way, and down and up a quarter turn each way
constexpr float k_YawLimit = std::numbers::pi_v<float>;
constexpr float k_PitchLimit = std::numbers::pi_v<float> / 2.0f;
/// The head speeds up towards where it should turn at a steady rate, and slows so as to stop there: never faster than
/// would let it stop in time, nor than would overshoot in this step. It settles once within a hair of the target, and
/// never turns past the limit either way.
[[nodiscard]] LookAxis TurnHead(LookAxis axis, float target, float acceleration, float seconds, float limit);

/// Which way to turn the head to look at a point, in radians: yaw to the left of ahead, pitch up
struct LookAngles
{
	float yaw;
	float pitch;
};
[[nodiscard]] LookAngles AnglesTowards(const glm::vec3& head, const glm::vec3& ahead, const glm::vec3& target);
/// The time in a head turning animation that shows an angle: the middle looks ahead, the ends as far as the limit
[[nodiscard]] uint32_t LookTime(float angle, float limit, uint32_t duration);
} // namespace openblack::creature_layers
