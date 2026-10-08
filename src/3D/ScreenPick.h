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
#include <span>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// What is under the cursor, as the game finds it while it draws: each object whose bounding sphere covers the cursor's
/// pixel is projected to the screen triangle by triangle, the triangles clipped to the view and those facing away
/// dropped, and the first triangle covering the pixel gives the object's depth. The interface then weighs the nearest
/// object against the land under the cursor.
namespace openblack::screen_pick
{

/// The view the cursor is tested in
struct View
{
	/// World to clip space: its rows give the clip x and y, and the depth along the view as w
	glm::mat4 worldToClip {1.0f};
	glm::vec2 resolution {0.0f};
	/// How far the near plane is ahead of the camera
	float near {1.0f};
	/// The projection's scale of x over the depth: 1 over the tangent of half the horizontal field of view
	float xScale {1.0f};
	glm::vec3 camera {0.0f};
	/// The cursor's pixel, from the top left
	glm::vec2 cursor {0.0f};
};

/// How far along the view a point is: its clip w
[[nodiscard]] float Depth(const View& view, glm::vec3 point);

/// Whether the cursor is over an object's bounding sphere: its centre in the world, its radius (the model's half
/// diagonal times its scale) and the object's origin. An object whose sphere is behind the near plane is not; one the
/// camera is inside is, wherever the cursor is; otherwise its circle on the screen must reach the screen and hold the
/// cursor strictly inside it.
[[nodiscard]] bool CursorOverSphere(const View& view, glm::vec3 centre, float radius, glm::vec3 origin);

/// A corner of a triangle in clip space with its texture coordinates
struct ClipCorner
{
	float x;
	float y;
	float w;
	glm::vec2 uv;
};

/// Where a corner lands in clip space
[[nodiscard]] ClipCorner ToClip(const View& view, glm::vec3 world, glm::vec2 uv);

/// Whether a texel of a texture is solid, at 64 by 64 places over it: the texel's alpha is above nothing
using AlphaMask = std::array<bool, 64 * 64>;
/// The mask of a texture of 4-bit channels, alpha in each texel's top 4 bits, by rows of width texels
[[nodiscard]] AlphaMask MaskOf(std::span<const uint16_t> texels, uint32_t width, uint32_t height);
/// Whether the mask is solid at texture coordinates u, v
[[nodiscard]] bool Solid(const AlphaMask& mask, glm::vec2 uv);

/// A primitive of a model, its corners already in clip space
struct Primitive
{
	std::span<const ClipCorner> corners;
	/// Three corner indices a triangle
	std::span<const uint16_t> indices;
	/// Drawn from both sides: no triangle facing away is dropped
	bool twoSided {false};
	/// The texture's solid places, for a model tested through its texture's holes; none tests the triangles alone
	const AlphaMask* mask {nullptr};
};

/// A corner on the screen: its pixel, near over its depth, and its texture coordinates
struct ScreenCorner
{
	float x;
	float y;
	float depth;
	glm::vec2 uv;
};

/// Room a pick keeps from one primitive to the next, so that testing a frame's objects stops allocating once it has grown
struct PickScratch
{
	std::vector<uint8_t> beyond;
	std::vector<ScreenCorner> onScreen;
	std::vector<std::array<ScreenCorner, 3>> drawn;
};

/// How far along the view the first triangle of the primitive covering the cursor is, none when none does
[[nodiscard]] std::optional<float> FirstHit(const View& view, const Primitive& primitive);
/// The same, reusing the room of earlier tests
[[nodiscard]] std::optional<float> FirstHit(const View& view, const Primitive& primitive, PickScratch& scratch);

/// An object drawn this frame whose sphere the cursor may be over, as the interface weighs it
struct Candidate
{
	/// Where its sphere's centre is, its radius (the model's half diagonal times its scale) and the object's origin
	glm::vec3 centre {0.0f};
	float radius {0.0f};
	glm::vec3 origin {0.0f};
	/// Half its model's extents across x and z, unscaled: its footprint
	glm::vec2 halfExtents {0.0f};
};
/// The object the interface picks among the frame's objects in the order they are drawn: one whose sphere the cursor is
/// over and which lies no further than the nearest so far by more than its radius is tested, its distance found by
/// `distanceOf` (none for a miss), and it replaces the nearest only when strictly nearer, so a tie keeps the first drawn
struct Picked
{
	size_t index;
	float distance;
};
[[nodiscard]] std::optional<Picked> PickAmong(const View& view, std::span<const Candidate> candidates,
                                              const std::function<std::optional<float>(size_t)>& distanceOf);

/// The interface's choice between the object picked and the land under the cursor: the object is kept unless the land is
/// nearer or as near and the land's point lies outside the object's footprint, the circle through its model's
/// half-extents across x and z (unscaled) about its origin
[[nodiscard]] bool LandHidesObject(float landDistance, float objectDistance, glm::vec2 landPoint, glm::vec2 objectOrigin,
                                   glm::vec2 halfExtents);

/// Where a line from a point along a direction first meets a model's triangles, and the face's normal turned along the
/// line, as the hand feels a model it rests on
struct MeshHit
{
	glm::vec3 point;
	glm::vec3 normal;
	float distance;
	/// The triangle met, by its first index, and where on it: the point is its first corner plus s of the way along its
	/// first side and t along its third
	uint32_t firstIndex {0};
	float s {0.0f};
	float t {0.0f};
};
/// The triangles are three corners each in the world. Faces nearly side-on to the line (normal and line at less than
/// 0.005 of a right angle's cosine) are passed by; only hits ahead of the start count unless behind is allowed.
[[nodiscard]] std::optional<MeshHit> NearestIntersection(std::span<const glm::vec3> corners, std::span<const uint16_t> indices,
                                                         glm::vec3 origin, glm::vec3 direction, bool behindAllowed);

} // namespace openblack::screen_pick
