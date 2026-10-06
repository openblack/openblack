/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureAnimation.h"

#include <cmath>

#include <algorithm>
#include <optional>

using namespace openblack;
using namespace openblack::skeletal_animation;

namespace
{
/// The time constant breathing settles back to its resting period with, and the one it changes to any other with
constexpr float k_BreathRestSeconds = 10.0f;
constexpr float k_BreathChangeSeconds = 0.5f;
/// Breathing is five seconds at size 1, by the square root of the size
constexpr float k_BreathSecondsAtSizeOne = 5.0f;

/// Where each bone is in a list of the bones an animation moves, or nothing for those it doesn't
std::vector<std::optional<size_t>> JointSlots(const std::vector<uint32_t>& joints)
{
	std::vector<std::optional<size_t>> slots;
	for (size_t i = 0; i < joints.size(); ++i)
	{
		const auto joint = static_cast<size_t>(joints[i]);
		if (joint >= slots.size())
		{
			slots.resize(joint + 1);
		}
		slots[joint] = i;
	}
	return slots;
}

std::optional<size_t> SlotOf(const std::vector<std::optional<size_t>>& slots, uint32_t joint)
{
	return joint < slots.size() ? slots[joint] : std::nullopt;
}

/// The rotation and translation a stand gives a bone in its first frame, when it moves that bone
struct StandPose
{
	std::vector<std::optional<size_t>> rotated;
	std::vector<std::optional<size_t>> translated;
	const Animation* stand;

	explicit StandPose(const Animation* animation)
	    : stand(animation)
	{
		if (stand != nullptr)
		{
			rotated = JointSlots(stand->rotatedJoints);
			translated = JointSlots(stand->translatedJoints);
		}
	}

	[[nodiscard]] std::optional<glm::vec3> Euler(uint32_t joint) const
	{
		const auto slot = SlotOf(rotated, joint);
		if (!slot || stand->frames.empty())
		{
			return std::nullopt;
		}
		return stand->frames.front().eulerAngles[*slot];
	}

	[[nodiscard]] std::optional<glm::vec3> Translation(uint32_t joint) const
	{
		const auto slot = SlotOf(translated, joint);
		if (!slot || stand->frames.empty())
		{
			return std::nullopt;
		}
		return stand->frames.front().translations[*slot];
	}
};

/// One side of a blend, ready to look its bones up
struct Side
{
	const Animation* animation;
	StandPose stand;
	float weight;
	std::vector<std::optional<size_t>> rotated;
	std::vector<std::optional<size_t>> translated;

	explicit Side(const creature_animation::BlendTerm& term)
	    : animation(term.animation != nullptr && !term.animation->frames.empty() ? term.animation : nullptr)
	    , stand(term.stand)
	    , weight(term.weight)
	{
		if (animation != nullptr)
		{
			rotated = JointSlots(animation->rotatedJoints);
			translated = JointSlots(animation->translatedJoints);
		}
	}

	[[nodiscard]] size_t Frame(size_t frame) const { return std::min(frame, animation->frames.size() - 1); }

	[[nodiscard]] Matrix Rotation(uint32_t joint, size_t frame, const Matrix& base) const
	{
		if (const auto slot = animation != nullptr ? SlotOf(rotated, joint) : std::nullopt)
		{
			return RotationYXZ(animation->frames[Frame(frame)].eulerAngles[*slot]);
		}
		if (const auto euler = stand.Euler(joint))
		{
			return RotationYXZ(*euler);
		}
		return base;
	}

	[[nodiscard]] glm::vec3 Translation(uint32_t joint, size_t frame, const glm::vec3& base) const
	{
		if (const auto slot = animation != nullptr ? SlotOf(translated, joint) : std::nullopt)
		{
			return animation->frames[Frame(frame)].translations[*slot];
		}
		return stand.Translation(joint).value_or(base);
	}
};
} // namespace

Animation creature_animation::AdjustFromStand(const Animation& animation, const Animation& baseStand,
                                              const Animation& variantStand)
{
	const StandPose base(&baseStand);
	const StandPose variant(&variantStand);
	auto adjusted = animation;
	for (auto& frame : adjusted.frames)
	{
		for (size_t i = 0; i < adjusted.rotatedJoints.size(); ++i)
		{
			const auto joint = adjusted.rotatedJoints[i];
			const auto from = base.Euler(joint);
			const auto to = variant.Euler(joint);
			const auto inverseFrom = from ? Transpose(RotationYXZ(*from)) : k_Identity;
			const auto toRotation = to ? RotationYXZ(*to) : k_Identity;
			frame.eulerAngles[i] = EulerYXZ(Multiply(Multiply(RotationYXZ(frame.eulerAngles[i]), inverseFrom), toRotation));
		}
		for (size_t i = 0; i < adjusted.translatedJoints.size(); ++i)
		{
			const auto joint = adjusted.translatedJoints[i];
			frame.translations[i] +=
			    variant.Translation(joint).value_or(glm::vec3(0.0f)) - base.Translation(joint).value_or(glm::vec3(0.0f));
		}
	}
	return adjusted;
}

Animation creature_animation::Blend(const Animation& base, const BlendTerm& evilGood, const BlendTerm& thinFat)
{
	const Side a(evilGood);
	const Side b(thinFat);
	auto blended = base;
	if (a.weight == 0.0f && b.weight == 0.0f)
	{
		return blended;
	}
	for (size_t f = 0; f < blended.frames.size(); ++f)
	{
		auto& frame = blended.frames[f];
		for (size_t i = 0; i < blended.rotatedJoints.size(); ++i)
		{
			const auto joint = blended.rotatedJoints[i];
			const auto rotation = RotationYXZ(frame.eulerAngles[i]);
			const auto towardsA = a.Rotation(joint, f, rotation);
			const auto towardsB = b.Rotation(joint, f, rotation);
			Matrix result {};
			for (size_t r = 0; r < 3; ++r)
			{
				for (size_t c = 0; c < 3; ++c)
				{
					const auto value = rotation.at(r).at(c);
					result.at(r).at(c) =
					    value + (a.weight * (towardsA.at(r).at(c) - value)) + (b.weight * (towardsB.at(r).at(c) - value));
				}
			}
			frame.eulerAngles[i] = EulerYXZ(result);
		}
		for (size_t i = 0; i < blended.translatedJoints.size(); ++i)
		{
			const auto joint = blended.translatedJoints[i];
			const auto translation = frame.translations[i];
			frame.translations[i] = translation + (a.weight * (a.Translation(joint, f, translation) - translation)) +
			                        (b.weight * (b.Translation(joint, f, translation) - translation));
		}
	}
	return blended;
}

std::vector<glm::mat4> creature_animation::BlendRest(std::span<const glm::mat4> base, std::span<const glm::mat4> evilGood,
                                                     float evilGoodWeight, std::span<const glm::mat4> thinFat,
                                                     float thinFatWeight)
{
	std::vector<glm::mat4> blended(base.begin(), base.end());
	for (size_t i = 0; i < blended.size(); ++i)
	{
		const auto towardsA = i < evilGood.size() ? evilGood[i] : base[i];
		const auto towardsB = i < thinFat.size() ? thinFat[i] : base[i];
		// The rotation and the translation, as the game's twelve numbers
		for (glm::length_t c = 0; c < 4; ++c)
		{
			for (glm::length_t r = 0; r < 3; ++r)
			{
				const auto value = base[i][c][r];
				blended[i][c][r] =
				    value + (evilGoodWeight * (towardsA[c][r] - value)) + (thinFatWeight * (towardsB[c][r] - value));
			}
		}
	}
	return blended;
}

float creature_animation::BreathPeriod(float size)
{
	return k_BreathSecondsAtSizeOne * std::sqrt(std::max(size, 0.0f));
}

float creature_animation::AdvanceBreath(float phase, float seconds, float period)
{
	if (period <= 0.0f)
	{
		return phase;
	}
	const auto next = phase + (seconds / period);
	return next - std::floor(next);
}

float creature_animation::EaseBreathPeriod(float current, float target, float restingPeriod, float turnSeconds)
{
	if (current <= 0.0f)
	{
		return target;
	}
	// Exactly the resting period, as the creature stopping sets it, calms slowly; anything else is taken up quickly
	const auto seconds = target == restingPeriod ? k_BreathRestSeconds : k_BreathChangeSeconds;
	// A whole number of turns, truncated
	const auto turns = turnSeconds > 0.0f ? static_cast<int32_t>(seconds / turnSeconds) : 1;
	return current + ((target - current) / static_cast<float>(std::max(turns, 1)));
}

uint32_t creature_animation::BreathTime(float phase, uint32_t duration)
{
	if (duration == 0)
	{
		return 0;
	}
	const auto time = static_cast<uint32_t>(std::max(phase, 0.0f) * static_cast<float>(duration));
	return std::min(time, duration - 1);
}
