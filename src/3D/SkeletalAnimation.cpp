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
#include <optional>

#include <MorphFile.h>
#include <glm/geometric.hpp>
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

namespace
{
/// Where each bone is in a list of the bones an animation moves, or nothing for those it doesn't
std::vector<std::optional<size_t>> JointSlots(std::span<const uint32_t> joints, size_t count)
{
	std::vector<std::optional<size_t>> slots(count);
	for (size_t i = 0; i < joints.size(); ++i)
	{
		if (joints[i] < count)
		{
			slots[joints[i]] = i;
		}
	}
	return slots;
}

/// Mirrored keyframe angles: y and z negated
glm::vec3 MirrorEuler(glm::vec3 euler)
{
	euler.y = -euler.y;
	euler.z = -euler.z;
	return euler;
}

/// A movement flipped across the body: the root moves sideways along x, every other bone along its own z
glm::vec3 MirrorMove(glm::vec3 move, uint32_t joint)
{
	if (joint == 0)
	{
		move.x = -move.x;
	}
	else
	{
		move.z = -move.z;
	}
	return move;
}

uint32_t Destination(std::span<const uint32_t> mirror, uint32_t joint)
{
	return joint < mirror.size() ? mirror[joint] : joint;
}

/// A keyframe rotation between two frames, blended on the matrices' elements and made unit again
Matrix BlendKeyframes(const glm::vec3& from, const glm::vec3& to, float t, bool mirrored)
{
	return NormaliseRows(
	    Lerp(RotationYXZ(mirrored ? MirrorEuler(from) : from), RotationYXZ(mirrored ? MirrorEuler(to) : to), t));
}
} // namespace

std::vector<Pose> SampleCycle(const Animation& animation, const Animation& stand, uint32_t timeMs, const Skeleton& skeleton,
                              std::span<const uint32_t> mirror)
{
	const auto count = skeleton.parents.size();
	std::vector<Pose> poses(count, Pose {.rotation = k_Identity, .translation = glm::vec3(0.0f)});
	if (animation.frames.empty() || count == 0)
	{
		return poses;
	}
	const bool mirrored = !mirror.empty();
	const auto* standFrame = stand.frames.empty() ? nullptr : &stand.frames.front();
	const auto standRotated = JointSlots(stand.rotatedJoints, count);
	const auto standTranslated = JointSlots(stand.translatedJoints, count);
	const auto standTranslation = [&](uint32_t joint) -> std::optional<glm::vec3> {
		if (standFrame == nullptr || !standTranslated[joint])
		{
			return std::nullopt;
		}
		return standFrame->translations[*standTranslated[joint]];
	};
	const auto rotated = JointSlots(animation.rotatedJoints, count);
	const auto translated = JointSlots(animation.translatedJoints, count);
	const auto [from, to, t] = FindFrames(animation, timeMs);

	for (uint32_t joint = 0; joint < count; ++joint)
	{
		// The bone the keyframes of this one move: itself, or its mirror bone
		const auto destination = std::min(Destination(mirror, joint), static_cast<uint32_t>(count - 1));
		auto& pose = poses[destination];
		const auto parent = skeleton.parents[destination];

		Matrix rotation = k_Identity;
		if (const auto slot = rotated[joint])
		{
			rotation =
			    BlendKeyframes(animation.frames[from].eulerAngles[*slot], animation.frames[to].eulerAngles[*slot], t, mirrored);
		}
		else if (standFrame != nullptr && standRotated[destination])
		{
			rotation = RotationYXZ(standFrame->eulerAngles[*standRotated[destination]]);
		}
		pose.rotation = Multiply(skeleton.restRotations[destination], rotation);
		if (destination != 0 && parent != k_NoParent)
		{
			pose.rotation = Multiply(pose.rotation, skeleton.inverseRestRotations[parent]);
		}

		if (const auto slot = translated[joint])
		{
			const auto& a = animation.frames[from].translations[*slot];
			const auto& b = animation.frames[to].translations[*slot];
			const auto translation = (b - a) * t + a;
			if (mirrored)
			{
				// The movement away from the stand, flipped, from where the stand has the mirror bone
				const auto move = translation - standTranslation(joint).value_or(glm::vec3(0.0f));
				pose.translation = standTranslation(destination).value_or(glm::vec3(0.0f)) + MirrorMove(move, joint);
			}
			else
			{
				pose.translation = translation;
			}
		}
		else if (const auto standMove = standTranslation(destination))
		{
			pose.translation = *standMove;
		}
	}
	return poses;
}

std::vector<Pose> WeightedSum(std::span<const std::vector<Pose>> poses, std::span<const float> weights)
{
	if (poses.empty())
	{
		return {};
	}
	std::vector<Pose> sum(poses.front().size(), Pose {.rotation = {}, .translation = glm::vec3(0.0f)});
	for (size_t i = 0; i < poses.size() && i < weights.size(); ++i)
	{
		const auto weight = weights[i];
		for (size_t joint = 0; joint < sum.size() && joint < poses[i].size(); ++joint)
		{
			for (size_t r = 0; r < 3; ++r)
			{
				for (size_t c = 0; c < 3; ++c)
				{
					sum[joint].rotation.at(r).at(c) += weight * poses[i][joint].rotation.at(r).at(c);
				}
			}
			sum[joint].translation += weight * poses[i][joint].translation;
		}
	}
	return sum;
}

void AddLayer(std::vector<Pose>& poses, const Animation& layer, uint32_t timeMs, size_t referenceFrame,
              const Skeleton& skeleton, std::span<const uint32_t> mirror)
{
	if (layer.frames.empty() || poses.empty())
	{
		return;
	}
	const bool mirrored = !mirror.empty();
	const auto count = std::min(poses.size(), skeleton.parents.size());
	const auto& reference = layer.frames[std::min(referenceFrame, layer.frames.size() - 1)];
	const auto [from, to, t] = FindFrames(layer, timeMs);

	for (size_t slot = 0; slot < layer.rotatedJoints.size(); ++slot)
	{
		const auto joint = layer.rotatedJoints[slot];
		const auto destination = Destination(mirror, joint);
		if (joint >= count || destination >= count)
		{
			continue;
		}
		const auto keyframe =
		    BlendKeyframes(layer.frames[from].eulerAngles[slot], layer.frames[to].eulerAngles[slot], t, mirrored);
		const auto referenceEuler = mirrored ? MirrorEuler(reference.eulerAngles[slot]) : reference.eulerAngles[slot];
		// A bone's keyframe rotation sits just before its parent's inverse rest rotation; the layer's turn away from its
		// reference goes in straight after the keyframe
		const auto parent = skeleton.parents[destination];
		const bool hasParent = destination != 0 && parent != k_NoParent && parent < count;
		auto rotation = poses[destination].rotation;
		if (hasParent)
		{
			rotation = Multiply(rotation, skeleton.restRotations[parent]);
		}
		rotation = Multiply(Multiply(rotation, Transpose(RotationYXZ(referenceEuler))), keyframe);
		if (hasParent)
		{
			rotation = Multiply(rotation, skeleton.inverseRestRotations[parent]);
		}
		poses[destination].rotation = rotation;
	}

	for (size_t slot = 0; slot < layer.translatedJoints.size(); ++slot)
	{
		const auto joint = layer.translatedJoints[slot];
		const auto destination = Destination(mirror, joint);
		if (joint >= count || destination >= count)
		{
			continue;
		}
		const auto& a = layer.frames[from].translations[slot];
		const auto& b = layer.frames[to].translations[slot];
		const auto move = ((b - a) * t) + a - reference.translations[slot];
		poses[destination].translation += mirrored ? MirrorMove(move, joint) : move;
	}
}

std::vector<uint32_t> MirrorJoints(std::span<const glm::mat4> rest)
{
	// Bones closer than this share of the skeleton's reach count as in the same place
	constexpr float k_Tolerance = 0.03f;
	const auto count = rest.size();
	std::vector<uint32_t> mirror(count);
	std::vector<glm::vec3> positions(count);
	float reach = 0.0f;
	for (size_t i = 0; i < count; ++i)
	{
		mirror[i] = static_cast<uint32_t>(i);
		positions[i] = glm::vec3(rest[i][3]);
		reach = std::max(reach, glm::length(positions[i]));
	}
	const auto tolerance = std::max(reach * k_Tolerance, 1e-6f);
	for (size_t i = 0; i < count; ++i)
	{
		if (std::abs(positions[i].x) <= tolerance)
		{
			continue;
		}
		const glm::vec3 reflected {-positions[i].x, positions[i].y, positions[i].z};
		std::optional<size_t> best;
		float bestDistance = tolerance;
		for (size_t j = 0; j < count; ++j)
		{
			const auto distance = glm::length(positions[j] - reflected);
			if (j != i && distance <= bestDistance)
			{
				best = j;
				bestDistance = distance;
			}
		}
		if (best)
		{
			mirror[i] = static_cast<uint32_t>(*best);
		}
	}
	// Only pairs that agree on each other
	for (size_t i = 0; i < count; ++i)
	{
		if (mirror[mirror[i]] != i)
		{
			mirror[i] = static_cast<uint32_t>(i);
		}
	}
	return mirror;
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
	    .displacement = {source.header.displacement[0], source.header.displacement[1], source.header.displacement[2]},
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
