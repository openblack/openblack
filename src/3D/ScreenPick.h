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

/// How far along the view the first triangle of the primitive covering the cursor is, none when none does
[[nodiscard]] std::optional<float> FirstHit(const View& view, const Primitive& primitive);

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
};
/// The triangles are three corners each in the world. Faces nearly side-on to the line (normal and line at less than
/// 0.005 of a right angle's cosine) are passed by; only hits ahead of the start count unless behind is allowed.
[[nodiscard]] std::optional<MeshHit> NearestIntersection(std::span<const glm::vec3> corners, std::span<const uint16_t> indices,
                                                         glm::vec3 origin, glm::vec3 direction, bool behindAllowed);

} // namespace openblack::screen_pick
