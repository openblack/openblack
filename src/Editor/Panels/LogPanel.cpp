/******************************************************************************
 * Copyright (c) 2018-2026 openblack developers
 *
 * For a complete list of all authors, please refer to contributors.md
 * Interested in contributing? Visit https://github.com/openblack/openblack
 *
 * openblack is licensed under the GNU General Public License version 3.
 *******************************************************************************/

#include "LogPanel.h"

#include <algorithm>
#include <mutex>

#include <imgui.h>
#include <imgui_stdlib.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/spdlog.h>

#include "Editor/EditorOutline.h"
#include "Editor/EditorStyle.h"
#include "Editor/Scripts/LogRing.h"

namespace openblack::editor
{

using scripts::LogRing;

namespace
{
constexpr std::string_view k_LoggerName = "scripting";
constexpr size_t k_KeptLines = 2000;
constexpr std::array<const char*, 5> k_LevelNames {"Trace", "Debug", "Info", "Warnings", "Errors"};

LogRing::Level LevelOf(spdlog::level::level_enum level)
{
	switch (level)
	{
	case spdlog::level::trace:
		return LogRing::Level::Trace;
	case spdlog::level::debug:
		return LogRing::Level::Debug;
	case spdlog::level::warn:
		return LogRing::Level::Warning;
	case spdlog::level::err:
	case spdlog::level::critical:
		return LogRing::Level::Error;
	default:
		return LogRing::Level::Info;
	}
}

ImVec4 ColourOf(LogRing::Level level)
{
	switch (level)
	{
	case LogRing::Level::Error:
		return style::k_Error;
	case LogRing::Level::Warning:
		return style::k_Warning;
	case LogRing::Level::Info:
		return ImGui::GetStyleColorVec4(ImGuiCol_Text);
	default:
		return style::k_Muted;
	}
}
} // namespace

/// Keeps the log's lines in a ring, which the panel reads under the sink's lock
class LogSink final: public spdlog::sinks::base_sink<std::mutex>
{
public:
	LogSink()
	    : _ring(k_KeptLines)
	{
	}

	template <typename Function>
	void Read(Function&& function)
	{
		const std::lock_guard lock(mutex_);
		function(_ring);
	}

protected:
	void sink_it_(const spdlog::details::log_msg& message) override
	{
		_ring.Push(LevelOf(message.level), std::string_view(message.payload.data(), message.payload.size()));
	}
	void flush_() override {}

private:
	LogRing _ring;
};

LogPanel::LogPanel() noexcept
    : _sink(std::make_shared<LogSink>())
{
	Attach();
}

LogPanel::~LogPanel() noexcept
{
	if (_attached)
	{
		if (auto logger = spdlog::get(std::string(k_LoggerName)))
		{
			auto& sinks = logger->sinks();
			sinks.erase(std::remove(sinks.begin(), sinks.end(), _sink), sinks.end());
		}
	}
}

void LogPanel::Attach() noexcept
{
	if (_attached)
	{
		return;
	}
	if (auto logger = spdlog::get(std::string(k_LoggerName)))
	{
		logger->sinks().push_back(_sink);
		_attached = true;
	}
}

size_t LogPanel::Errors() const noexcept
{
	size_t errors = 0;
	_sink->Read([&errors](const LogRing& ring) { errors = ring.Errors(); });
	return errors;
}

void LogPanel::Draw() noexcept
{
	Attach();
	if (!_attached)
	{
		ImGui::TextDisabled("The scripting log hasn't started");
		return;
	}
	ImGui::SetNextItemWidth(ImGui::GetFontSize() * 14.0f);
	ImGui::InputTextWithHint("##Search", "Search the log", &_search);
	for (size_t i = 0; i < _levels.size(); ++i)
	{
		ImGui::SameLine();
		ImGui::Checkbox(k_LevelNames.at(i), &_levels.at(i));
	}
	ImGui::SameLine();
	ImGui::Checkbox("Follow", &_follow);
	ImGui::SameLine();
	bool clear = ImGui::Button("Clear");

	_sink->Read([&](LogRing& ring) {
		if (clear)
		{
			ring.Clear();
		}
		ImGui::SameLine();
		ImGui::TextColored(ring.Errors() > 0 ? style::k_Error : style::k_Muted, "%zu errors, %zu warnings", ring.Errors(),
		                   ring.Warnings());
		ImGui::BeginChild("Lines", ImVec2(0.0f, 0.0f), ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
		for (const auto& line : ring.Lines())
		{
			if (!_levels.at(static_cast<size_t>(line.level)) || !MatchesSearch(line.text, _search))
			{
				continue;
			}
			ImGui::TextColored(ColourOf(line.level), "%s", line.text.c_str());
		}
		if (_follow && ImGui::GetScrollY() >= ImGui::GetScrollMaxY() - ImGui::GetTextLineHeight())
		{
			ImGui::SetScrollHereY(1.0f);
		}
		ImGui::EndChild();
	});
}

} // namespace openblack::editor
