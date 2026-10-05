/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "HandAnimation.h"

#include <cmath>

#include <algorithm>
#include <limits>

#include <MorphFile.h>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace openblack
{

namespace
{
using Matrix = std::array<std::array<float, 3>, 3>;

/// How far the smoothed cursor may trail the real one, in pixels. It is also the lag that leans the hand fully.
constexpr float k_MaxCursorLag = 80.0f;
/// Spring pulling the smoothed cursor towards the real one, per second
constexpr float k_CursorStiffness = 20.0f;
/// Fraction of the smoothed cursor's velocity left after a second
constexpr float k_CursorDamping = 0.03f;
/// Smaller leans are not applied
constexpr float k_MinLean = 0.0001f;
/// Seconds spent cross-fading from the previous pose when the cycle or the state changes
constexpr float k_FadeDuration = 0.13f;
/// Height in world units of the hand's rest pose bones at its standard size
constexpr float k_StandardHeight = 3.2f;
/// Distances from the camera: the hand shrinks closer than k_ShrinkDistance and grows further than
/// k_GrowDistance, up to k_MaxDistance
constexpr float k_MinDistance = 2.0f;
constexpr float k_ShrinkDistance = 10.0f;
constexpr float k_GrowDistance = 150.0f;
constexpr float k_MaxDistance = 1800.0f;
constexpr uint32_t k_NoParent = std::numeric_limits<uint32_t>::max();

constexpr Matrix k_Identity {{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};

/// A rotation matrix from y, x and z angles, combined in that order as the game does
Matrix RotationYXZ(float y, float x, float z)
{
	const auto cy = std::cos(y);
	const auto sy = std::sin(y);
	const auto cx = std::cos(x);
	const auto sx = std::sin(x);
	const auto cz = std::cos(z);
	const auto sz = std::sin(z);
	return {{
	    {(cy * cz) - (sy * sx * sz), -sz * cx, (sy * cz) + (cy * sx * sz)},
	    {(cy * sz) + (sy * sx * cz), cx * cz, (sy * sz) - (cy * sx * cz)},
	    {-sy * cx, sx, cy * cx},
	}};
}

/// The keyframe angles are stored x, y, z
Matrix RotationYXZ(const glm::vec3& euler)
{
	return RotationYXZ(euler.y, euler.x, euler.z);
}

/// Row vector product a * b
Matrix Multiply(const Matrix& a, const Matrix& b)
{
	Matrix result {};
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			result.at(r).at(c) = (a.at(r)[0] * b.at(0).at(c)) + (a.at(r)[1] * b.at(1).at(c)) + (a.at(r)[2] * b.at(2).at(c));
		}
	}
	return result;
}

Matrix Transpose(const Matrix& m)
{
	Matrix result {};
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			result.at(r).at(c) = m.at(c).at(r);
		}
	}
	return result;
}

Matrix Lerp(const Matrix& a, const Matrix& b, float t)
{
	Matrix result {};
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			result.at(r).at(c) = ((b.at(r).at(c) - a.at(r).at(c)) * t) + a.at(r).at(c);
		}
	}
	return result;
}

/// Keyframe rotations are blended element-wise, then each row is made unit length again
Matrix NormaliseRows(Matrix m)
{
	for (auto& row : m)
	{
		const auto length = std::sqrt((row[0] * row[0]) + (row[1] * row[1]) + (row[2] * row[2]));
		if (length > 0.0f)
		{
			for (auto& value : row)
			{
				value /= length;
			}
		}
	}
	return m;
}

/// The two keyframes around a time and how far between them it is. Cycles wrap from their last frame to the
/// first, pose ranges end on their last frame.
struct FrameSpan
{
	size_t from;
	size_t to;
	float t;
};

FrameSpan FindFrames(const HandAnimation::Animation& animation, uint32_t timeMs)
{
	const auto count = static_cast<int32_t>(animation.frames.size());
	auto duration = static_cast<int32_t>(animation.duration);
	if (!animation.looping && count > 1)
	{
		duration = (duration * count) / (count - 1);
	}
	duration = std::max(duration, 1);
	const auto time = static_cast<int32_t>(timeMs);
	const auto frame = std::clamp((count * time) / duration, 0, count - 1);
	const auto next = frame + 1 == count ? 0 : frame + 1;
	const auto t =
	    ((static_cast<float>(count) / static_cast<float>(duration)) * static_cast<float>(time)) - static_cast<float>(frame);
	return {.from = static_cast<size_t>(frame), .to = static_cast<size_t>(next), .t = t};
}

/// The time of a pose range that leans fully one way at -k_MaxCursorLag and the other at k_MaxCursorLag
uint32_t LeanTime(float lag, uint32_t duration)
{
	return static_cast<uint32_t>((lag + k_MaxCursorLag) / (k_MaxCursorLag * 2.0f) * static_cast<float>(duration));
}
} // namespace

bool HandAnimation::Load(const morph::MorphFile& file, const std::vector<uint32_t>& boneParents,
                         const std::vector<glm::mat4>& restMatrices)
{
	size_t animationCount = 0;
	for (const auto& set : file.GetAnimationSpecs().animationSets)
	{
		animationCount += set.animations.size();
	}
	if (boneParents.empty() || boneParents.size() != restMatrices.size() || file.GetBaseAnimation(0) == nullptr)
	{
		return false;
	}

	_animations.assign(animationCount, std::nullopt);
	for (size_t i = 0; i < animationCount; ++i)
	{
		const auto* source = file.GetBaseAnimation(i);
		if (source == nullptr)
		{
			continue;
		}
		auto& animation = _animations[i].emplace();
		animation.duration = source->header.duration;
		animation.looping = (source->header.looping & 1u) != 0;
		animation.rotatedJoints = source->rotatedJointIndices;
		animation.translatedJoints = source->translatedJointIndices;
		for (const auto& keyframe : source->keyframes)
		{
			auto& frame = animation.frames.emplace_back();
			for (const auto& angles : keyframe.eulerAngles)
			{
				frame.eulerAngles.emplace_back(angles[0], angles[1], angles[2]);
			}
			for (const auto& translation : keyframe.translations)
			{
				frame.translations.emplace_back(translation[0], translation[1], translation[2]);
			}
		}
		if (animation.frames.empty())
		{
			_animations[i].reset();
		}
	}
	if (!_animations[static_cast<size_t>(Cycle::Wiggle)])
	{
		return false;
	}

	_boneParents = boneParents;
	float lowest = restMatrices.front()[3].y;
	float highest = lowest;
	for (const auto& matrix : restMatrices)
	{
		lowest = std::min(lowest, matrix[3].y);
		highest = std::max(highest, matrix[3].y);
	}
	_restHeight = std::max(highest - lowest, 0.001f);
	_restRotations.resize(restMatrices.size());
	_inverseRestRotations.resize(restMatrices.size());
	for (size_t i = 0; i < restMatrices.size(); ++i)
	{
		// The matrices are the transposes of the game's: column r holds row r
		Matrix rotation {};
		for (size_t r = 0; r < 3; ++r)
		{
			for (size_t c = 0; c < 3; ++c)
			{
				rotation.at(r).at(c) = restMatrices[i][static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)];
			}
		}
		_restRotations[i] = rotation;

		const auto inverse = glm::inverse(glm::mat3(restMatrices[i]));
		for (size_t r = 0; r < 3; ++r)
		{
			for (size_t c = 0; c < 3; ++c)
			{
				_inverseRestRotations.at(i).at(r).at(c) = inverse[static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)];
			}
		}
	}

	_cycleTimes = {};
	_springStarted = false;
	_fadeTime.reset();
	_lastPoses = EvaluatePoses(Cycle::Wiggle, 0, std::nullopt);
	ComposeBoneMatrices(_lastPoses);
	return true;
}

float HandAnimation::SizeAtDistance(float distanceFromCamera)
{
	const auto distance = std::clamp(distanceFromCamera, k_MinDistance, k_MaxDistance);
	auto size = 1.0f;
	if (distance < k_ShrinkDistance)
	{
		size = std::pow(distance / k_ShrinkDistance, 0.8f);
	}
	if (distance > k_GrowDistance)
	{
		size *= (distance / k_GrowDistance) * (1.0f - ((distance - k_GrowDistance) / (k_MaxDistance - k_GrowDistance)) * 0.3f);
	}
	return size;
}

float HandAnimation::ScaleAtDistance(float distanceFromCamera) const
{
	return k_StandardHeight * SizeAtDistance(distanceFromCamera) / _restHeight;
}

const HandAnimation::Animation* HandAnimation::GetAnimation(size_t specIndex) const
{
	if (specIndex >= _animations.size() || !_animations[specIndex])
	{
		return nullptr;
	}
	return &*_animations[specIndex];
}

// The C animation of a state: the joints it moves follow its keyframes, all others take the stand pose, the first
// frame of the wiggle cycle.
void HandAnimation::ApplyCycle(const Animation& animation, uint32_t timeMs, std::vector<Pose>& poses) const
{
	const auto& stand = *_animations[static_cast<size_t>(Cycle::Wiggle)];
	const auto& standFrame = stand.frames.front();
	const auto [from, to, t] = FindFrames(animation, timeMs);

	size_t rotated = 0;
	size_t translated = 0;
	for (uint32_t joint = 0; joint < poses.size(); ++joint)
	{
		auto& pose = poses[joint];
		const auto parent = _boneParents[joint];

		// Keyframe rotations are in the rest pose's model space, the joint's own rotation is relative to its
		// parent's rest rotation
		Matrix rotation = k_Identity;
		if (rotated < animation.rotatedJoints.size() && animation.rotatedJoints[rotated] == joint)
		{
			rotation = NormaliseRows(Lerp(RotationYXZ(animation.frames[from].eulerAngles[rotated]),
			                              RotationYXZ(animation.frames[to].eulerAngles[rotated]), t));
			++rotated;
		}
		else
		{
			const auto iter = std::ranges::find(stand.rotatedJoints, joint);
			if (iter != stand.rotatedJoints.end())
			{
				rotation = RotationYXZ(standFrame.eulerAngles[static_cast<size_t>(iter - stand.rotatedJoints.begin())]);
			}
		}
		pose.rotation = Multiply(_restRotations[joint], rotation);
		if (joint != 0 && parent != k_NoParent)
		{
			pose.rotation = Multiply(pose.rotation, _inverseRestRotations[parent]);
		}

		if (translated < animation.translatedJoints.size() && animation.translatedJoints[translated] == joint)
		{
			const auto& a = animation.frames[from].translations[translated];
			const auto& b = animation.frames[to].translations[translated];
			pose.translation = (b - a) * t + a;
			++translated;
		}
		else
		{
			const auto iter = std::ranges::find(stand.translatedJoints, joint);
			if (iter != stand.translatedJoints.end())
			{
				pose.translation = standFrame.translations[static_cast<size_t>(iter - stand.translatedJoints.begin())];
			}
		}
	}
}

// An L animation adds how far its pose at a time differs from its middle frame
void HandAnimation::ApplyLean(const Animation& animation, uint32_t timeMs, std::vector<Pose>& poses) const
{
	const auto [from, to, t] = FindFrames(animation, timeMs);
	const auto& reference = animation.frames[(animation.frames.size() - 1) / 2];

	for (size_t i = 0; i < animation.rotatedJoints.size(); ++i)
	{
		const auto joint = animation.rotatedJoints[i];
		if (joint >= poses.size())
		{
			continue;
		}
		const auto parent = _boneParents[joint];
		const auto rotation = NormaliseRows(
		    Lerp(RotationYXZ(animation.frames[from].eulerAngles[i]), RotationYXZ(animation.frames[to].eulerAngles[i]), t));
		const auto inverseReference = Transpose(RotationYXZ(reference.eulerAngles[i]));

		auto& pose = poses[joint];
		const bool hasParent = joint != 0 && parent != k_NoParent;
		if (hasParent)
		{
			pose.rotation = Multiply(pose.rotation, _restRotations[parent]);
		}
		pose.rotation = Multiply(Multiply(pose.rotation, inverseReference), rotation);
		if (hasParent)
		{
			pose.rotation = Multiply(pose.rotation, _inverseRestRotations[parent]);
		}
	}

	for (size_t i = 0; i < animation.translatedJoints.size(); ++i)
	{
		const auto joint = animation.translatedJoints[i];
		if (joint >= poses.size())
		{
			continue;
		}
		const auto& a = animation.frames[from].translations[i];
		const auto& b = animation.frames[to].translations[i];
		poses[joint].translation += (b - a) * t + a - reference.translations[i];
	}
}

std::vector<HandAnimation::Pose> HandAnimation::EvaluatePoses(Cycle cycle, uint32_t timeMs, std::optional<glm::vec2> lean) const
{
	std::vector<Pose> poses(_boneParents.size(), Pose {.rotation = k_Identity, .translation = glm::vec3(0.0f)});
	const auto* animation = GetAnimation(static_cast<size_t>(cycle));
	if (animation == nullptr)
	{
		animation = GetAnimation(static_cast<size_t>(Cycle::Wiggle));
		timeMs = 0;
	}
	ApplyCycle(*animation, timeMs, poses);

	if (lean)
	{
		const auto* sideways = GetAnimation(static_cast<size_t>(cycle) + 1);
		if (sideways != nullptr && std::abs(lean->x) > k_MinLean)
		{
			ApplyLean(*sideways, LeanTime(lean->x, sideways->duration), poses);
		}
		// The game checks the smoothed cursor rather than the lag here, which is practically never zero, so the
		// forward and backward lean is always applied
		const auto* forward = GetAnimation(static_cast<size_t>(cycle) + 2);
		if (forward != nullptr)
		{
			ApplyLean(*forward, LeanTime(lean->y, forward->duration), poses);
		}
	}
	return poses;
}

void HandAnimation::ComposeBoneMatrices(const std::vector<Pose>& poses)
{
	_boneMatrices.resize(poses.size());
	for (size_t i = 0; i < poses.size(); ++i)
	{
		glm::mat4 local(1.0f);
		for (size_t r = 0; r < 3; ++r)
		{
			for (size_t c = 0; c < 3; ++c)
			{
				local[static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)] = poses.at(i).rotation.at(r).at(c);
			}
		}
		local[3] = glm::vec4(poses[i].translation, 1.0f);
		const auto parent = _boneParents[i];
		_boneMatrices[i] = parent != k_NoParent && parent < i ? _boneMatrices[parent] * local : local;
	}
}

void HandAnimation::Update(std::chrono::microseconds dt, State state, Cycle cycle, glm::ivec2 cursor)
{
	if (!IsLoaded())
	{
		return;
	}
	const auto seconds = std::chrono::duration<float>(dt).count();
	const auto mouse = glm::vec2(cursor);

	// A spring-damped copy of the cursor that is never more than k_MaxCursorLag pixels behind it
	if (!_springStarted)
	{
		_smoothedCursor = mouse;
		_smoothedVelocity = glm::vec2(0.0f);
		_springStarted = true;
	}
	else
	{
		for (glm::length_t axis = 0; axis < 2; ++axis)
		{
			_smoothedCursor[axis] = std::clamp(_smoothedCursor[axis] + (seconds * _smoothedVelocity[axis]),
			                                   mouse[axis] - k_MaxCursorLag, mouse[axis] + k_MaxCursorLag);
			_smoothedVelocity[axis] += (mouse[axis] - _smoothedCursor[axis]) * k_CursorStiffness * seconds;
			_smoothedVelocity[axis] *= std::pow(k_CursorDamping, seconds);
		}
	}
	// Hovering, the hand leans towards where the cursor came from. Dragging the camera, it leans the other way
	// sideways as if pulling the land along.
	const auto sideways = state == State::Normal ? _smoothedCursor.x - mouse.x : mouse.x - _smoothedCursor.x;
	_lean = glm::clamp(glm::vec2(sideways, mouse.y - _smoothedCursor.y), -k_MaxCursorLag, k_MaxCursorLag);

	auto& time = _cycleTimes.at(static_cast<size_t>(state));
	if (state != _state || cycle != _cycle)
	{
		if (state != _state && state == State::Camera)
		{
			// Starting to drag the camera starts its cycle over
			time = std::chrono::microseconds::zero();
		}
		if (state == State::Normal && cycle == Cycle::HoldFingers)
		{
			time = std::chrono::microseconds::zero();
		}
		_fadeFrom = _lastPoses;
		_fadeTime = 0.0f;
		_state = state;
		_cycle = cycle;
	}

	const auto* animation = GetAnimation(static_cast<size_t>(cycle));
	const auto duration = std::chrono::milliseconds(animation != nullptr ? std::max(animation->duration, 1u) : 1u);
	time = (time + dt) % std::chrono::duration_cast<std::chrono::microseconds>(duration);
	const auto timeMs = static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(time).count());

	auto poses = EvaluatePoses(cycle, timeMs, _lean);

	if (_fadeTime)
	{
		*_fadeTime += seconds;
		if (*_fadeTime < k_FadeDuration && _fadeFrom.size() == poses.size())
		{
			const auto t = *_fadeTime / k_FadeDuration;
			for (size_t i = 0; i < poses.size(); ++i)
			{
				poses[i].rotation = Lerp(_fadeFrom[i].rotation, poses[i].rotation, t);
				poses[i].translation = (poses[i].translation - _fadeFrom[i].translation) * t + _fadeFrom[i].translation;
			}
		}
		else
		{
			_fadeTime.reset();
		}
	}

	_lastPoses = poses;
	ComposeBoneMatrices(poses);
}

} // namespace openblack
