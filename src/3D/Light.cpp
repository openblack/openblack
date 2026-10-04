/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Light.h"

#include <algorithm>
#include <bit>

#include <GLWFile.h>
#include <glm/ext/vector_uint3_sized.hpp>
#include <glm/geometric.hpp>

using namespace openblack;

namespace
{
// Besides its colour and position, LH3D reads a .glw light's type, a 4x3 matrix of its axes and position (unk14 to
// unk25), its cone's length (unk26), flags (the bits of dirX), cone angle (dirY) and size (emitterSize)
enum LightFlags : uint32_t
{
	k_DrawsCone = 1u << 0,
	k_AlignedGlow = 1u << 3,
};
constexpr uint32_t k_SpotLight = 1;

/// LH3D turns a light's colour into a byte of a sprite's colour at half intensity: 1 is 128, and it saturates
uint8_t ColourByte(float value)
{
	return static_cast<uint8_t>(std::clamp(static_cast<int>(value * 128.0f), 0, 255));
}

glm::vec4 ToColour(glm::u8vec3 bytes)
{
	return {glm::vec3(bytes) / 255.0f, 1.0f};
}
} // namespace

LightEmitter openblack::MakeLightEmitter(const glw::Glow& light)
{
	const glm::vec3 colour {light.red, light.green, light.blue};
	const glm::vec3 axisX {light.unk14, light.unk15, light.unk16};
	const glm::vec3 axisY {light.unk17, light.unk18, light.unk19};
	const glm::vec3 axisZ {light.unk20, light.unk21, light.unk22};
	const glm::vec3 origin {light.unk23, light.unk24, light.unk25};
	const auto flags = std::bit_cast<uint32_t>(light.dirX);
	const auto type = light.unk1;
	const float coneLength = light.unk26;
	const float coneAngle = light.dirY;

	// The glow grows with the light's brightness and size, and never quite vanishes
	constexpr float k_MinimumSize = 0.0001f;
	const float size = glm::length(colour) * light.emitterSize * 0.05f;

	LightEmitter emitter {};
	const glm::u8vec3 halo {ColourByte(colour.r), ColourByte(colour.g), ColourByte(colour.b)};
	emitter.glow.position = {light.posX, light.posY, light.posZ};
	emitter.glow.haloColour = ToColour(halo);
	emitter.glow.centreColour = ToColour(glm::u8vec3(128) + halo / glm::u8vec3(2));
	emitter.glow.haloSize = std::max(size * 2.0f, k_MinimumSize);
	emitter.glow.centreSize = std::max(size * 1.2f, k_MinimumSize);
	if ((flags & k_AlignedGlow) != 0)
	{
		// The sprite's x and y run along the light's x and z
		emitter.glow.orientation = glm::mat3(axisX, axisZ, axisY);
	}

	if (type == k_SpotLight && (flags & k_DrawsCone) != 0)
	{
		const auto coneColour = colour * 0.5f;
		emitter.cone = LightCone {
		    .transform =
		        glm::mat4(glm::vec4(axisX, 0.0f), glm::vec4(axisY, 0.0f), glm::vec4(axisZ, 0.0f), glm::vec4(origin, 1.0f)),
		    .colour = ToColour({ColourByte(coneColour.r), ColourByte(coneColour.g), ColourByte(coneColour.b)}),
		    .nearRadius = size * 0.2f,
		    .length = coneLength,
		    .angle = coneAngle,
		};
	}
	return emitter;
}
