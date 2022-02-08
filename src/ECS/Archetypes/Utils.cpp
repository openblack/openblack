/*****************************************************************************
 * Copyright (c) 2018-2022 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *****************************************************************************/

#include "Utils.h"

#include <glm/glm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "3D/L3DMesh.h"
#include "3D/MeshPack.h"
#include "ECS/Components/Transform.h"
#include "Game.h"

using namespace openblack;
using namespace openblack::ecs::components;

std::pair<glm::vec2, float> openblack::ecs::archetypes::GetFixedObstacleBoundingCircle(MeshId meshId,
                                                                                       const Transform& transform)
{
	const auto& bb = Game::instance()->GetMeshPack().GetMesh(meshId).GetBoundingBox();
	const auto bbSize = glm::max(glm::vec2(1.0f, 1.0f), glm::xz(bb.Size() * 0.5f * transform.scale));
	glm::mat4 modelMatrix = glm::mat4(1.0f);
	modelMatrix = glm::translate(modelMatrix, transform.position);
	modelMatrix *= glm::mat4(transform.rotation);
	modelMatrix = glm::scale(modelMatrix, transform.scale);
	const auto point = glm::xz(glm::vec4(bb.Center(), 1.0f) * glm::transpose(modelMatrix));
	const auto radius = (glm::compMax(bbSize) / glm::compMin(bbSize) > 1.4) ? glm::length(bbSize) : glm::compMax(bbSize);

	return std::make_pair(point, radius);
}
