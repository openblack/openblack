/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureRig.h"

#include <glm/gtx/transform.hpp>

using namespace openblack;
using namespace openblack::creature;

const skeletal_animation::Animation* CreatureRig::GetAnimation(Mesh mesh, size_t index) const
{
	const auto slot = static_cast<size_t>(mesh);
	if (slot < animations.size() && index < animations.at(slot).size() && animations.at(slot)[index].has_value())
	{
		return &*animations.at(slot)[index];
	}
	const auto& base = animations.front();
	return index < base.size() && base[index].has_value() ? &*base[index] : nullptr;
}

glm::mat4 creature::PlacementMatrix(const glm::vec3& position, const glm::mat3& rotation, const glm::vec3& scale)
{
	auto model = glm::mat4(rotation);
	model = glm::translate(model, position * rotation);
	return glm::scale(model, scale);
}

glm::mat4 creature::PosedBone(uint32_t bone, std::span<const glm::mat4> boneMatrices, const glm::mat4& placement)
{
	return bone < boneMatrices.size() ? placement * boneMatrices[bone] : placement;
}

std::array<glm::vec3, 3> creature::PosedTriangle(const std::array<std::array<glm::vec3, 3>, CreatureRig::k_MeshCount>& vertices,
                                                 const std::array<uint32_t, 3>& bones, const creature_morph::Morph& morph,
                                                 std::span<const glm::mat4> boneMatrices, const glm::mat4& placement)
{
	const auto evilGood = morph.evilGood < 0.0f ? CreatureRig::Mesh::Evil : CreatureRig::Mesh::Good;
	const auto thinFat = morph.thinFat < 0.0f ? CreatureRig::Mesh::Thin : CreatureRig::Mesh::Fat;
	const auto weakStrong = morph.weakStrong < 0.0f ? CreatureRig::Mesh::Weak : CreatureRig::Mesh::Strong;
	std::array<glm::vec3, 3> world {};
	for (size_t i = 0; i < world.size(); ++i)
	{
		const auto vertexOf = [&vertices, i](CreatureRig::Mesh mesh) { return vertices.at(static_cast<size_t>(mesh)).at(i); };
		const auto local = creature_morph::Blend(vertexOf(CreatureRig::Mesh::Base), vertexOf(evilGood), vertexOf(thinFat),
		                                         vertexOf(weakStrong), morph);
		world.at(i) = glm::vec3(PosedBone(bones.at(i), boneMatrices, placement) * glm::vec4(local, 1.0f));
	}
	return world;
}
