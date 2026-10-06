/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <array>
#include <string>
#include <string_view>

#include <glm/gtc/type_precision.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

#include "Enums.h"

/// The panel shown over the screen during a fight: a see-through box at the top left, a row for each creature with its
/// name over a bar of its fight health and a thinner bar of its stamina. The bars run green to yellow while more than
/// half full and yellow to red below.
namespace openblack::creature_fight_hud
{
/// The box is this share of the screen wide and high
constexpr float k_WidthShare = 33.0f / 70.0f;
constexpr float k_HeightShare = 9.0f / 70.0f;
/// The box's distance from the screen's corner and the space inside it, as shares of a row's height
constexpr float k_MarginShare = 0.15f;
constexpr float k_PaddingShare = 0.12f;
/// The name's size, and the health and stamina bars' heights, as shares of a row's height
constexpr float k_TextShare = 0.36f;
constexpr float k_HealthShare = 0.22f;
constexpr float k_StaminaShare = 0.1f;

struct Values
{
	struct Side
	{
		std::u16string name;
		float health {1.0f};
		float stamina {1.0f};
	};
	std::array<Side, 2> sides;
};

struct Rect
{
	glm::vec2 min {0.0f};
	glm::vec2 max {0.0f};
};

struct Layout
{
	Rect box;
	float textSize {0.0f};
	struct Row
	{
		glm::vec2 name;
		Rect health;
		Rect stamina;
	};
	std::array<Row, 2> rows;
};

/// Where everything goes on a screen of a resolution, in pixels
[[nodiscard]] Layout Compute(glm::u16vec2 resolution);
/// A bar's part filled, from its left, for a value from 0 to 1
[[nodiscard]] Rect Filled(const Rect& bar, float value);
/// A bar's colour for a value from 0 to 1
[[nodiscard]] glm::vec4 BarColour(float value);
/// The name a creature is shown by: its species', as creatures have no names of their own yet
[[nodiscard]] std::u16string_view SpeciesName(CreatureType species);
} // namespace openblack::creature_fight_hud
