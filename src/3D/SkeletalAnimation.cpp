/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "SkeletalAnimation.h"

#include <cmath>

#include <algorithm>

#include <MorphFile.h>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

namespace openblack::skeletal_animation
{

Skeleton Skeleton::FromRestMatrices(std::span<const uint32_t> parents, std::span<const glm::mat4> rest)
{
	Skeleton skeleton;
	skeleton.parents.assign(parents.begin(), parents.end());
	skeleton.restRotations.resize(rest.size());
	skeleton.inverseRestRotations.resize(rest.size());
	for (size_t i = 0; i < rest.size(); ++i)
	{
		// The matrices are the transposes of the game's: column r holds row r
		Matrix rotation {};
		for (size_t r = 0; r < 3; ++r)
		{
			for (size_t c = 0; c < 3; ++c)
			{
				rotation.at(r).at(c) = rest[i][static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)];
			}
		}
		skeleton.restRotations[i] = rotation;
		skeleton.inverseRestRotations[i] = Inverse(rotation);
	}
	return skeleton;
}

Matrix RotationYXZ(const glm::vec3& euler)
{
	const auto cy = std::cos(euler.y);
	const auto sy = std::sin(euler.y);
	const auto cx = std::cos(euler.x);
	const auto sx = std::sin(euler.x);
	const auto cz = std::cos(euler.z);
	const auto sz = std::sin(euler.z);
	return {{
	    {(cy * cz) - (sy * sx * sz), -sz * cx, (sy * cz) + (cy * sx * sz)},
	    {(cy * sz) + (sy * sx * cz), cx * cz, (sy * sz) - (cy * sx * cz)},
	    {-sy * cx, sx, cy * cx},
	}};
}

glm::vec3 EulerYXZ(const Matrix& m)
{
	// The third row is (-sin y cos x, sin x, cos y cos x) and the second column (-sin z cos x, cos x cos z, ...)
	const auto x = std::asin(std::clamp(m[2][1], -1.0f, 1.0f));
	const auto y = std::atan2(-m[2][0], m[2][2]);
	const auto z = std::atan2(-m[0][1], m[1][1]);
	return {x, y, z};
}

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

Matrix Inverse(const Matrix& m)
{
	glm::mat3 matrix;
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			matrix[static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)] = m.at(r).at(c);
		}
	}
	const auto inverse = glm::inverse(matrix);
	Matrix result {};
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			result.at(r).at(c) = inverse[static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)];
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

FrameSpan FindFrames(const Animation& animation, uint32_t timeMs)
{
	const auto count = static_cast<int32_t>(animation.frames.size());
	auto duration = static_cast<int32_t>(animation.duration);
	if (!animation.looping && count > 1)
	{
		duration = (duration * count) / (count - 1);
	}
	duration = std::max(duration, 1);
	const auto time = static_cast<int32_t>(timeMs);
	const auto frame = std::clamp((count * time) / duration, 0, std::max(count - 1, 0));
	const auto next = frame + 1 >= count ? 0 : frame + 1;
	const auto t =
	    ((static_cast<float>(count) / static_cast<float>(duration)) * static_cast<float>(time)) - static_cast<float>(frame);
	return {.from = static_cast<size_t>(frame), .to = static_cast<size_t>(next), .t = t};
}

std::vector<Pose> SampleCycle(const Animation& animation, const Animation& stand, uint32_t timeMs, const Skeleton& skeleton)
{
	std::vector<Pose> poses(skeleton.parents.size(), Pose {.rotation = k_Identity, .translation = glm::vec3(0.0f)});
	if (animation.frames.empty())
	{
		return poses;
	}
	const auto* standFrame = stand.frames.empty() ? nullptr : &stand.frames.front();
	const auto [from, to, t] = FindFrames(animation, timeMs);

	size_t rotated = 0;
	size_t translated = 0;
	for (uint32_t joint = 0; joint < poses.size(); ++joint)
	{
		auto& pose = poses[joint];
		const auto parent = skeleton.parents[joint];

		Matrix rotation = k_Identity;
		if (rotated < animation.rotatedJoints.size() && animation.rotatedJoints[rotated] == joint)
		{
			rotation = NormaliseRows(Lerp(RotationYXZ(animation.frames[from].eulerAngles[rotated]),
			                              RotationYXZ(animation.frames[to].eulerAngles[rotated]), t));
			++rotated;
		}
		else if (standFrame != nullptr)
		{
			const auto iter = std::ranges::find(stand.rotatedJoints, joint);
			if (iter != stand.rotatedJoints.end())
			{
				rotation = RotationYXZ(standFrame->eulerAngles[static_cast<size_t>(iter - stand.rotatedJoints.begin())]);
			}
		}
		pose.rotation = Multiply(skeleton.restRotations[joint], rotation);
		if (joint != 0 && parent != k_NoParent)
		{
			pose.rotation = Multiply(pose.rotation, skeleton.inverseRestRotations[parent]);
		}

		if (translated < animation.translatedJoints.size() && animation.translatedJoints[translated] == joint)
		{
			const auto& a = animation.frames[from].translations[translated];
			const auto& b = animation.frames[to].translations[translated];
			pose.translation = (b - a) * t + a;
			++translated;
		}
		else if (standFrame != nullptr)
		{
			const auto iter = std::ranges::find(stand.translatedJoints, joint);
			if (iter != stand.translatedJoints.end())
			{
				pose.translation = standFrame->translations[static_cast<size_t>(iter - stand.translatedJoints.begin())];
			}
		}
	}
	return poses;
}

std::vector<glm::mat4> ComposeBoneMatrices(std::span<const Pose> poses, std::span<const uint32_t> parents)
{
	std::vector<glm::mat4> matrices(poses.size());
	for (size_t i = 0; i < poses.size(); ++i)
	{
		glm::mat4 local(1.0f);
		for (size_t r = 0; r < 3; ++r)
		{
			for (size_t c = 0; c < 3; ++c)
			{
				local[static_cast<glm::length_t>(r)][static_cast<glm::length_t>(c)] = poses[i].rotation.at(r).at(c);
			}
		}
		local[3] = glm::vec4(poses[i].translation, 1.0f);
		const auto parent = i < parents.size() ? parents[i] : k_NoParent;
		matrices[i] = parent != k_NoParent && parent < i ? matrices[parent] * local : local;
	}
	return matrices;
}

Animation FromMorph(const morph::Animation& source)
{
	Animation animation {
	    .duration = source.header.duration,
	    .looping = (source.header.looping & 1u) != 0,
	    .rotatedJoints = source.rotatedJointIndices,
	    .translatedJoints = source.translatedJointIndices,
	    .frames = {},
	};
	animation.frames.reserve(source.keyframes.size());
	for (const auto& keyframe : source.keyframes)
	{
		auto& frame = animation.frames.emplace_back();
		frame.eulerAngles.reserve(keyframe.eulerAngles.size());
		for (const auto& angles : keyframe.eulerAngles)
		{
			frame.eulerAngles.emplace_back(angles[0], angles[1], angles[2]);
		}
		frame.translations.reserve(keyframe.translations.size());
		for (const auto& translation : keyframe.translations)
		{
			frame.translations.emplace_back(translation[0], translation[1], translation[2]);
		}
	}
	return animation;
}

} // namespace openblack::skeletal_animation
