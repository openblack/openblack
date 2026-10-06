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

#include <span>
#include <string_view>

#include "CrashReport.h"

/// Catches every way the game can crash (failed assertions, C runtime errors, abort, std::terminate, fatal signals,
/// unhandled system exceptions and fatal renderer errors) and, instead of a dialog waiting for someone to click it,
/// writes a report with a stack trace to stderr, the game's log and a file in a "crashes" folder (with a minidump on
/// Windows), then exits with a non-zero code. With a debugger attached it breaks into the debugger first.
namespace openblack::crash_handler
{

/// The command-line switch that leaves the operating system's and C runtime's own crash dialogs in place
constexpr std::string_view k_CrashDialogsOption = "--crash-dialogs";

/// Whether the command line asks for the old crash dialogs; read before the options are parsed so that crashes while
/// parsing them are caught too
[[nodiscard]] bool WantsCrashDialogs(std::span<char*> args);

/// Installs every hook. Call it first thing in main, once.
void Install();

/// Whether Install has run, so code that used to throw on fatal errors can still do so when it hasn't
[[nodiscard]] bool IsInstalled();

/// Moves the reports next to the log file and copies them into the log, once the log's destination is known; a log
/// written to the terminal gets nothing more, since stderr already carries the report
void SetLogFile(std::string_view logFile);

/// Reports a fatal error the game detected itself, such as the renderer losing its device, and exits
[[noreturn]] void ReportFatal(crash_report::CrashKind kind, std::string_view message, std::string_view file = {},
                              uint32_t line = 0, std::string_view detail = {});

} // namespace openblack::crash_handler
