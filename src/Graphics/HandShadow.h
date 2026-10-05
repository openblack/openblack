/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <optional>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "3D/SkyInterface.h"

namespace openblack::graphics
{

/// The god hand's shadow, after the game's dynamic shadows. Black & White draws the hand's silhouette into a 32x32
/// texture with eight coverage samples per texel and projects it onto the land, darkening it by at most 8/15. The
/// shadow fades out as the camera pulls away from the ground beneath the hand. Unlike the game, whose light sits in
/// a fixed direction, the shadow here follows the sun across the sky and fades out at night.
struct HandShadow
{
	/// Silhouette render target size. The game's shadow texture has 32x32 texels across the silhouette's bounds; this
	/// one spans the hand's bounding sphere, which the silhouette fills about two thirds of, so the shadow is as soft.
	static constexpr uint16_t k_TextureSize = 48;
	/// Darkness under a fully covered texel. The game's texture holds the coverage as 8 of the 15 levels of a 4 bit
	/// alpha channel, yet the game's shadows darken the land by only about a fifth: measured on a screenshot of the
	/// hand's shadow over grass, (86, 74, 32) against (100-110, 94-99, 35-45) around it.
	static constexpr float k_MaxDarkness = 0.21f;
	/// Camera distance from the ground beneath the hand, in hand radii, beyond which the shadow starts fading and
	/// where it has gone
	static constexpr float k_FadeStart = 50.0f;
	static constexpr float k_FadeEnd = 80.0f;

	/// View and projection of the silhouette pass
	glm::mat4 view;
	glm::mat4 projection;
	/// World position to shadow texture coordinates in xy and distance past the hand's centre along the light in z
	glm::mat4 receiverMatrix;
	/// Direction the sunlight travels in
	glm::vec3 lightDirection;
	glm::vec3 centre;
	float radius;
	/// Fraction of k_MaxDarkness applied, 0 when there is no shadow
	float strength;
	/// Receivers this far along the light past the hand's centre or further are shaded, so the land in front of the
	/// hand is not
	float startDepth;

	/// Direction sunlight travels in at a time of day in hours. The sun rises and sets with the sky's dusk and climbs
	/// highest at midday, coming from the same side of the island as Black & White's fixed light.
	[[nodiscard]] static glm::vec3 SunlightDirection(float hours, const SkyInterface::DayNightTimes& times);
	/// 1 in full day, 0 at night, ramping through dawn and dusk
	[[nodiscard]] static float Daylight(float hours, const SkyInterface::DayNightTimes& times);
	/// 1 near the ground, fading to 0 by k_FadeEnd hand radii away
	[[nodiscard]] static float DistanceFade(float cameraDistance, float handRadius);

	/// handBones are the model matrices of the hand's bones in world space. originBottomLeft and homogeneousDepth
	/// are those of bgfx::Caps.
	[[nodiscard]] static std::optional<HandShadow> Compute(const std::vector<glm::mat4>& handBones, glm::vec3 cameraPosition,
	                                                       float groundHeight, float hours,
	                                                       const SkyInterface::DayNightTimes& times, bool originBottomLeft,
	                                                       bool homogeneousDepth);
};

} // namespace openblack::graphics
