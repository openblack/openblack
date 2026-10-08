/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <optional>
#include <span>
#include <vector>

#include <entt/entity/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/ScreenPick.h"

namespace openblack::graphics
{
class L3DMesh;
class L3DSubMesh;
} // namespace openblack::graphics

namespace openblack::ecs
{
class Registry;
}

/// A model's triangles where they are drawn this frame, which the cursor pick and the marks a blow or a burn leaves on a
/// creature's skin both test lines against
namespace openblack::ecs::posed_model
{
/// The bones a model is posed by as it is drawn: an animal by its clip, a villager by its state's clip, a creature by
/// its animations, anything else in the pose its model rests in
[[nodiscard]] std::span<const glm::mat4> BonesOf(const ecs::Registry& registry, entt::entity entity,
                                                 const graphics::L3DMesh& mesh);

/// The corners of a submesh in the world as it is drawn: posed by its bones, a creature's body blended from its
/// meshes, and a model following the land moved up and down with it
void Place(const ecs::Registry& registry, entt::entity entity, const graphics::L3DMesh& mesh, size_t index,
           const glm::mat4& model, std::vector<glm::vec3>& corners);

/// Whether the submesh is drawn at the level of detail and stage the model is drawn at: the nearest level of detail and
/// the first stage, as openblack draws every model
[[nodiscard]] bool IsDrawn(const graphics::L3DSubMesh& subMesh);

/// Where a line first meets a model's nearest level of detail as it is posed, the texture corners of the triangle it
/// meets and which of the model's skins that triangle is drawn with
struct SkinHit
{
	screen_pick::MeshHit hit;
	std::array<glm::vec2, 3> uvs;
	/// Which of the model's skins: the last of them that the first submesh's primitive of the same place uses, the
	/// first skin when none does
	uint32_t skin {0};
};
[[nodiscard]] std::optional<SkinHit> NearestSkinHit(const ecs::Registry& registry, entt::entity entity,
                                                    const graphics::L3DMesh& mesh, const glm::mat4& model, glm::vec3 origin,
                                                    glm::vec3 direction);
} // namespace openblack::ecs::posed_model
