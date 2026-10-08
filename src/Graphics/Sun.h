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

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/// Black & White's sun: a square far out to the north west, facing the island, and its glare over the view
namespace openblack::graphics::sun
{

/// Where the sun stands at an hour of script time and how strongly it shows, 0 to 255: it rises from 6 to noon to 7500
/// units high and sets as it rose, coming up from 3 and going down after 18 by a third of the way each hour. None
/// while it is down.
struct Placement
{
	glm::vec3 position;
	float alpha;
};
[[nodiscard]] std::optional<Placement> Place(float scriptHour);

/// The glare is the sun again, larger, over the finished view
inline constexpr float k_GlareScale = 1.8f;

/// Where the glare looks for what is in the way: the sun's middle and four points 500 units from it across the world's x
/// and up its y, each moved on by the near plane's distance along the line from the camera to the sun, and never lower
/// than 10
inline constexpr std::array<glm::vec2, 5> k_GlareSamples = {{
    {0.0f, 0.0f},
    {-500.0f, -500.0f},
    {-500.0f, 500.0f},
    {500.0f, -500.0f},
    {500.0f, 500.0f},
}};
/// The samples are never lower than this
inline constexpr float k_GlareLowestSample = 10.0f;
[[nodiscard]] std::array<glm::vec3, 5> GlareSamples(glm::vec3 sun, glm::vec3 camera, float near);

/// A sample the land doesn't hide is hidden by what is drawn at its pixel, when that is nearer the camera than this: the
/// game hides it where its depth buffer, of 16 bits holding 1 - near / depth, is below 65000
[[nodiscard]] float GlareHidingDepth(float near);

/// The glare's strength, 0 to 255, eased towards a fifth less for each hidden sample by a hundredth of the way for
/// each millisecond of game time; it stays put while the game is paused
[[nodiscard]] float EaseGlare(float glare, int hiddenSamples, uint32_t frameMilliseconds);

} // namespace openblack::graphics::sun
