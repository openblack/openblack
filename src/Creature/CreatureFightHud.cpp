/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureFightHud.h"

#include <algorithm>

#include <glm/common.hpp>

using namespace openblack;
using namespace openblack::creature_fight_hud;

namespace
{
constexpr glm::vec4 k_Green {0.0f, 1.0f, 0.0f, 1.0f};
constexpr glm::vec4 k_Yellow {1.0f, 1.0f, 0.0f, 1.0f};
constexpr glm::vec4 k_Red {1.0f, 0.0f, 0.0f, 1.0f};

// Indexed by species, from the cow
constexpr std::array<std::u16string_view, static_cast<size_t>(CreatureType::_COUNT) - 1> k_SpeciesNames {
    u"Cow",        u"Tiger", u"Leopard", u"Wolf", u"Lion",     u"Horse", u"Tortoise", u"Zebra",     u"Brown Bear",
    u"Polar Bear", u"Sheep", u"Chimp",   u"Ogre", u"Mandrill", u"Rhino", u"Gorilla",  u"Giant Ape",
};
} // namespace

Layout creature_fight_hud::Compute(glm::u16vec2 resolution)
{
	const auto screen = glm::vec2(resolution);
	const auto size = glm::vec2(screen.x * k_WidthShare, screen.y * k_HeightShare);
	const auto rowHeight = size.y / 2.0f;
	const auto margin = rowHeight * k_MarginShare;
	const auto padding = rowHeight * k_PaddingShare;

	Layout layout;
	layout.box = {.min = glm::vec2(margin), .max = glm::vec2(margin) + size};
	layout.textSize = rowHeight * k_TextShare;
	for (size_t i = 0; i < layout.rows.size(); ++i)
	{
		auto& row = layout.rows.at(i);
		const auto top = layout.box.min.y + (rowHeight * static_cast<float>(i)) + padding;
		const auto left = layout.box.min.x + padding;
		const auto right = layout.box.max.x - padding;
		row.name = {left, top};
		const auto healthTop = top + layout.textSize + (padding * 0.5f);
		row.health = {.min = {left, healthTop}, .max = {right, healthTop + (rowHeight * k_HealthShare)}};
		const auto staminaTop = row.health.max.y + (padding * 0.5f);
		row.stamina = {.min = {left, staminaTop}, .max = {right, staminaTop + (rowHeight * k_StaminaShare)}};
	}
	return layout;
}

Rect creature_fight_hud::Filled(const Rect& bar, float value)
{
	const auto fill = std::clamp(value, 0.0f, 1.0f);
	return {.min = bar.min, .max = {bar.min.x + ((bar.max.x - bar.min.x) * fill), bar.max.y}};
}

glm::vec4 creature_fight_hud::BarColour(float value)
{
	const auto fill = std::clamp(value, 0.0f, 1.0f);
	if (fill > 0.5f)
	{
		return glm::mix(k_Yellow, k_Green, (fill - 0.5f) * 2.0f);
	}
	return glm::mix(k_Red, k_Yellow, fill * 2.0f);
}

std::u16string_view creature_fight_hud::SpeciesName(CreatureType species)
{
	const auto index = static_cast<size_t>(species);
	return index >= 1 && index <= k_SpeciesNames.size() ? k_SpeciesNames.at(index - 1) : u"Creature";
}
