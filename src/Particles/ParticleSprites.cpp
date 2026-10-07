/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ParticleSprites.h"

#include <cmath>

#include <algorithm>

using namespace openblack::particles;
using namespace openblack::graphics;

namespace
{
/// A sprite centred at its base is raised by half its height
constexpr float k_Half = 0.5f;

/// The plane's corners in the order of Corners, -1..1 across and up
constexpr std::array<glm::vec2, 4> k_PlaneCorners {{{-1.0f, 1.0f}, {1.0f, 1.0f}, {1.0f, -1.0f}, {-1.0f, -1.0f}}};
} // namespace

sprites::SpriteInstance sprites::InstanceOf(const Effect::DrawAtom& atom)
{
	const auto& creator = *atom.creator;
	const float halfWidth = std::max(atom.scale, k_MinimumHalfWidth);
	const float halfHeight = halfWidth * atom.stretch;
	glm::vec3 position = atom.position;
	if (creator.centreAtBase)
	{
		position.y += halfHeight * k_Half;
	}
	const int frame = maths::FrameIndex(atom.frame, creator.numFrames, creator.loopAnim);
	const auto cell = maths::SpriteCellUv(creator.fileOffset + frame, creator.spritesPerRow);
	const float angle = creator.ignoreRotation ? 0.0f : maths::SpriteAngle(atom.rotation);
	const float alpha = std::clamp(atom.alpha * static_cast<float>(creator.scaleAlpha) / 255.0f, 0.0f, 255.0f);
	return {
	    .positionHalfWidth = {position, halfWidth},
	    .shape = {halfHeight, angle, creator.origin.x * halfWidth, creator.origin.y * halfHeight},
	    .uv = {cell.corner, cell.size},
	    .colour = glm::vec4(atom.rgb[0], atom.rgb[1], atom.rgb[2], std::trunc(alpha)) / 255.0f,
	    .flags = {creator.horizontal ? 1.0f : 0.0f, creator.useLandscapeColour ? 1.0f : 0.0f, 0.0f, 0.0f},
	};
}

render_modes::Mode sprites::RenderMode(const Creator& creator)
{
	if (creator.additive)
	{
		return creator.writeDepth ? render_modes::Mode::AlphaTexturedAlphaAdditive
		                          : render_modes::Mode::AlphaTexturedAlphaAdditiveNz;
	}
	return creator.writeDepth ? render_modes::Mode::AlphaTexturedAlpha : render_modes::Mode::AlphaTexturedAlphaNz;
}

std::array<glm::vec3, 4> sprites::Corners(const SpriteInstance& sprite, const glm::vec3& cameraRight, const glm::vec3& cameraUp)
{
	const glm::vec3 position(sprite.positionHalfWidth);
	const float halfWidth = sprite.positionHalfWidth.w;
	const float halfHeight = sprite.shape.x;
	const float c = std::cos(sprite.shape.y);
	const float s = std::sin(sprite.shape.y);
	const glm::vec2 origin(sprite.shape.z, sprite.shape.w);
	std::array<glm::vec3, 4> corners {};
	for (size_t i = 0; i < corners.size(); ++i)
	{
		const auto plane = k_PlaneCorners.at(i);
		if (sprite.flags.x > 0.5f)
		{
			// On the ground, turned by its yaw: its top edge towards -z
			const float x = plane.x * halfWidth - origin.x;
			const float z = -plane.y * halfHeight - origin.y;
			corners.at(i) = position + glm::vec3(c, 0.0f, s) * x + glm::vec3(-s, 0.0f, c) * z;
		}
		else
		{
			// In the plane of the screen, rolled by its angle
			const float x = plane.x * halfWidth - origin.x;
			const float y = plane.y * halfHeight - origin.y;
			corners.at(i) = position + cameraRight * (x * c + y * s) + cameraUp * (y * c - x * s);
		}
	}
	return corners;
}
