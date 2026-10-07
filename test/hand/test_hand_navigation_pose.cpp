/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include <gtest/gtest.h>

#include "3D/HandNavigationPose.h"

using namespace openblack;
using namespace openblack::hand_navigation_pose;
namespace tricon = camera_drag::tricon;

TEST(HandNavigationPose, GrippingTheLandShowsTheGrip)
{
	EXPECT_EQ(WhileDragging(true, tricon::k_Rotate), Pose::Grip);
	EXPECT_EQ(WhileDragging(true, 0), Pose::Grip);
}

TEST(HandNavigationPose, DraggingByTheEdgeShowsTurningTiltingOrZooming)
{
	EXPECT_EQ(WhileDragging(false, tricon::k_Rotate | tricon::k_Pitch), Pose::Rotate);
	EXPECT_EQ(WhileDragging(false, tricon::k_Pitch), Pose::Pitch);
	EXPECT_EQ(WhileDragging(false, tricon::k_Zoom | tricon::k_Pitch), Pose::Zoom);
	EXPECT_EQ(WhileDragging(false, 0), Pose::Idle);
	// Without the camera help's turning, there is no turning pose
	EXPECT_EQ(WhileDragging(false, tricon::k_Rotate, camera_drag::feature::k_Pitch), Pose::Idle);
}

TEST(HandNavigationPose, HoveringAtTheEdgeOffersTurning)
{
	EXPECT_EQ(WhileHovering(tricon::k_Idle), std::nullopt);
	EXPECT_EQ(WhileHovering(tricon::k_Idle | tricon::k_Rotate | tricon::k_Turning), Pose::Rotate);
	// Turning wins over tilting at the very bottom and the top
	EXPECT_EQ(WhileHovering(tricon::k_Idle | tricon::k_Rotate | tricon::k_Pitch), Pose::Rotate);
	EXPECT_EQ(WhileHovering(tricon::k_Pitch), Pose::Pitch);
}

TEST(HandNavigationPose, StandsUpTowardsTheFocus)
{
	const auto up = UpTowardsFocus({0.0f, 0.0f, 0.0f}, {10.0f, 0.0f, 0.0f});
	EXPECT_NEAR(up.x, 10.0f / glm::length(glm::vec3(10.0f, 4.0f, 0.0f)), 1e-5f);
	EXPECT_NEAR(up.y, 4.0f / glm::length(glm::vec3(10.0f, 4.0f, 0.0f)), 1e-5f);
	EXPECT_EQ(UpTowardsFocus({1.0f, 2.0f, 3.0f}, {1.0f, 2.0f, 3.0f}), glm::vec3(0.0f, 1.0f, 0.0f));
}
