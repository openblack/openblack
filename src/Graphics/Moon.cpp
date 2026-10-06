/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "Moon.h"

#include <cmath>

#include <algorithm>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

using namespace openblack::graphics;

namespace
{
constexpr float k_BasisScale = 4.0f;
/// The moon leans back a little
constexpr float k_Tilt = -0.13089970f;
constexpr float k_MeshScale = 0.65f;
constexpr float k_GlowHalfSize = 500.0f;
constexpr float k_GlowUvMinimum = 0.25f;
constexpr float k_GlowUvMaximum = 0.49375f;

/// Turns two of a matrix's axes into each other by an angle, as the game's matrices turn their rows
void TurnAxes(glm::mat3& m, int i, int j, float c, float s)
{
	const auto a = m[i];
	const auto b = m[j];
	m[i] = (c * a) + (s * b);
	m[j] = (c * b) - (s * a);
}
} // namespace

std::optional<moon::Placement> moon::Place(float scriptHour)
{
	// An hour of the day is a twelfth of half a turn
	const float angle = scriptHour * 0.2617993950843811f;
	const glm::vec3 offset {4000.0f, (1100.0f * std::cos(angle)) - 150.0f, 800.0f * std::sin(angle)};
	const float alpha = std::min(200.0f, std::floor((0.5f * offset.y) - 110.0f));
	if (alpha <= 0.0f)
	{
		return std::nullopt;
	}
	return Placement {.offset = offset, .alpha = alpha};
}

float moon::Phase(int64_t unixTime)
{
	// Whole days from a new moon, in moon months of about 29.5 days
	const auto days = static_cast<int32_t>(unixTime / 86400) - 0x2AD2;
	const auto months = static_cast<float>(static_cast<double>(days) * 0.03386318012808897);
	const float fraction = months - static_cast<float>(static_cast<int32_t>(months));
	return static_cast<float>(static_cast<double>(1.0f - fraction) * 6.2831854820251465);
}

glm::mat3 moon::Basis(const glm::mat4& view, const glm::mat4& inverseView, const glm::vec3& position)
{
	// Its face square to the line from the camera, and upright
	const glm::vec3 inView(view * glm::vec4(position, 1.0f));
	const glm::vec3 facing = inView != glm::vec3(0.0f) ? glm::normalize(inView) : inView;
	glm::vec3 across {facing.z, 0.0f, -facing.x};
	if (across != glm::vec3(0.0f))
	{
		across = glm::normalize(across);
	}
	const glm::vec3 up = glm::cross(facing, across);
	return glm::mat3(inverseView) * glm::mat3(across, up, facing) * k_BasisScale;
}

glm::mat4 moon::Model(const glm::mat3& basis, const glm::vec3& position, float phase)
{
	glm::mat3 axes = basis;
	TurnAxes(axes, 0, 1, std::cos(k_Tilt), -std::sin(k_Tilt));
	const float turn = phase + glm::pi<float>();
	TurnAxes(axes, 0, 2, std::cos(turn), std::sin(turn));
	glm::mat4 model(axes * k_MeshScale);
	model[3] = glm::vec4(position, 1.0f);
	return model;
}

moon::Glow moon::MakeGlow(const glm::mat3& basis, const glm::vec3& position)
{
	const auto across = basis[0] * k_GlowHalfSize;
	const auto up = basis[1] * k_GlowHalfSize;
	return {
	    .corners = {position - up - across, position + across - up, position + up - across, position + up + across},
	    .uvs = {glm::vec2(k_GlowUvMinimum, k_GlowUvMinimum), glm::vec2(k_GlowUvMaximum, k_GlowUvMinimum),
	            glm::vec2(k_GlowUvMinimum, k_GlowUvMaximum), glm::vec2(k_GlowUvMaximum, k_GlowUvMaximum)},
	};
}

glm::vec3 moon::GlowColour(const glm::vec3& moonColour)
{
	return {moonColour.r / 6.0f, moonColour.g / 5.0f, moonColour.b / 4.0f};
}
