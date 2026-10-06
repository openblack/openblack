/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>

#include <deque>
#include <string>
#include <string_view>

namespace openblack::editor::scripts
{

/// The last lines of a log, as many as it keeps, the oldest dropped first, with how many errors and warnings came in
class LogRing
{
public:
	enum class Level : uint8_t
	{
		Trace,
		Debug,
		Info,
		Warning,
		Error,
	};
	struct Line
	{
		Level level {Level::Info};
		std::string text;
		/// The line's number since the log started, so a view can tell new lines from old
		uint64_t number {0};
	};

	explicit LogRing(size_t capacity)
	    : _capacity(capacity)
	{
	}

	void Push(Level level, std::string_view text)
	{
		if (_capacity == 0)
		{
			return;
		}
		if (_lines.size() == _capacity)
		{
			_lines.pop_front();
		}
		_lines.push_back({.level = level, .text = std::string(text), .number = _next++});
		if (level == Level::Error)
		{
			++_errors;
		}
		else if (level == Level::Warning)
		{
			++_warnings;
		}
	}
	void Clear()
	{
		_lines.clear();
		_errors = 0;
		_warnings = 0;
	}
	[[nodiscard]] const std::deque<Line>& Lines() const { return _lines; }
	[[nodiscard]] size_t Errors() const { return _errors; }
	[[nodiscard]] size_t Warnings() const { return _warnings; }
	/// The number the next line will have
	[[nodiscard]] uint64_t Next() const { return _next; }

private:
	size_t _capacity;
	std::deque<Line> _lines;
	size_t _errors {0};
	size_t _warnings {0};
	uint64_t _next {0};
};

} // namespace openblack::editor::scripts
