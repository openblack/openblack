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

#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>

namespace openblack
{
namespace glw
{
struct Glow;
}

/// The two sprites the game's glow of a light draws over each other, additively, from the atmosphere texture
struct Glow
{
	glm::vec3 position;
	/// The light's colour
	glm::vec4 haloColour;
	/// Half as strong, over a grey base, so it is whiter
	glm::vec4 centreColour;
	/// The half widths of the sprites
	float haloSize;
	float centreSize;
	/// The glows aligned to a wall, as those of the temple's windows are, lie in the plane of their light's x and z axes
	/// instead of facing the camera
	std::optional<glm::mat3> orientation;
};

/// A spot light's beam, an open cone drawn additively from its light down its -y axis that fades to nothing
struct LightCone
{
	/// The light's axes and position
	glm::mat4 transform;
	/// Of the cone's narrow end, at the light
	glm::vec4 colour;
	float nearRadius;
	float length;
	/// The cone's full angle, in degrees
	float angle;
};

struct LightEmitter
{
	Glow glow;
	std::optional<LightCone> cone;
};

/// The glow and beam the game draws for a light of a .glw file
[[nodiscard]] LightEmitter MakeLightEmitter(const glw::Glow& light);

struct Lights
{
	std::vector<LightEmitter> emitters;
};
} // namespace openblack
