/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#pragma once

#include <BulletDynamics/Dynamics/btRigidBody.h>
#include <LinearMath/btDefaultMotionState.h>

namespace openblack::ecs::components
{

struct RigidBody
{
	btRigidBody handle;
	// TODO(bwrsandman): it would be more cache friendly to not use a pointer here
	std::unique_ptr<btDefaultMotionState> motionState;

	RigidBody(const btRigidBody::btRigidBodyConstructionInfo& info, const btTransform& startTransform)
	    : handle {info}
	    , motionState(std::make_unique<btDefaultMotionState>(startTransform))
	{
		handle.setMotionState(motionState.get());
	}
};

} // namespace openblack::ecs::components
