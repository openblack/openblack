/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "GroundBlobs.h"

#include <glm/geometric.hpp>

namespace openblack::graphics::ground_blobs
{

namespace
{
/// Half a blob's width, either way across the foot: 0.2 along the diagonals
const glm::vec3 k_Across = glm::normalize(glm::vec3(-1.0f, 0.0f, 1.0f)) * 0.2f;
const glm::vec3 k_Back = -k_Across;
/// The light's offset, 2 units along the diagonal
const glm::vec3 k_Offset = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f)) * 2.0f;
/// How far behind the foot a blob starts, as a part of its fall
constexpr float k_Behind = -0.02f;
} // namespace

glm::vec3 Fall(const glm::vec3& landNormal, float scale)
{
	const auto offset = k_Offset * scale;
	return offset - (glm::dot(offset, landNormal) * landNormal);
}

Quad MakeQuad(const glm::vec3& foot, const glm::vec3& fall)
{
	const auto behind = fall * k_Behind;
	return {{foot + behind + k_Back, foot + behind + k_Across, foot + fall + k_Across, foot + fall + k_Back}};
}

std::array<Quad, 2> Feet(const glm::vec3& first, const glm::vec3& second, const glm::vec3& fall)
{
	return {MakeQuad(first, fall + ((second - first) * 0.5f)), MakeQuad(second, fall + ((first - second) * 0.5f))};
}

} // namespace openblack::graphics::ground_blobs
