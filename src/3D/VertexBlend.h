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

#include <span>
#include <vector>

#include <glm/vec3.hpp>

/// Where the parts of a boned body meet, a vertex placed by one bone is drawn part of the way towards a vertex at the
/// same place placed by the next, so the seams at the shoulders, neck and tail bend rather than tear. Each primitive
/// lists its blends: the vertex that moves, the vertex it moves towards and how far, at most a half. Only the position
/// moves, after both vertices have been placed by their own bones; the normals, colours and texture coordinates stay.
/// No vertex moves towards a vertex that moves itself.
namespace openblack::vertex_blend
{
/// A blend as a primitive lists it, its vertices counted in the primitive
struct Blend
{
	uint16_t vertex;
	uint16_t towards;
	float weight;
};

/// What a vertex of a submesh is blended with: the vertex it moves towards, counted in the submesh, and how far
struct Partner
{
	int32_t vertex {-1};
	float weight {0.0f};

	[[nodiscard]] bool Blended() const { return vertex >= 0 && weight > 0.0f; }
};

/// Every vertex's partner in a submesh of primitives, from the blends each lists in turn: primitiveVertices[i] vertices
/// and primitiveBlends[i] blends for primitive i. Blends naming vertices outside their primitive are left out.
[[nodiscard]] std::vector<Partner> Partners(std::span<const uint32_t> primitiveVertices,
                                            std::span<const uint32_t> primitiveBlends, std::span<const Blend> blends);

/// The positions of a submesh placed by their bones, with the blends applied
void Apply(std::span<glm::vec3> positions, std::span<const Partner> partners);

/// A blend's weight in a whole number of 32767ths, as it is handed to the vertex shader, and back
[[nodiscard]] int16_t QuantiseWeight(float weight);
[[nodiscard]] float WeightOf(int16_t quantised);
} // namespace openblack::vertex_blend
