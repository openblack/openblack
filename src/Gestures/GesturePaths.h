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
#include <span>
#include <vector>

#include <GestureFile.h>
#include <glm/vec2.hpp>

#include "Enums.h"
#include "Gestures/GestureRecorder.h"

/// Paths drawn by a program rather than a hand, for the testbed and the tests: the cursor's place at each sample, on
/// the screen the recorder measures (768 pixels high), as a hand drawing at a steady speed would leave them.
namespace openblack::gesture
{

/// How far the cursor goes between two samples when a hand draws at an easy pace: about 700 pixels a second
constexpr float k_DrawStepPixels = 20.0f;

/// The samples along straight strokes through the points
[[nodiscard]] std::vector<glm::vec2> Trace(std::span<const glm::vec2> points, float step = k_DrawStepPixels);
/// A template's shape drawn in a square of a size round a middle
[[nodiscard]] std::vector<glm::vec2> TraceTemplate(const gestures::GestureTemplate& gestureTemplate, glm::vec2 middle,
                                                   float size, float step = k_DrawStepPixels);
/// A gesture drawn from the first of its templates the recogniser knows it by, in a square round a middle, or none when
/// no template of it is drawn so as to be recognised
[[nodiscard]] std::optional<std::vector<glm::vec2>> TraceGesture(std::span<const gestures::GestureTemplate> templates,
                                                                 GestureType gesture, glm::vec2 middle, float size,
                                                                 float screenAspect);

/// A circle drawn round a middle, starting at its top, clockwise on the screen or not
[[nodiscard]] std::vector<glm::vec2> TraceCircle(glm::vec2 middle, float radius, bool clockwise, float step = k_DrawStepPixels);
/// A scribble: strokes back and forth across a width, each a little lower than the last
[[nodiscard]] std::vector<glm::vec2> TraceScribble(glm::vec2 middle, float width, int strokes, float step = k_DrawStepPixels);

/// A template made from a recorded path, its key points fitted into a box from 0 to 1, as the game's own templates were
[[nodiscard]] gestures::GestureTemplate MakeTemplate(GestureType gesture, std::span<const KeyPoint> keys, bool checkDirection,
                                                     bool allowMirror, bool checkAspect, float screenAspect);

} // namespace openblack::gesture
