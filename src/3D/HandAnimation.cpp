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

namespace openblack
{

namespace
{
using skeletal_animation::FindFrames;
using skeletal_animation::k_Identity;
using skeletal_animation::k_NoParent;
using skeletal_animation::Lerp;
using skeletal_animation::Multiply;
using skeletal_animation::NormaliseRows;
using skeletal_animation::RotationYXZ;
using skeletal_animation::Transpose;

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
		const auto& animation = _animations[i].emplace(skeletal_animation::FromMorph(*source));
		if (animation.frames.empty())
		{
			_animations[i].reset();
		}
	}
	if (!_animations[static_cast<size_t>(Cycle::Wiggle)])
	{
		return false;
	}

	_skeleton = skeletal_animation::Skeleton::FromRestMatrices(boneParents, restMatrices);
	float lowest = restMatrices.front()[3].y;
	float highest = lowest;
	for (const auto& matrix : restMatrices)
	{
		lowest = std::min(lowest, matrix[3].y);
		highest = std::max(highest, matrix[3].y);
	}
	_restHeight = std::max(highest - lowest, 0.001f);
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
	poses = skeletal_animation::SampleCycle(animation, *_animations[static_cast<size_t>(Cycle::Wiggle)], timeMs, _skeleton);
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
		const auto parent = _skeleton.parents[joint];
		const auto rotation = NormaliseRows(
		    Lerp(RotationYXZ(animation.frames[from].eulerAngles[i]), RotationYXZ(animation.frames[to].eulerAngles[i]), t));
		const auto inverseReference = Transpose(RotationYXZ(reference.eulerAngles[i]));

		auto& pose = poses[joint];
		const bool hasParent = joint != 0 && parent != k_NoParent;
		if (hasParent)
		{
			pose.rotation = Multiply(pose.rotation, _skeleton.restRotations[parent]);
		}
		pose.rotation = Multiply(Multiply(pose.rotation, inverseReference), rotation);
		if (hasParent)
		{
			pose.rotation = Multiply(pose.rotation, _skeleton.inverseRestRotations[parent]);
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
	std::vector<Pose> poses(_skeleton.parents.size(), Pose {.rotation = k_Identity, .translation = glm::vec3(0.0f)});
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
	_boneMatrices = skeletal_animation::ComposeBoneMatrices(poses, _skeleton.parents);
}

void HandAnimation::StepCursorSpring(float seconds, glm::vec2 mouse)
{
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
	_cursorLag =
	    glm::clamp(glm::vec2(_smoothedCursor.x - mouse.x, mouse.y - _smoothedCursor.y), -k_MaxCursorLag, k_MaxCursorLag);
}

void HandAnimation::StartFade()
{
	_fadeFrom = _lastPoses;
	_fadeTime = 0.0f;
}

void HandAnimation::ApplyFade(float seconds, std::vector<Pose>& poses)
{
	if (!_fadeTime)
	{
		return;
	}
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

glm::vec3 HandAnimation::LeafBoneCentre() const
{
	std::vector<bool> hasChild(_skeleton.parents.size(), false);
	for (const auto parent : _skeleton.parents)
	{
		if (parent != k_NoParent && parent < hasChild.size())
		{
			hasChild[parent] = true;
		}
	}
	glm::vec3 sum(0.0f);
	size_t count = 0;
	for (size_t bone = 0; bone < _boneMatrices.size() && bone < hasChild.size(); ++bone)
	{
		if (!hasChild[bone])
		{
			sum += glm::vec3(_boneMatrices[bone][3]);
			++count;
		}
	}
	return count > 0 ? sum / static_cast<float>(count) : glm::vec3(0.0f);
}

void HandAnimation::UpdateHeld(std::chrono::microseconds dt, Cycle cycle, uint32_t timeMs, glm::ivec2 cursor)
{
	if (!IsLoaded())
	{
		return;
	}
	const auto seconds = std::chrono::duration<float>(dt).count();
	StepCursorSpring(seconds, glm::vec2(cursor));
	_lean = glm::vec2(0.0f);
	if (!_holding)
	{
		StartFade();
		_holding = true;
	}
	_cycle = cycle;
	auto poses = EvaluatePoses(cycle, timeMs, std::nullopt);
	ApplyFade(seconds, poses);
	_lastPoses = poses;
	ComposeBoneMatrices(poses);
}

void HandAnimation::Update(std::chrono::microseconds dt, State state, Cycle cycle, glm::ivec2 cursor)
{
	if (!IsLoaded())
	{
		return;
	}
	const auto seconds = std::chrono::duration<float>(dt).count();
	const auto mouse = glm::vec2(cursor);
	StepCursorSpring(seconds, mouse);
	// Hovering, the hand leans towards where the cursor came from. Dragging the camera, it leans the other way
	// sideways as if pulling the land along.
	const auto sideways = state == State::Normal ? _smoothedCursor.x - mouse.x : mouse.x - _smoothedCursor.x;
	_lean = glm::clamp(glm::vec2(sideways, mouse.y - _smoothedCursor.y), -k_MaxCursorLag, k_MaxCursorLag);

	auto& time = _cycleTimes.at(static_cast<size_t>(state));
	// Letting go of a seed fades back from the holding pose
	if (_holding)
	{
		_holding = false;
		StartFade();
	}
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
	ApplyFade(seconds, poses);

	_lastPoses = poses;
	ComposeBoneMatrices(poses);
}

} // namespace openblack
