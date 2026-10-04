/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <memory>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace openblack::graphics
{
struct BeamMesh;
}

namespace openblack::ecs::components
{

/// LH3DMist: a dome of mist.l3d that turns to face the camera, drawn by the smoke's alpha with a frame of the smoke
/// texture at its transform's position
struct MistDome
{
	std::shared_ptr<const graphics::BeamMesh> dome;
	/// The frame of the smoke texture, as LH3DObject::SetAnimatedUV slides it
	glm::vec2 uvOffset {0.0f};
	glm::vec4 colour {1.0f};
};

} // namespace openblack::ecs::components
