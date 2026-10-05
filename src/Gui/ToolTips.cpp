/*******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "ToolTips.h"

#include <algorithm>
#include <utility>

#include <fmt/format.h>

using namespace openblack::gui;

ToolTips::ToolTips(std::span<const ToolTipInfo, k_Count> info)
{
	std::ranges::copy(info, _info.begin());
}

std::string ToolTips::TextName(uint32_t index)
{
	return fmt::format("HELP_TEXT_TOOLTIP_{:02}", index + 1);
}

void ToolTips::Submit(uint32_t index, ToolTipAction action, uint32_t arrows, bool force)
{
	if (index >= k_Count || (_level == ToolTipLevel::Minimum && PriorityOf(index) < k_ImportantPriority))
	{
		return;
	}
	if (index == _current)
	{
		_submitted = true;
	}
	// One being shown keeps the others off for its display time, and a lower one off for as long as it is shown
	if (!force && _displayLeft > 0)
	{
		return;
	}
	if (_current.has_value() && (PriorityOf(*_current) > PriorityOf(index) || (index == *_current && !force)))
	{
		return;
	}
	_important = force;
	_current = index;
	_displayLeft = static_cast<int32_t>(k_TurnsPerSecond * _info.at(index).displayTime);
	_lingerLeft = static_cast<int32_t>(k_TurnsPerSecond * _info.at(index).displayTimeAfterFocus);
	_action = action;
	_arrows = arrows;
	_submitted = true;
	if (PriorityOf(index) < k_ImportantPriority)
	{
		_timesShown.at(index) = std::min(_timesShown.at(index) + 1, k_MaxTimesShown);
	}
}

bool ToolTips::KeepIntelligently()
{
	// Once the tooltip has been shown its display time and has faded in, it fades out over a
	// second, and isn't shown again until it is submitted afresh
	if (_icon.has_value() && _icon->current != _icon->end)
	{
		return true;
	}
	if (_displayLeft > 0 || PriorityOf(*_current) >= k_ImportantPriority)
	{
		return true;
	}
	if (!_icon.has_value())
	{
		return false;
	}
	if (_icon->current < 1.0f)
	{
		return true;
	}
	_icon->start = _icon->current;
	_icon->end = 0.0f;
	_icon->fadeTime = 1.0f;
	_icon->elapsed = 0.0f;
	return true;
}

void ToolTips::ProcessTurn()
{
	// The help system picks this turn's tooltip and its text, then forgets this turn's submission
	const bool submitted = std::exchange(_submitted, false);
	if (_level == ToolTipLevel::None)
	{
		_icon.reset();
		return;
	}
	// A tooltip no longer submitted ends at once, unless it lingers
	if (!submitted)
	{
		if (_lingerLeft <= 0)
		{
			_current.reset();
			_displayLeft = 0;
			_icon.reset();
			return;
		}
		--_lingerLeft;
	}
	if (!_current.has_value() || (_level == ToolTipLevel::Minimum && PriorityOf(*_current) < k_ImportantPriority) ||
	    (_level == ToolTipLevel::Intelligent && !KeepIntelligently()))
	{
		_icon.reset();
		return;
	}
	if (_displayLeft > 0)
	{
		--_displayLeft;
	}
	// Any change starts the fade again
	if (_icon.has_value() && (_icon->index != *_current || _icon->action != _action || _icon->arrows != _arrows))
	{
		_icon.reset();
	}
	if (!_icon.has_value())
	{
		const float fadeTime =
		    !_important && _level != ToolTipLevel::All ? static_cast<float>(_timesShown.at(*_current)) : 0.0f;
		_icon = Icon {.index = *_current,
		              .action = _action,
		              .arrows = _arrows,
		              .start = 0.0f,
		              .end = 1.0f,
		              .fadeTime = fadeTime,
		              .current = 0.0f,
		              .elapsed = 0.0f};
	}
}

void ToolTips::Update(float seconds)
{
	if (!_icon.has_value())
	{
		return;
	}
	auto& icon = *_icon;
	icon.elapsed = std::clamp(icon.elapsed + seconds, 0.0f, icon.fadeTime);
	icon.current = icon.fadeTime > 0.0f ? icon.start + ((icon.end - icon.start) * icon.elapsed / icon.fadeTime) : icon.end;
}

std::optional<ToolTips::Shown> ToolTips::GetShown() const
{
	if (!_icon.has_value())
	{
		return std::nullopt;
	}
	const float alpha = _icon->current <= 0.0f ? 0.0f : std::min(_icon->current, 1.0f);
	return Shown {.index = _icon->index, .action = _icon->action, .arrows = _icon->arrows, .alpha = alpha};
}
