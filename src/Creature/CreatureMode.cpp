/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "CreatureMode.h"

#include <cmath>

#include <algorithm>

namespace openblack::creature_mode
{

float CreatureHeight(float size)
{
	return k_HeightPerSize * std::max(size, 0.05f);
}

KeyAction OnCreatureKey(std::optional<entt::entity> following, std::optional<entt::entity> yours)
{
	if (!yours.has_value())
	{
		return KeyAction::None;
	}
	return following == yours ? KeyAction::Leave : KeyAction::Enter;
}

std::optional<entt::entity> DoubleClicks::OnPress(const Press& press)
{
	const auto last = _last;
	_last = press;
	if (!last.has_value() || !press.creature.has_value() || last->creature != press.creature)
	{
		return std::nullopt;
	}
	const bool quick = press.milliseconds - last->milliseconds <= k_DoubleClickMilliseconds;
	const bool still = std::abs(press.screen.x - last->screen.x) <= k_DoubleClickSlop &&
	                   std::abs(press.screen.y - last->screen.y) <= k_DoubleClickSlop;
	if (!quick || !still)
	{
		return std::nullopt;
	}
	// A third quick press starts a new double click rather than making another
	_last.reset();
	return press.creature;
}

std::optional<creature_physiology::Faint> PassOutFrom(const creature_panel::Values& values)
{
	using creature_physiology::Faint;
	if (creature_panel::Percent(values.damage) >= 100)
	{
		return Faint::OutOfLife;
	}
	if (creature_panel::Percent(values.hunger) >= 100)
	{
		return Faint::Starving;
	}
	if (creature_panel::Percent(values.tiredness) >= 100)
	{
		return Faint::Exhausted;
	}
	return std::nullopt;
}

glm::vec3 PenOf(std::optional<glm::vec3> home, std::optional<glm::vec3> temple, glm::vec3 noPen)
{
	return home.value_or(temple.value_or(noPen));
}

} // namespace openblack::creature_mode
