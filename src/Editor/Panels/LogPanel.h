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
#include <memory>
#include <string>

namespace openblack::editor
{

class LogSink;

/// The scripts' output and errors as the scripting log has them, kept from when the editor starts, with the
/// levels to show, a search and the newest lines followed
class LogPanel
{
public:
	LogPanel() noexcept;
	LogPanel(const LogPanel&) = delete;
	LogPanel& operator=(const LogPanel&) = delete;
	~LogPanel() noexcept;

	void Draw() noexcept;
	/// How many errors have come in, for the tab's title
	[[nodiscard]] size_t Errors() const noexcept;
	/// Starts taking the scripting log's lines once it exists
	void Attach() noexcept;

private:
	std::shared_ptr<LogSink> _sink;
	bool _attached {false};
	std::string _search;
	std::array<bool, 5> _levels {false, true, true, true, true};
	bool _follow {true};
};

} // namespace openblack::editor
