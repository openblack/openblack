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
#include <chrono>
#include <optional>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace openblack
{
namespace morph
{
class MorphFile;
} // namespace morph

/// Poses the god hand the way Black & White's CHand does it.
///
/// The hand animations live in Data/CTR/hh.hbn, named by Data/hndspec5.txt. Each hand state plays a cycle (a C
/// animation) and leans the hand with the two pose ranges that follow it in the spec (L animations, _lr and _fb).
/// The lean comes from how far a spring-smoothed copy of the cursor trails the real one, so the hand sways as the
/// mouse moves and settles when it stops. Changing pose cross-fades from the last one.
class HandAnimation
{
public:
	/// Cycles of hndspec5.txt, each followed by its _lr and _fb pose ranges
	enum class Cycle : uint8_t
	{
		Wiggle = 0,
		Point = 3,
		HoldAbove = 6,
		HoldSide = 9,
		CanPickUp = 12,
		HoldFingers = 15,
		Stroke = 24,
		Tickle = 27,
		Slap = 33,
		Grip = 42,
		Rotate = 45,
		Pitch = 48,
		Zoom = 51,
		TapHouse = 57,
		Horn = 60,
		Phile = 63,
	};

	/// The hand states that drive the cycles: HandStateNormal while hovering and HandStateCamera while dragging the
	/// camera. They lean the hand opposite ways sideways.
	enum class State : uint8_t
	{
		Normal,
		Camera,
	};

	/// A keyframe pose: per joint YXZ Euler angles and translations of the joints an animation moves
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

	/// LHMatrix: a row-major rotation and a translation, used with row vectors
	struct Pose
	{
		std::array<std::array<float, 3>, 3> rotation;
		glm::vec3 translation;
	};

	/// boneParents and restMatrices are those of the hand mesh, Hand_Boned_Base2.l3d
	bool Load(const morph::MorphFile& file, const std::vector<uint32_t>& boneParents,
	          const std::vector<glm::mat4>& restMatrices);
	[[nodiscard]] bool IsLoaded() const { return !_boneParents.empty(); }

	/// Advances the animation by a frame. cursor is the mouse position in window pixels.
	void Update(std::chrono::microseconds dt, State state, Cycle cycle, glm::ivec2 cursor);

	/// Model space matrices of the hand's bones, in the form L3DMesh::GetBoneMatrices gives the rest pose
	[[nodiscard]] const std::vector<glm::mat4>& GetBoneMatrices() const { return _boneMatrices; }
	[[nodiscard]] const Animation* GetAnimation(size_t specIndex) const;

	/// CHand::SetDistanceFromView and SetSize: the hand is scaled so its rest pose bones span 3.2 units from top
	/// to bottom, growing past 150 units from the camera so that it stays about as large on screen
	[[nodiscard]] static float SizeAtDistance(float distanceFromCamera);
	/// The scale of the hand mesh for a distance from the camera
	[[nodiscard]] float ScaleAtDistance(float distanceFromCamera) const;

	// Debug introspection
	[[nodiscard]] State GetState() const { return _state; }
	[[nodiscard]] Cycle GetCycle() const { return _cycle; }
	[[nodiscard]] glm::vec2 GetLean() const { return _lean; }

	/// The local poses of the bones for a cycle at a time and leans, without cross-fading. lean is the clamped
	/// cursor lag in pixels, positive to the right and down.
	[[nodiscard]] std::vector<Pose> EvaluatePoses(Cycle cycle, uint32_t timeMs, std::optional<glm::vec2> lean) const;

private:
	void ApplyCycle(const Animation& animation, uint32_t timeMs, std::vector<Pose>& poses) const;
	void ApplyLean(const Animation& animation, uint32_t timeMs, std::vector<Pose>& poses) const;
	void ComposeBoneMatrices(const std::vector<Pose>& poses);

	std::vector<std::optional<Animation>> _animations;
	std::vector<uint32_t> _boneParents;
	/// Rest pose rotations of the bones in model space and their inverses
	std::vector<std::array<std::array<float, 3>, 3>> _restRotations;
	std::vector<std::array<std::array<float, 3>, 3>> _inverseRestRotations;
	std::vector<glm::mat4> _boneMatrices;
	/// Height of the rest pose's bones in mesh units (LH3DAnim::SetTransform's return value)
	float _restHeight {1.0f};

	State _state {State::Normal};
	Cycle _cycle {Cycle::Wiggle};
	/// Each state keeps its own cycle time, the camera's restarts whenever it is entered
	std::array<std::chrono::microseconds, 2> _cycleTimes {};

	bool _springStarted {false};
	glm::vec2 _smoothedCursor {0.0f, 0.0f};
	glm::vec2 _smoothedVelocity {0.0f, 0.0f};
	glm::vec2 _lean {0.0f, 0.0f};

	/// The pose being faded from after the cycle or state changed
	std::vector<Pose> _fadeFrom;
	std::vector<Pose> _lastPoses;
	std::optional<float> _fadeTime;
};

} // namespace openblack
