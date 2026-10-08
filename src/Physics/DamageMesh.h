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

#include <array>
#include <functional>
#include <optional>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// How a building breaks where a rock strikes it: its model is kept as loose triangles in the world, those near the
/// rock's line of flight are knocked out (big triangles halved until they fit), and the pieces that no longer hold on to
/// the rest fly off.
namespace openblack::physics::damage
{

/// Triangles whose shortest side, squared, is over these are of size 3, 2 and 1; the rest of size 0
inline constexpr std::array<float, 3> k_SizeClassEdges {32.0f, 8.0f, 2.0f};
/// Two corners closer than this on every axis are the same corner
inline constexpr float k_SameCorner = 0.01f;
/// The most triangles a primitive keeps, or breaks off, at one blow
inline constexpr size_t k_MostListed = 2048;
/// The most groups of joined triangles a mesh is sorted into
inline constexpr int32_t k_MostGroups = 64;
/// A group touching the land holds on: a corner lower than this above the land under it
inline constexpr float k_AnchorHeight = 0.1f;
/// What of a rock's speed the pieces it breaks off fly with
inline constexpr float k_PieceSpeedShare = 0.3f;
/// How much wider than the rock its line of breakage reaches
inline constexpr float k_ReachBeyondRock = 0.7f;
/// The spin of the pieces a blow breaks off, and of the parts that fall away after, per unit of a random draw from -100
/// to 100
inline constexpr float k_BrokenSpin = 0.01f;
inline constexpr float k_FallingSpin = 0.02f;
/// Game turns a piece lasts for each triangle it was broken off with
inline constexpr uint32_t k_TurnsPerTriangle = 100;
/// A piece that comes to rest bigger than this goes back into its building as rubble
inline constexpr float k_RubbleArea = 9.0f;

struct Corner
{
	glm::vec3 position {0.0f};
	glm::vec2 uv {0.0f};
};

struct Triangle
{
	std::array<Corner, 3> corners;
	/// The triangle of the same primitive across each side (from corner i to i + 1), none at an open side
	std::array<int32_t, 3> neighbours {-1, -1, -1};
	/// The group of joined triangles it belongs to, none before sorting or past the most groups
	int32_t group {-1};
	/// How big it is, and the size it counts as part of the building at: a triangle counts while the two are equal
	int8_t sizeClass {0};
	int8_t countedClass {0};

	[[nodiscard]] bool CountsAsBuilding() const { return sizeClass == countedClass; }
};

/// The triangles of one of the model's primitives, drawn with its material
struct Primitive
{
	/// Which of the model's primitives it came from, for its material
	uint32_t material {0};
	std::vector<Triangle> triangles;
};

/// A broken building's model, or a piece broken off it
struct Mesh
{
	std::vector<Primitive> primitives;
	/// How many triangles it had when made, against which what is left of the building is counted
	uint32_t trianglesAtCreation {0};
	/// What is left of the building: the share of its first triangles that still count
	float remaining {1.0f};
	/// How much snow lies on it, 0 to 255, and whether it is held there while a piece flies
	uint8_t snowLevel {0};
	bool snowFrozen {false};

	[[nodiscard]] size_t TriangleCount() const;
};

/// The size of a triangle, from its shortest side
[[nodiscard]] int8_t SizeClassOf(const Triangle& triangle);
/// A triangle made from three corners, its size counted
[[nodiscard]] Triangle MakeTriangle(const std::array<Corner, 3>& corners);
/// Finds each triangle's neighbour across each of its sides: a triangle of the same primitive with both ends of the side
/// among its corners
void LinkNeighbours(Primitive& primitive);

/// How many of a triangle's corners lie within a reach of the line through a point along a direction (of the point
/// itself when the direction is nothing)
[[nodiscard]] int CornersNear(const Triangle& triangle, glm::vec3 point, glm::vec3 direction, float reach);
/// A triangle halved through the middle of its longest side, both halves a size smaller
[[nodiscard]] std::array<Triangle, 2> Halve(const Triangle& triangle);

/// What a blow does to a primitive's triangles
struct Sorted
{
	std::vector<Triangle> kept;
	std::vector<Triangle> broken;
};
/// Sorts a primitive's triangles by how many corners lie near the blow's line: none kept, all three broken, one or two
/// halved and sorted again; the smallest ones go by the most corners. Each list holds at most the most a primitive keeps.
[[nodiscard]] Sorted SortByBlow(const std::vector<Triangle>& triangles, glm::vec3 point, glm::vec3 direction, float reach);

/// A blow on a mesh: each primitive keeps its triangles away from the blow's line, and what broke off each primitive
/// comes back, in the primitives' order, as a primitive of its own (none for a primitive that lost nothing)
[[nodiscard]] std::vector<Primitive> Strike(Mesh& mesh, glm::vec3 point, glm::vec3 direction, float reach);

/// Sorts every triangle of the mesh into groups that join each other (at least two corners shared), across all its
/// primitives. The number of groups.
int32_t LabelGroups(Mesh& mesh);
/// Which groups hold on: those with a corner just above the land (when the land is given), else only the first
[[nodiscard]] std::vector<bool> Anchors(const Mesh& mesh, int32_t groups,
                                        const std::optional<std::function<float(glm::vec2)>>& landHeight);
/// How many triangles each group has, over all primitives
[[nodiscard]] std::vector<uint32_t> GroupSizes(const Mesh& mesh, int32_t groups);

/// A part that falls away: the triangles of one group in one primitive
struct FallingPart
{
	Primitive primitive;
	int32_t group {-1};
};
/// Takes away every group that doesn't hold on: groups of more than one triangle come back as parts, one per primitive
/// holding some, their triangles no longer counting as building; lone triangles and unsorted ones are lost
[[nodiscard]] std::vector<FallingPart> TakeAwayLooseGroups(Mesh& mesh, const std::vector<bool>& anchors, int32_t groups);

/// What a rock does to a building by its momentum (speed times weight)
enum class Blow : uint8_t
{
	Nothing,
	/// A light knock, heard only
	Knock,
	/// A heavier knock, heard only
	HardKnock,
	/// It breaks the building
	Breaks,
};
inline constexpr float k_KnockMomentum = 300.0f;
inline constexpr float k_HardKnockMomentum = 1000.0f;
inline constexpr float k_BreakingMomentum = 2000.0f;
/// The same rock striking a building again this hard passes through it from then on
inline constexpr float k_PassThroughMomentum = 1400.0f;
[[nodiscard]] Blow JudgeBlow(float momentum);
/// A building drawn at least this much repaired, but not wholly, has its broken model made afresh when struck again
inline constexpr float k_RebuildDrawShare = 0.2f;
[[nodiscard]] bool RebuildsOnBlow(float drawShare);
/// The life a building's repair starts from when it is struck: a little less than its own, so it is drawn mostly
/// unrepaired
[[nodiscard]] float RepairStartLife(float life);
/// How much of a building is drawn standing: how far it is repaired since it was last broken (from the life its repair
/// started at, else a little short of its life) and no more than is built; wholly drawn while it isn't built yet
[[nodiscard]] float DrawShare(float life, float built, std::optional<float> repairStart);

/// How much of a building's life a breaking blow takes, before its defences: down to what is left of its model
[[nodiscard]] float BreakageShare(float life, float remaining);

/// The share of the triangles it was made with that still count, held between 0 and 1 (a blow that only halves
/// triangles leaves more that count than there were); a mesh keeps it only when it fell within
[[nodiscard]] float RemainingFraction(const Mesh& mesh);
/// The middle of a primitive's corners
[[nodiscard]] glm::vec3 CentreOf(const Primitive& primitive);
/// Moves every corner of a primitive by an offset
void Offset(Primitive& primitive, glm::vec3 offset);

} // namespace openblack::physics::damage
