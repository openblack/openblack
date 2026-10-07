/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

// Poses the hand with the animations of a Black & White installation. The game's data is not distributed, so this
// only runs when OPENBLACK_GAME_PATH points at an installation.

#include <cmath>
#include <cstdlib>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <limits>
#include <vector>

#include <L3DFile.h>
#include <MorphFile.h>
#include <PackFile.h>
#include <glm/gtc/matrix_access.hpp>
#include <gtest/gtest.h>

#include "3D/HandAnimation.h"

using namespace openblack;

namespace
{
constexpr auto k_Frame = std::chrono::microseconds(16'667);

class HandAnimationTest: public ::testing::Test
{
protected:
	void SetUp() override
	{
		const auto* gamePath = std::getenv("OPENBLACK_GAME_PATH");
		if (gamePath == nullptr)
		{
			GTEST_SKIP() << "Set OPENBLACK_GAME_PATH to a Black & White installation to pose the hand";
		}
		const auto data = std::filesystem::path(gamePath) / "Data";

		l3d::L3DFile mesh;
		ASSERT_EQ(mesh.Open(data / "CreatureMesh" / "Hand_Boned_Base2.l3d"), l3d::L3DResult::Success);
		// As L3DMesh poses the rest pose
		for (const auto& bone : mesh.GetBones())
		{
			auto matrix = glm::mat4(bone.orientation[0], bone.orientation[1], bone.orientation[2], 0.0f, bone.orientation[3],
			                        bone.orientation[4], bone.orientation[5], 0.0f, bone.orientation[6], bone.orientation[7],
			                        bone.orientation[8], 0.0f, bone.position.x, bone.position.y, bone.position.z, 1.0f);
			if (bone.parent != std::numeric_limits<uint32_t>::max())
			{
				matrix = _rest[bone.parent] * matrix;
			}
			_parents.push_back(bone.parent);
			_rest.push_back(matrix);
		}

		pack::PackFile pack;
		ASSERT_EQ(pack.Open(data / "CTR" / "hh.hbn"), pack::PackResult::Success);
		ASSERT_TRUE(pack.HasBlock("Hand"));
		morph::MorphFile morphFile;
		ASSERT_EQ(morphFile.Open(pack.GetBlock("Hand"), data), morph::MorphResult::Success);
		ASSERT_TRUE(_animation.Load(morphFile, _parents, _rest));
	}

	static float Distance(const glm::mat4& a, const glm::mat4& b)
	{
		float distance = 0.0f;
		for (glm::length_t c = 0; c < 4; ++c)
		{
			for (glm::length_t r = 0; r < 4; ++r)
			{
				distance = std::max(distance, std::abs(a[c][r] - b[c][r]));
			}
		}
		return distance;
	}

	std::vector<uint32_t> _parents;
	std::vector<glm::mat4> _rest;
	HandAnimation _animation;
};
} // namespace

TEST_F(HandAnimationTest, SpecSlotsHoldTheExpectedAnimations)
{
	// hh.hbn holds 26 of the 69 animations of hndspec5.txt
	using Cycle = HandAnimation::Cycle;
	for (const auto cycle : {Cycle::Wiggle, Cycle::Point, Cycle::Grip, Cycle::Rotate, Cycle::Pitch, Cycle::Zoom})
	{
		const auto* cycleAnimation = _animation.GetAnimation(static_cast<size_t>(cycle));
		ASSERT_NE(cycleAnimation, nullptr) << static_cast<int>(cycle);
		EXPECT_TRUE(cycleAnimation->looping);
	}
	for (const auto lean : {1u, 2u, 43u, 44u})
	{
		const auto* leanAnimation = _animation.GetAnimation(lean);
		ASSERT_NE(leanAnimation, nullptr) << lean;
		EXPECT_FALSE(leanAnimation->looping);
	}
	EXPECT_EQ(_animation.GetAnimation(46), nullptr); // Lrotate_lr
	EXPECT_EQ(_animation.GetAnimation(static_cast<size_t>(Cycle::Wiggle))->duration, 266u);
	EXPECT_EQ(_animation.GetAnimation(static_cast<size_t>(Cycle::Grip))->duration, 533u);
}

TEST_F(HandAnimationTest, StandPoseKeepsThePalmAtRest)
{
	// The stand pose only bends the fingers: the palm and the knuckle bones keep their rest pose
	_animation.Update(k_Frame, HandAnimation::State::Normal, HandAnimation::Cycle::Wiggle, {320, 240});
	const auto& bones = _animation.GetBoneMatrices();
	ASSERT_EQ(bones.size(), _rest.size());
	for (size_t i = 0; i < 8; ++i)
	{
		for (glm::length_t c = 0; c < 4; ++c)
		{
			const auto limit = c < 3 ? 0.001f : 0.2f;
			for (glm::length_t r = 0; r < 3; ++r)
			{
				EXPECT_NEAR(bones[i][c][r], _rest[i][c][r], limit) << "bone " << i << " column " << c;
			}
		}
	}
	bool fingersBent = false;
	for (size_t i = 8; i < bones.size(); ++i)
	{
		fingersBent |= Distance(bones[i], _rest[i]) > 1.0f;
	}
	EXPECT_TRUE(fingersBent);
}

TEST_F(HandAnimationTest, PosesStayRigid)
{
	using Cycle = HandAnimation::Cycle;
	for (const auto cycle : {Cycle::Wiggle, Cycle::Grip, Cycle::Rotate})
	{
		for (uint32_t time = 0; time < 533; time += 37)
		{
			for (const auto lean : {glm::vec2(-80.0f, -80.0f), glm::vec2(0.0f), glm::vec2(80.0f, 40.0f)})
			{
				for (const auto& pose : _animation.EvaluatePoses(cycle, time, lean))
				{
					for (const auto& row : pose.rotation)
					{
						const auto length = std::sqrt((row[0] * row[0]) + (row[1] * row[1]) + (row[2] * row[2]));
						EXPECT_NEAR(length, 1.0f, 0.01f);
					}
					EXPECT_TRUE(std::isfinite(pose.translation.x));
				}
			}
		}
	}
}

TEST_F(HandAnimationTest, CursorLagLeansTheHand)
{
	using Cycle = HandAnimation::Cycle;
	const auto centre = _animation.EvaluatePoses(Cycle::Wiggle, 0, glm::vec2(0.0f));
	const auto left = _animation.EvaluatePoses(Cycle::Wiggle, 0, glm::vec2(-80.0f, 0.0f));
	const auto right = _animation.EvaluatePoses(Cycle::Wiggle, 0, glm::vec2(80.0f, 0.0f));
	// Lwiggle_lr turns the root
	float leftTurn = 0.0f;
	float rightTurn = 0.0f;
	for (size_t r = 0; r < 3; ++r)
	{
		for (size_t c = 0; c < 3; ++c)
		{
			leftTurn = std::max(leftTurn, std::abs(left.at(0).rotation.at(r).at(c) - centre.at(0).rotation.at(r).at(c)));
			rightTurn = std::max(rightTurn, std::abs(right.at(0).rotation.at(r).at(c) - centre.at(0).rotation.at(r).at(c)));
		}
	}
	EXPECT_GT(leftTurn, 0.05f);
	EXPECT_GT(rightTurn, 0.05f);

	// Moving the cursor makes the smoothed cursor trail it, settling when it stops
	_animation.Update(k_Frame, HandAnimation::State::Normal, Cycle::Wiggle, {100, 240});
	_animation.Update(k_Frame, HandAnimation::State::Normal, Cycle::Wiggle, {300, 240});
	// Hovering, the hand leans back towards where the cursor came from
	EXPECT_FLOAT_EQ(_animation.GetLean().x, -80.0f);
	for (int i = 0; i < 600; ++i)
	{
		_animation.Update(k_Frame, HandAnimation::State::Normal, Cycle::Wiggle, {300, 240});
	}
	EXPECT_NEAR(_animation.GetLean().x, 0.0f, 0.5f);
}

TEST_F(HandAnimationTest, GrippingCrossFadesToTheGripCycle)
{
	using Cycle = HandAnimation::Cycle;
	using State = HandAnimation::State;
	for (int i = 0; i < 30; ++i)
	{
		_animation.Update(k_Frame, State::Normal, Cycle::Wiggle, {320, 240});
	}
	const auto idle = _animation.GetBoneMatrices();
	_animation.Update(k_Frame, State::Camera, Cycle::Grip, {320, 240});
	const auto fading = _animation.GetBoneMatrices();
	for (int i = 0; i < 30; ++i)
	{
		_animation.Update(k_Frame, State::Camera, Cycle::Grip, {320, 240});
	}
	const auto gripping = _animation.GetBoneMatrices();

	float fadeStep = 0.0f;
	float gripChange = 0.0f;
	for (size_t i = 0; i < idle.size(); ++i)
	{
		fadeStep = std::max(fadeStep, Distance(fading[i], idle[i]));
		gripChange = std::max(gripChange, Distance(gripping[i], idle[i]));
	}
	EXPECT_GT(gripChange, 1.0f);
	// One frame into the 0.13s fade the hand has moved only part of the way
	EXPECT_LT(fadeStep, gripChange * 0.5f);
}

TEST(HandSize, FollowsCHandSetDistanceFromView)
{
	using openblack::HandAnimation;
	EXPECT_FLOAT_EQ(HandAnimation::SizeAtDistance(100.0f), 1.0f);
	EXPECT_FLOAT_EQ(HandAnimation::SizeAtDistance(150.0f), 1.0f);
	EXPECT_NEAR(HandAnimation::SizeAtDistance(5.0f), std::pow(0.5f, 0.8f), 0.0001f);
	EXPECT_NEAR(HandAnimation::SizeAtDistance(300.0f), 2.0f * (1.0f - 150.0f / 1650.0f * 0.3f), 0.0001f);
	// Beyond the hand's furthest distance it stops growing
	EXPECT_FLOAT_EQ(HandAnimation::SizeAtDistance(5000.0f), HandAnimation::SizeAtDistance(1800.0f));
}

TEST_F(HandAnimationTest, HoldingASeedTakesAStillFrameAndFadesInAndOut)
{
	using Cycle = HandAnimation::Cycle;
	_animation.Update(k_Frame, HandAnimation::State::Normal, Cycle::Wiggle, {320, 240});
	const auto normal = _animation.GetBoneMatrices();
	// Taking hold cross-fades to the side hold's frame, which then stays still however long it is held
	_animation.UpdateHeld(k_Frame, Cycle::HoldSide, 66, {320, 240});
	const auto fading = _animation.GetBoneMatrices();
	for (int frame = 0; frame < 20; ++frame)
	{
		_animation.UpdateHeld(k_Frame, Cycle::HoldSide, 66, {320, 240});
	}
	const auto held = _animation.GetBoneMatrices();
	_animation.UpdateHeld(k_Frame, Cycle::HoldSide, 66, {320, 240});
	float still = 0.0f;
	float faded = 0.0f;
	float moved = 0.0f;
	for (size_t i = 0; i < held.size(); ++i)
	{
		still = std::max(still, Distance(held[i], _animation.GetBoneMatrices()[i]));
		faded = std::max(faded, Distance(fading[i], held[i]));
		moved = std::max(moved, Distance(normal[i], held[i]));
	}
	EXPECT_LT(still, 1e-5f);
	EXPECT_GT(moved, 1e-3f);
	EXPECT_GT(faded, 1e-4f);
	// Without leaning, whatever the cursor does
	_animation.UpdateHeld(k_Frame, Cycle::HoldSide, 66, {900, 700});
	EXPECT_EQ(_animation.GetLean(), glm::vec2(0.0f));
	EXPECT_NE(_animation.GetCursorLag(), glm::vec2(0.0f));
	// The fingertips' middle is somewhere in the hand
	EXPECT_GT(glm::length(_animation.LeafBoneCentre()), 0.0f);
}

TEST_F(HandAnimationTest, StandardSizeSpansThreeUnitsTwo)
{
	// The rest pose's bones of Hand_Boned_Base2 span 536.75 units from top to bottom
	EXPECT_NEAR(_animation.ScaleAtDistance(100.0f), 3.2f / 536.75f, 0.0001f);
}
