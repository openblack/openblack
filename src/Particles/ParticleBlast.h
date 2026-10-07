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

#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <entt/core/fwd.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "3D/AllMeshes.h"

namespace openblack::particles
{
struct WaveTarget;
}

/// The beam explosion's sums: when it acts, how far its wave reaches and when the wave reaches an object, the beam's fall
/// and the cones' shapes, and how the objects it breaks come apart and fly
namespace openblack::particles::blast
{

/// An explosion without a miracle reaches this far for its shield test
constexpr float k_DefaultEffectRadius = 5.0f;
/// The rocks it throws up
constexpr MeshId k_RockMesh = MeshId::Z_SpellRock01;
/// The heap of rubble it leaves on dry land, this big, for so long
constexpr MeshId k_RubbleMesh = MeshId::TreeRootsPile;
constexpr float k_RubbleScale = 8.0f;
constexpr float k_RubbleMilliseconds = 15000.0f;
/// The pieces fly out from this far below its centre
constexpr float k_FragmentDrop = 5.0f;
/// A chain of joined triangles holds at most this many
constexpr size_t k_MostTrianglesInPiece = 16;
/// The wave's range grows with the caster's tribal power within these
constexpr float k_LeastTribalPower = 1.0f;
constexpr float k_MostTribalPower = 5.0f;

/// The explosion rule's properties
struct ExplosionRules
{
	int maxObjectsToDelete {20};
	int maxObjectsToExplode {20};
	float maxDistance {100.0f};
	float blastSpeed {10.0f};
	float spreadSpeed {10.0f};
	float timeToDoEventsFor {5.0f};
	float initialDelay {3.5f};
	float smokeDelay {3.0f};
	float beamDelay {0.0f};
};

/// Whether an explosion of an age sends its event this step: from its initial delay for its time to do events
[[nodiscard]] bool SendsEvents(float age, const ExplosionRules& rules);
/// How far its wave reaches
[[nodiscard]] float WaveRange(float maxDistance, float tribalPower);
/// Whether the wave's front has reached an object, by its edge
[[nodiscard]] bool Reached(float front, const WaveTarget& target, const glm::vec3& centre);
/// Where the straight way from one point to another first enters a sphere; its end when it never does
[[nodiscard]] glm::vec3 EntersSphere(const glm::vec3& from, const glm::vec3& to, const glm::vec3& centre, float radius);
/// Where the pieces fly out from
[[nodiscard]] glm::vec3 FragmentOrigin(const glm::vec3& centre);
/// A piece's velocity: out from the origin at the speed, plus a random part of it, less of it upwards
[[nodiscard]] glm::vec3 FragmentVelocity(const glm::vec3& piece, const glm::vec3& origin, float speed, float randomFactor,
                                         const glm::vec3& randomUnit);

/// How far along a move an atom is at an age, none outside its time; it reaches the end on the step that takes it past.
/// Moving smoothly it eases in and out.
[[nodiscard]] std::optional<float> MoveFraction(float age, float dt, float start, float stop, bool smoothly);

/// An atom scaled across and in height apart: the scale across, and the stretch that makes its height its own
struct XYZScale
{
	float across;
	float stretch;
};
[[nodiscard]] XYZScale ScaleXYZ(float across, float height);

/// A model's triangles as one of its primitives has them
struct SourcePrimitive
{
	std::span<const glm::vec3> positions;
	std::span<const glm::vec2> uvs;
	std::span<const glm::vec3> normals;
	/// Three to a triangle, counting into the positions
	std::span<const uint16_t> indices;
	uint32_t skin {0};
};

/// A piece of a model: a chain of its joined triangles, in the model's frame, three corners to a triangle
struct FragmentPiece
{
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	/// Each triangle's skin
	std::vector<uint32_t> skins;
};

/// How a model breaks: each primitive's triangles into chains, each starting from the first triangle not yet taken and
/// going on to the first neighbour not yet taken (sharing an edge) of the last, until there is none or it holds sixteen
[[nodiscard]] std::vector<FragmentPiece> BreakIntoPieces(std::span<const SourcePrimitive> primitives);

/// A piece placed in the world: the corners about its middle, and the model whose skins it is drawn with. Its normals
/// stay as the model has them, unturned by where the model stood, as the game leaves them.
struct MeshFragment
{
	entt::id_type mesh {0};
	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	std::vector<uint32_t> skins;
};
struct PlacedFragment
{
	glm::vec3 centre;
	std::shared_ptr<const MeshFragment> shape;
};
/// A piece of a model placed by its transform: its middle in the world, and its corners about it
[[nodiscard]] PlacedFragment PlaceFragment(const FragmentPiece& piece, const glm::mat4& transform, entt::id_type mesh);

} // namespace openblack::particles::blast
