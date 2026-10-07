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
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// A surface of revolution: a profile curve turned once round the vertical, as the particle files draw the swirl of
/// light under a miracle dispenser, the teleport's pool and the vortices and volcanoes the land opens. The surface is a
/// grid of rings: each ring is a point of the profile, from its middle (t = 0) out to its rim (t = 1), turned round in
/// even steps. Each ring is coloured by how far out it lies: it brightens from black over the inner part, fades out over
/// the outer part, and where it is dark it shines in the specular colour.
namespace openblack::particles::surface
{

/// The curves the surface turns. The radius and height are in the atom's units, scaled by its size.
enum class Profile : uint8_t
{
	/// A flat disk: radius t, height 0
	Disk,
	/// A funnel: radius t, rising as the square root of t from 3 below
	Funnel,
	/// A wider funnel: radius 1.5 t, rising as the square root of 2 t from 3 below
	FunnelSpout,
	/// A bowl: radius t, rising as t squared from 3 below
	FunnelParabola,
};

struct ProfilePoint
{
	float radius;
	float height;
};

/// The profile at t, 0 at the middle to 1 at the rim
[[nodiscard]] ProfilePoint Evaluate(Profile profile, float t);
/// The profile a file's function index names; any other index is the disk
[[nodiscard]] Profile ProfileOf(int functionIndex);

/// How a ring is shaded: its colour's brightness and its alpha, 0..1
struct RingShade
{
	float brightness;
	float alpha;
};

/// The shade of the ring t of the way out: the brightness rises from 0 to 1 over the first fadeIn of the way, and the
/// alpha falls from 1 to 0 over the last fadeOut
[[nodiscard]] RingShade ShadeAt(float t, float fadeIn, float fadeOut);

/// What the surface looks like
struct Shape
{
	Profile profile {Profile::Disk};
	/// Points round each ring and rings out from the middle
	int numU {10};
	int numV {10};
	/// The rings are shaded by ShadeAt, else every point is white and opaque
	bool fadeAlphas {false};
	float fadeIn {0.4f};
	float fadeOut {0.4f};
	/// The shaded rings shine in the specular colour where they are dark
	bool specular {false};
	/// How far the texture spans across and out, in times its size (its pixels over 256)
	glm::vec2 uvScale {1.0f};
};

/// A colour as bytes from the highest: alpha, red, green, blue
using Argb = uint32_t;

struct Vertex
{
	glm::vec3 position;
	glm::vec2 uv;
	Argb colour;
	Argb specular;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	/// Three for each triangle
	std::vector<uint32_t> indices;
	/// One for each point when the surface is lit, none otherwise
	std::vector<glm::vec3> normals;
};

/// The surface in its own frame: numV rings of numU points, the first ring at the middle, each shining in the specular
/// colour where it is dark when the shape asks
[[nodiscard]] Mesh Build(const Shape& shape, Argb specularColour);
/// The normals of a surface to be lit: each point's is the sum of the faces' it is on, made unit only when it is on more
/// than one
void ComputeNormals(Mesh& mesh);

/// The surface swaying: each ring turned about the vertical by its share of the way out times maxTwist times sway, and
/// its texture slid across by maxSlide times sway times the square of the share of the way in. Sway runs from -1 to 1.
/// Only the points and the texture change; the normals stay.
void Sway(const Mesh& still, const Shape& shape, float maxTwist, float maxSlide, float sway, Mesh& out);

/// A plane: the points with dot(normal, point) + offset above 0 are on its positive side
struct Plane
{
	glm::vec3 normal;
	float offset;
};
/// Every triangle that crosses the plane is cut along it into three
void Slice(Mesh& mesh, const Plane& plane);
/// A surface in the world laid over the land: cut along the land's cells, ten across, and their diagonals, then each
/// point raised or lowered by how much the land under it lies above or below the land under the middle
void DrapeOverLand(Mesh& worldMesh, glm::vec2 middle, const std::function<float(glm::vec2)>& landHeight);

/// A colour scaled by a byte factor as the game does: each channel times the factor, over 256, the alpha kept
[[nodiscard]] Argb ScaleRgb(Argb colour, uint32_t factor);
/// Two colours multiplied channel by channel, alpha too, over 256
[[nodiscard]] Argb Modulate(Argb a, Argb b);
/// The light level, 0..255, of a point facing along a normal lit from a direction: the ambient when it faces away, more
/// as it faces the light
[[nodiscard]] uint32_t LightLevel(glm::vec3 normal, glm::vec3 towardsLight, uint32_t ambient);

} // namespace openblack::particles::surface
