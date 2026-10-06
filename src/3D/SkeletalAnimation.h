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
#include <span>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace openblack::morph
{
struct Animation;
} // namespace openblack::morph

/// The keyframe animations of the boned meshes the hand and the creatures are posed with, and the maths Black & White
/// poses them by: keyframes of YXZ Euler angles and translations per bone, blended between keyframes on the rotation
/// matrices' elements.
namespace openblack::skeletal_animation
{
/// The game's 3 by 3 matrix, row-major and used with row vectors
using Matrix = std::array<std::array<float, 3>, 3>;

constexpr Matrix k_Identity {{{1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}}};
constexpr uint32_t k_NoParent = 0xFFFFFFFF;

/// A keyframe animation: per bone YXZ Euler angles and translations of the bones it moves
struct Animation
{
	uint32_t duration;
	bool looping;
	std::vector<uint32_t> rotatedJoints;
	std::vector<uint32_t> translatedJoints;
	struct Frame
	{
		std::vector<glm::vec3> eulerAngles;
		std::vector<glm::vec3> translations;
	};
	std::vector<Frame> frames;
};

/// A bone's pose relative to its parent: a row-major rotation and a translation, used with row vectors
struct Pose
{
	Matrix rotation;
	glm::vec3 translation;
};

/// The rest pose the keyframes are relative to: each bone's parent, and its rest rotation in the mesh's space and the
/// inverse of it
struct Skeleton
{
	std::vector<uint32_t> parents;
	std::vector<Matrix> restRotations;
	std::vector<Matrix> inverseRestRotations;

	/// From the rest pose's matrices in the mesh's space, in the form L3DMesh::GetBoneMatrices gives them
	[[nodiscard]] static Skeleton FromRestMatrices(std::span<const uint32_t> parents, std::span<const glm::mat4> rest);
	[[nodiscard]] bool Empty() const { return parents.empty(); }
};

/// A rotation matrix from y, x and z angles, combined in that order as the game does. Keyframe angles are stored x, y, z.
[[nodiscard]] Matrix RotationYXZ(const glm::vec3& euler);
/// The x, y and z angles of a rotation matrix made by RotationYXZ
[[nodiscard]] glm::vec3 EulerYXZ(const Matrix& m);
/// Row vector product a * b
[[nodiscard]] Matrix Multiply(const Matrix& a, const Matrix& b);
[[nodiscard]] Matrix Transpose(const Matrix& m);
[[nodiscard]] Matrix Inverse(const Matrix& m);
/// a moved towards b by t, element by element
[[nodiscard]] Matrix Lerp(const Matrix& a, const Matrix& b, float t);
/// Each row made unit length again, as keyframe rotations are after being blended
[[nodiscard]] Matrix NormaliseRows(Matrix m);

/// The two keyframes around a time and how far between them it is. Cycles wrap from their last frame to the first,
/// pose ranges end on their last frame.
struct FrameSpan
{
	size_t from;
	size_t to;
	float t;
};
[[nodiscard]] FrameSpan FindFrames(const Animation& animation, uint32_t timeMs);

/// The bones' poses at a time of a cycle: the bones it moves follow its keyframes, all others take the first frame of
/// the stand animation. Keyframe rotations are in the rest pose's space; each bone's own is relative to its parent's
/// rest rotation.
///
/// Given a mirror (see MirrorJoints), the animation plays left to right: each bone's keyframes move its mirror bone
/// instead, their y and z angles negated, and their movement away from the stand's first frame flipped across the body.
[[nodiscard]] std::vector<Pose> SampleCycle(const Animation& animation, const Animation& stand, uint32_t timeMs,
                                            const Skeleton& skeleton, std::span<const uint32_t> mirror = {});

/// Poses summed by weight, rotation matrices and translations element by element, as animations playing together
/// (walking into running, say) are. The weights are expected to add up to 1.
[[nodiscard]] std::vector<Pose> WeightedSum(std::span<const std::vector<Pose>> poses, std::span<const float> weights);

/// Plays an animation on top of poses: each bone it moves is turned and moved by how far its keyframes at the time are
/// from a reference keyframe of the same animation. Turning the head to look about, the face's expressions and
/// gestures such as nodding are layered this way. A mirror flips the layer as SampleCycle does.
void AddLayer(std::vector<Pose>& poses, const Animation& layer, uint32_t timeMs, size_t referenceFrame,
              const Skeleton& skeleton, std::span<const uint32_t> mirror = {});

/// Each bone's mirror bone across the body's x = 0 plane in the rest pose: a left leg's right leg, a bone along the
/// middle itself. Bones are paired when each is where the other would be reflected; any without a clear partner keep
/// their own place.
[[nodiscard]] std::vector<uint32_t> MirrorJoints(std::span<const glm::mat4> rest);

/// The matrices in the mesh's space of the bones posed, in the form L3DMesh::GetBoneMatrices gives the rest pose
[[nodiscard]] std::vector<glm::mat4> ComposeBoneMatrices(std::span<const Pose> poses, std::span<const uint32_t> parents);

/// An animation of a .cbn or .hbn file
[[nodiscard]] Animation FromMorph(const morph::Animation& animation);
} // namespace openblack::skeletal_animation
